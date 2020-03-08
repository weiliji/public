#pragma  once
#include "Base/JSON.h"
#include <utility>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace Public{
namespace Base {

class JsonWriter
{
public:
	JsonWriter() {}
	virtual ~JsonWriter() {}
public:
	std::string document_;
protected:
	static bool isControlCharacter(char ch)
	{
		return ch > 0 && ch <= 0x1F;
	}

	static bool containsControlCharacter(const char* str)
	{
		while (*str)
		{
			if (isControlCharacter(*(str++)))
				return true;
		}
		return false;
	}

	std::string valueToQuotedString(const char *value)
	{
		// Not sure how to handle unicode...
		if (strpbrk(value, "\"\\\b\f\n\r\t") == NULL && !containsControlCharacter(value))
			return std::string("\"") + value + "\"";
		// We have to walk value and escape any special characters.
		// Appending to std::string is not efficient, but this should be rare.
		// (Note: forward slashes are *not* rare, but I am not escaping them.)
		unsigned maxsize = (unsigned)strlen(value) * 2 + 3; // allescaped+quotes+NULL
		std::string result;
		result.reserve(maxsize); // to avoid lots of mallocs
		result += "\"";
		for (const char* c = value; *c != 0; ++c)
		{
			switch (*c)
			{
			case '\"':
				result += "\\\"";
				break;
			case '\\':
				result += "\\\\";
				break;
			case '\b':
				result += "\\b";
				break;
			case '\f':
				result += "\\f";
				break;
			case '\n':
				result += "\\n";
				break;
			case '\r':
				result += "\\r";
				break;
			case '\t':
				result += "\\t";
				break;
				//case '/':
				// Even though \/ is considered a legal escape in JSON, a bare
				// slash is also legal, so I see no reason to escape it.
				// (I hope I am not misunderstanding something.
				// blep notes: actually escaping \/ may be useful in javascript to avoid </ 
				// sequence.
				// Should add a flag to allow this compatibility mode and prevent this 
				// sequence from occurring.
			default:
				if (isControlCharacter(*c))
				{
					std::ostringstream oss;
					oss << "\\u" << std::hex << std::uppercase << std::setfill('0') << std::setw(4) << static_cast<int>(*c);
					result += oss.str();
				}
				else
				{
					result += *c;
				}
				break;
			}
		}
		result += "\"";
		return result;
	}
};

class JsonFastWriter :public JsonWriter
{
public:
	JsonFastWriter(const JsonValue& root,bool enableyaml = false):yamlCompatiblityEnabled_(enableyaml)
	{
		writeValue(root);
	}
private:

	void  writeValue(const JsonValue &value)
	{
		switch (value.type())
		{
		case JsonValue::realValue:
		case JsonValue::intValue:
		case JsonValue::booleanValue:
			if (value.data().type() == Value::Type_Int64 || value.data().type() == Value::Type_Uint64)
			{
				document_ += valueToQuotedString(value->readString().c_str());
			}
			else
			{
				document_ += value->readString();
			}
			break;
		case JsonValue::stringValue:
			document_ += valueToQuotedString(value->readString().c_str());
			break;
		case JsonValue::arrayValue:
		{
			document_ += "[";
			size_t size = value.size();
			for (size_t index = 0; index < size; ++index)
			{
				if (index > 0)
					document_ += ",";
				writeValue(value[index]);
			}
			document_ += "]";
		}
		break;
		case JsonValue::objectValue:
		{
			const std::map<std::string, JsonValue>& members = value.object();
			document_ += "{";
			for (std::map<std::string, JsonValue>::const_iterator it = members.begin(); it != members.end(); it++)
			{
				const std::string &name = it->first;
				if (it != members.begin())
					document_ += ",";
				document_ += valueToQuotedString(name.c_str());
				document_ += yamlCompatiblityEnabled_ ? ": "
					: ":";
				writeValue(it->second);
			}
			document_ += "}";
		}
		break;
		case JsonValue::nullValue:
		default:
			document_ += "null";
			break;
		}
	}
private:
	bool yamlCompatiblityEnabled_ = false;
};

class JsonStyleWriter:public JsonWriter
{
public:
	JsonStyleWriter(const JsonValue& root) 
		: rightMargin_(74)
		, indentSize_(3)
	{
		writeCommentBeforeValue(root);
		writeValue(root);
		writeCommentAfterValueOnSameLine(root);
		document_ += "\n";
	}
private:

	void writeValue(const JsonValue &value)
	{
		switch (value.type())
		{
		case JsonValue::realValue:
		case JsonValue::intValue:
		case JsonValue::booleanValue:
			if (value.data().type() == Value::Type_Int64 || value.data().type() == Value::Type_Uint64)
			{
				pushValue(valueToQuotedString(value->readString().c_str()));
			}
			else
			{
				pushValue(value->readString());
			}
			break;
		case JsonValue::stringValue:
			pushValue(valueToQuotedString(value->readString().c_str()));
			break;
		case JsonValue::arrayValue:
			writeArrayValue(value);
			break;
		case JsonValue::objectValue:
		{
			const std::map<std::string, JsonValue>& members = value.object();
			if (members.size() == 0)
				pushValue("{}");
			else
			{
				writeWithIndent("{");
				indent();

				for(std::map<std::string, JsonValue>::const_iterator it = members.begin();it != members.end();it++)
				{
					const std::string &name = it->first;
					const JsonValue &childValue = it->second;
					writeCommentBeforeValue(childValue);
					writeWithIndent(valueToQuotedString(name.c_str()));
					document_ += " : ";
					writeValue(childValue);
					if (++it == members.end())
					{
						writeCommentAfterValueOnSameLine(childValue);
						break;
					}
					document_ += ",";
					writeCommentAfterValueOnSameLine(childValue);
				}
				unindent();
				writeWithIndent("}");
			}
		}
		break;
		case JsonValue::nullValue:
		default:
			pushValue("null");
			break;
		}
	}


	void writeArrayValue(const JsonValue &value)
	{
		size_t size = value.size();
		if (size == 0)
			pushValue("[]");
		else
		{
			bool isArrayMultiLine = isMultineArray(value);
			if (isArrayMultiLine)
			{
				writeWithIndent("[");
				indent();
				bool hasChildValue = !childValues_.empty();
				unsigned index = 0;
				while (true)
				{
					const JsonValue &childValue = value[index];
					writeCommentBeforeValue(childValue);
					if (hasChildValue)
						writeWithIndent(childValues_[index]);
					else
					{
						writeIndent();
						writeValue(childValue);
					}
					if (++index == size)
					{
						writeCommentAfterValueOnSameLine(childValue);
						break;
					}
					document_ += ",";
					writeCommentAfterValueOnSameLine(childValue);
				}
				unindent();
				writeWithIndent("]");
			}
			else // output on a single line
			{
				assert(childValues_.size() == size);
				document_ += "[ ";
				for (unsigned index = 0; index < size; ++index)
				{
					if (index > 0)
						document_ += ", ";
					document_ += childValues_[index];
				}
				document_ += " ]";
			}
		}
	}


	bool isMultineArray(const JsonValue &value)
	{
		size_t size = value.size();
		bool isMultiLine = size * 3 >= (size_t)rightMargin_;
		childValues_.clear();
		for (size_t index = 0; index < size && !isMultiLine; ++index)
		{
			const JsonValue &childValue = value[index];
			isMultiLine = isMultiLine ||
				((childValue.isArray() || childValue.isObject()) &&
					childValue.size() > 0);
		}
		if (!isMultiLine) // check if line length > max line length
		{
			childValues_.reserve(size);
			addChildValues_ = true;
			size_t lineLength = 4 + (size - 1) * 2; // '[ ' + ', '*n + ' ]'
			for (size_t index = 0; index < size && !isMultiLine; ++index)
			{
				writeValue(value[index]);
				lineLength += childValues_[index].length();
				isMultiLine = isMultiLine  &&  hasCommentForValue(value[index]);
			}
			addChildValues_ = false;
			isMultiLine = isMultiLine || lineLength >= (size_t)rightMargin_;
		}
		return isMultiLine;
	}


	void pushValue(const std::string &value)
	{
		if (addChildValues_)
			childValues_.push_back(value);
		else
			document_ += value;
	}


	void writeIndent()
	{
		if (!document_.empty())
		{
			char last = document_[document_.length() - 1];
			if (last == ' ')     // already indented
				return;
			if (last != '\n')    // Comments may add new-line
				document_ += '\n';
		}
		document_ += indentString_;
	}


	void writeWithIndent(const std::string &value)
	{
		writeIndent();
		document_ += value;
	}


	void indent()
	{
		indentString_ += std::string(indentSize_, ' ');
	}


	void unindent()
	{
		assert(int(indentString_.size()) >= indentSize_);
		indentString_.resize(indentString_.size() - indentSize_);
	}


	void writeCommentBeforeValue(const JsonValue &root)
	{
		if (!root.hasComment(JsonValue::commentBefore))
			return;
		document_ += normalizeEOL(root.getComment(JsonValue::commentBefore));
		document_ += "\n";
	}


	void writeCommentAfterValueOnSameLine(const JsonValue &root)
	{
		if (root.hasComment(JsonValue::commentAfterOnSameLine))
			document_ += " " + normalizeEOL(root.getComment(JsonValue::commentAfterOnSameLine));

		if (root.hasComment(JsonValue::commentAfter))
		{
			document_ += "\n";
			document_ += normalizeEOL(root.getComment(JsonValue::commentAfter));
			document_ += "\n";
		}
	}


	bool hasCommentForValue(const JsonValue &value)
	{
		return value.hasComment(JsonValue::commentBefore)
			|| value.hasComment(JsonValue::commentAfterOnSameLine)
			|| value.hasComment(JsonValue::commentAfter);
	}


	std::string normalizeEOL(const std::string &text)
	{
		std::string normalized;
		normalized.reserve(text.length());
		const char *begin = text.c_str();
		const char *end = begin + text.length();
		const char *current = begin;
		while (current != end)
		{
			char c = *current++;
			if (c == '\r') // mac or dos EOL
			{
				if (*current == '\n') // convert dos EOL
					++current;
				normalized += '\n';
			}
			else // handle unix EOL & other char
				normalized += c;
		}
		return normalized;
	}
private:
	typedef std::vector<std::string> ChildValues;

	ChildValues childValues_;
	std::string indentString_;
	int rightMargin_ = 74;
	int indentSize_ = 3;
	bool addChildValues_ = false;
};

}
} // namespace Json

