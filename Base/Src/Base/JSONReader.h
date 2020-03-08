#pragma  once
#include "Base/JSON.h"
#include "Base/ErrorInfo.h"
#include "JSONWriter.h"
# include <deque>
# include <stack>
# include <string>
# include <iostream>

namespace Public {
namespace Base {

	class Features
	{
	public:
		Features()
			: allowComments_(true)
			, strictRoot_(false)
		{
		}


		static Features all()
		{
			return Features();
		}


		static Features strictMode()
		{
			Features features;
			features.allowComments_ = false;
			features.strictRoot_ = true;
			return features;
		}

		/// \c true if comments are allowed. Default: \c true.
		bool allowComments_;

		/// \c true if root must be either an array or an object value. Default: \c false.
		bool strictRoot_;
	};

class JsonReader
{
	typedef char Char;
	typedef const Char *Location;
	enum TokenType
	{
		tokenEndOfStream = 0,
		tokenObjectBegin,
		tokenObjectEnd,
		tokenArrayBegin,
		tokenArrayEnd,
		tokenString,
		tokenNumber,
		tokenTrue,
		tokenFalse,
		tokenNull,
		tokenArraySeparator,
		tokenMemberSeparator,
		tokenComment,
		tokenError
	};

	class Token
	{
	public:
		TokenType type_;
		Location start_;
		Location end_;
	};

	class FmtErrorInfo
	{
	public:
		Token token_;
		std::string message_;
		Location extra_;
	};

	typedef std::deque<FmtErrorInfo> Errors;

	typedef std::stack<JsonValue *> Nodes;
private:
	const int32_t minInt = int32_t(~(uint32_t(-1) / 2));
	const int32_t maxInt = int32_t(uint32_t(-1) / 2);
	const uint32_t maxUInt = uint32_t(-1);
public:
	JsonReader() : features_(Features::all())
	{}
	ErrorInfo parse(const char* bufferaddr,int bufferlen, JsonValue& root, bool collectComments = false)
	{
		bool ret = parse(bufferaddr, bufferaddr + bufferlen, root,collectComments);
		if (ret) return ErrorInfo();

		return ErrorInfo(Error_Code_Fail, getFormatedErrorMessages());
	}
private:
	bool parse(const char *beginDoc, const char *endDoc,JsonValue &root,bool collectComments)
	{
		/*if (!features_.allowComments_)
		{
			collectComments = false;
		}*/

		begin_ = beginDoc;
		end_ = endDoc;
		collectComments_ = collectComments;
		current_ = begin_;
		lastValueEnd_ = 0;
		lastValue_ = 0;
		commentsBefore_ = "";
		errors_.clear();
		while (!nodes_.empty())
			nodes_.pop();
		nodes_.push(&root);

		bool successful = readValue();
		Token token;
		skipCommentTokens(token);
		if (collectComments_ && !commentsBefore_.empty())
			root.setComment(commentsBefore_, JsonValue::commentAfter);
		if (features_.strictRoot_)
		{
			if (!root.isArray() && !root.isObject())
			{
				// Set error location to start of doc, ideally should be first token found in doc
				token.type_ = tokenError;
				token.start_ = beginDoc;
				token.end_ = endDoc;
				addError("A valid JSON document must be either an array or an object value.",token);
				return false;
			}
		}
		return successful;
	}

	bool readValue()
	{
		Token token;
		skipCommentTokens(token);
		bool successful = true;

		if (collectComments_ && !commentsBefore_.empty())
		{
			currentValue().setComment(commentsBefore_, JsonValue::commentBefore);
			commentsBefore_ = "";
		}


		switch (token.type_)
		{
		case tokenObjectBegin:
			successful = readObject(token);
			break;
		case tokenArrayBegin:
			successful = readArray(token);
			break;
		case tokenNumber:
			successful = decodeNumber(token);
			break;
		case tokenString:
			successful = decodeString(token);
			break;
		case tokenTrue:
			currentValue() = Value(true);
			break;
		case tokenFalse:
			currentValue() = Value(false);
			break;
		case tokenNull:
			currentValue() = Value();
			break;
		default:
			return addError("Syntax error: value, object or array expected.", token);
		}

		if (collectComments_)
		{
			lastValueEnd_ = current_;
			lastValue_ = &currentValue();
		}

		return successful;
	}


	void skipCommentTokens(Token &token)
	{
		if (features_.allowComments_)
		{
			do
			{
				readToken(token);
			} while (token.type_ == tokenComment);
		}
		else
		{
			readToken(token);
		}
	}


	bool expectToken(TokenType type, Token &token, const char *message)
	{
		readToken(token);
		if (token.type_ != type)
			return addError(message, token);
		return true;
	}


	bool readToken(Token &token)
	{
		skipSpaces();
		token.start_ = current_;
		Char c = getNextChar();
		bool ok = true;
		switch (c)
		{
		case '{':
			token.type_ = tokenObjectBegin;
			break;
		case '}':
			token.type_ = tokenObjectEnd;
			break;
		case '[':
			token.type_ = tokenArrayBegin;
			break;
		case ']':
			token.type_ = tokenArrayEnd;
			break;
		case '"':
			token.type_ = tokenString;
			ok = readString();
			break;
		case '/':
			token.type_ = tokenComment;
			ok = readComment();
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case '-':
			token.type_ = tokenNumber;
			readNumber();
			break;
		case 't':
			token.type_ = tokenTrue;
			ok = match("rue", 3);
			break;
		case 'f':
			token.type_ = tokenFalse;
			ok = match("alse", 4);
			break;
		case 'n':
			token.type_ = tokenNull;
			ok = match("ull", 3);
			break;
		case ',':
			token.type_ = tokenArraySeparator;
			break;
		case ':':
			token.type_ = tokenMemberSeparator;
			break;
		case 0:
			token.type_ = tokenEndOfStream;
			break;
		default:
			ok = false;
			break;
		}
		if (!ok)
			token.type_ = tokenError;
		token.end_ = current_;
		return true;
	}


	void skipSpaces()
	{
		while (current_ != end_)
		{
			Char c = *current_;
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
				++current_;
			else
				break;
		}
	}


	bool match(Location pattern,
			int patternLength)
	{
		if (end_ - current_ < patternLength)
			return false;
		int index = patternLength;
		while (index--)
			if (current_[index] != pattern[index])
				return false;
		current_ += patternLength;
		return true;
	}


	bool readComment()
	{
		Location commentBegin = current_ - 1;
		Char c = getNextChar();
		bool successful = false;
		if (c == '*')
			successful = readCStyleComment();
		else if (c == '/')
			successful = readCppStyleComment();
		if (!successful)
			return false;

		if (collectComments_)
		{
			JsonValue::CommentPlacement placement = JsonValue::commentBefore;
			if (lastValueEnd_ && !containsNewLine(lastValueEnd_, commentBegin))
			{
				if (c != '*' || !containsNewLine(commentBegin, current_))
					placement = JsonValue::commentAfterOnSameLine;
			}

			addComment(commentBegin, current_, placement);
		}
		return true;
	}


	void addComment(Location begin,
			Location end,
		JsonValue::CommentPlacement placement)
	{
		assert(collectComments_);
		if (placement == JsonValue::commentAfterOnSameLine)
		{
			assert(lastValue_ != 0);
			lastValue_->setComment(std::string(begin, end), placement);
		}
		else
		{
			if (!commentsBefore_.empty())
				commentsBefore_ += "\n";
			commentsBefore_ += std::string(begin, end);
		}
	}


	bool readCStyleComment()
	{
		while (current_ != end_)
		{
			Char c = getNextChar();
			if (c == '*'  &&  *current_ == '/')
				break;
		}
		return getNextChar() == '/';
	}


	bool readCppStyleComment()
	{
		while (current_ != end_)
		{
			Char c = getNextChar();
			if (c == '\r' || c == '\n')
				break;
		}
		return true;
	}


	void readNumber()
	{
		while (current_ != end_)
		{
			if (!(*current_ >= '0'  &&  *current_ <= '9') &&
				!in(*current_, '.', 'e', 'E', '+', '-'))
				break;
			++current_;
		}
	}

	bool readString()
	{
		Char c = 0;
		while (current_ != end_)
		{
			c = getNextChar();
			if (c == '\\')
				getNextChar();
			else if (c == '"')
				break;
		}
		return c == '"';
	}


	bool readObject(Token &tokenStart)
	{
		Token tokenName;
		std::string name;
		currentValue() = JsonValue(JsonValue::objectValue);
		while (readToken(tokenName))
		{
			bool initialTokenOk = true;
			while (tokenName.type_ == tokenComment  &&  initialTokenOk)
				initialTokenOk = readToken(tokenName);
			if (!initialTokenOk)
				break;
			if (tokenName.type_ == tokenObjectEnd  &&  name.empty())  // empty object
				return true;
			if (tokenName.type_ != tokenString)
				break;

			name = "";
			if (!decodeString(tokenName, name))
				return recoverFromError(tokenObjectEnd);

			Token colon;
			if (!readToken(colon) || colon.type_ != tokenMemberSeparator)
			{
				return addErrorAndRecover("Missing ':' after object member name",
					colon,
					tokenObjectEnd);
			}
			JsonValue &value = currentValue()[name];
			nodes_.push(&value);
			bool ok = readValue();
			nodes_.pop();
			if (!ok) // error already set
				return recoverFromError(tokenObjectEnd);

			Token comma;
			if (!readToken(comma)
				|| (comma.type_ != tokenObjectEnd  &&
					comma.type_ != tokenArraySeparator &&
					comma.type_ != tokenComment))
			{
				return addErrorAndRecover("Missing ',' or '}' in object declaration",
					comma,
					tokenObjectEnd);
			}
			bool finalizeTokenOk = true;
			while (comma.type_ == tokenComment &&
				finalizeTokenOk)
				finalizeTokenOk = readToken(comma);
			if (comma.type_ == tokenObjectEnd)
				return true;
		}
		return addErrorAndRecover("Missing '}' or object member name",
			tokenName,
			tokenObjectEnd);
	}


	bool readArray(Token &tokenStart)
	{
		currentValue() = JsonValue(JsonValue::arrayValue);
		skipSpaces();
		if (*current_ == ']') // empty array
		{
			Token endArray;
			readToken(endArray);
			return true;
		}
		int index = 0;
		while (true)
		{
			JsonValue &value = currentValue()[index++];
			nodes_.push(&value);
			bool ok = readValue();
			nodes_.pop();
			if (!ok) // error already set
				return recoverFromError(tokenArrayEnd);

			Token token;
			// Accept Comment after last item in the array.
			ok = readToken(token);
			while (token.type_ == tokenComment  &&  ok)
			{
				ok = readToken(token);
			}
			bool badTokenType = (token.type_ == tokenArraySeparator  &&
				token.type_ == tokenArrayEnd);
			if (!ok || badTokenType)
			{
				return addErrorAndRecover("Missing ',' or ']' in array declaration",
					token,
					tokenArrayEnd);
			}
			if (token.type_ == tokenArrayEnd)
				break;
		}
		return true;
	}


	bool decodeNumber(Token &token)
	{
		bool isDouble = false;
		for (Location inspect = token.start_; inspect != token.end_; ++inspect)
		{
			isDouble = isDouble
				|| in(*inspect, '.', 'e', 'E', '+')
				|| (*inspect == '-'  &&  inspect != token.start_);
		}
		if (isDouble)
			return decodeDouble(token);
		Location current = token.start_;
		bool isNegative = *current == '-';
		if (isNegative)
			++current;
		uint32_t threshold = (isNegative ? uint32_t(-minInt): maxUInt) / 10;
		uint32_t value = 0;
		while (current < token.end_)
		{
			Char c = *current++;
			if (c < '0' || c > '9')
				return addError("'" + std::string(token.start_, token.end_) + "' is not a number.", token);
			if (value >= threshold)
				return decodeDouble(token);
			value = value * 10 + uint32_t(c - '0');
		}
		if (isNegative)
			currentValue() = Value(-int32_t(value));
		else if (value <= uint32_t(maxInt))
			currentValue() = Value(int32_t(value));
		else
			currentValue() = Value(value);
		return true;
	}


	bool decodeDouble(Token &token)
	{
		double value = 0;
		const int bufferSize = 32;
		int count;
		int length = int(token.end_ - token.start_);
		if (length <= bufferSize)
		{
			Char buffer[bufferSize];
			memcpy(buffer, token.start_, length);
			buffer[length] = 0;
			count = sscanf(buffer, "%lf", &value);
		}
		else
		{
			std::string buffer(token.start_, token.end_);
			count = sscanf(buffer.c_str(), "%lf", &value);
		}

		if (count != 1)
			return addError("'" + std::string(token.start_, token.end_) + "' is not a number.", token);
		currentValue() = Value(value);
		return true;
	}


	bool decodeString(Token &token)
	{
		std::string decoded;
		if (!decodeString(token, decoded))
			return false;
		currentValue() = Value(decoded);
		return true;
	}


	bool decodeString(Token &token, std::string &decoded)
	{
		decoded.reserve(token.end_ - token.start_ - 2);
		Location current = token.start_ + 1; // skip '"'
		Location end = token.end_ - 1;      // do not include '"'
		while (current != end)
		{
			Char c = *current++;
			if (c == '"')
				break;
			else if (c == '\\')
			{
				if (current == end)
					return addError("Empty escape sequence in string", token, current);
				Char escape = *current++;
				switch (escape)
				{
				case '"': decoded += '"'; break;
				case '/': decoded += '/'; break;
				case '\\': decoded += '\\'; break;
				case 'b': decoded += '\b'; break;
				case 'f': decoded += '\f'; break;
				case 'n': decoded += '\n'; break;
				case 'r': decoded += '\r'; break;
				case 't': decoded += '\t'; break;
				case 'u':
				{
					unsigned int unicode = 0;
					if (!decodeUnicodeCodePoint(token, current, end, unicode))
						return false;
					decoded += codePointToUTF8(unicode);
				}
				break;
				default:
					return addError("Bad escape sequence in string", token, current);
				}
			}
			else
			{
				decoded += c;
			}
		}
		return true;
	}

	bool decodeUnicodeCodePoint(Token &token,
			Location &current,
			Location end,
			unsigned int &unicode)
	{

		if (!decodeUnicodeEscapeSequence(token, current, end, unicode))
			return false;
		if (unicode >= 0xD800 && unicode <= 0xDBFF)
		{
			// surrogate pairs
			if (end - current < 6)
				return addError("additional six characters expected to parse unicode surrogate pair.", token, current);
			unsigned int surrogatePair;
			if (*(current++) == '\\' && *(current++) == 'u')
			{
				if (decodeUnicodeEscapeSequence(token, current, end, surrogatePair))
				{
					unicode = 0x10000 + ((unicode & 0x3FF) << 10) + (surrogatePair & 0x3FF);
				}
				else
					return false;
			}
			else
				return addError("expecting another \\u token to begin the second half of a unicode surrogate pair", token, current);
		}
		return true;
	}

	bool decodeUnicodeEscapeSequence(Token &token,
			Location &current,
			Location end,
			unsigned int &unicode)
	{
		if (end - current < 4)
			return addError("Bad unicode escape sequence in string: four digits expected.", token, current);
		unicode = 0;
		for (int index = 0; index < 4; ++index)
		{
			Char c = *current++;
			unicode *= 16;
			if (c >= '0'  &&  c <= '9')
				unicode += c - '0';
			else if (c >= 'a'  &&  c <= 'f')
				unicode += c - 'a' + 10;
			else if (c >= 'A'  &&  c <= 'F')
				unicode += c - 'A' + 10;
			else
				return addError("Bad unicode escape sequence in string: hexadecimal digit expected.", token, current);
		}
		return true;
	}


	bool addError(const std::string &message,
			Token &token,
			Location extra = 0)
	{
		FmtErrorInfo info;
		info.token_ = token;
		info.message_ = message;
		info.extra_ = extra;
		errors_.push_back(info);
		return false;
	}


	bool recoverFromError(TokenType skipUntilToken)
	{
		int errorCount = int(errors_.size());
		Token skip;
		while (true)
		{
			if (!readToken(skip))
				errors_.resize(errorCount); // discard errors caused by recovery
			if (skip.type_ == skipUntilToken || skip.type_ == tokenEndOfStream)
				break;
		}
		errors_.resize(errorCount);
		return false;
	}


	bool addErrorAndRecover(const std::string &message,
			Token &token,
			TokenType skipUntilToken)
	{
		addError(message, token);
		return recoverFromError(skipUntilToken);
	}


	JsonValue & currentValue()
	{
		return *(nodes_.top());
	}


	Char getNextChar()
	{
		if (current_ == end_)
			return 0;
		return *current_++;
	}


	void getLocationLineAndColumn(Location location,
			int &line,
			int &column) const
	{
		Location current = begin_;
		Location lastLineStart = current;
		line = 0;
		while (current < location  &&  current != end_)
		{
			Char c = *current++;
			if (c == '\r')
			{
				if (*current == '\n')
					++current;
				lastLineStart = current;
				++line;
			}
			else if (c == '\n')
			{
				lastLineStart = current;
				++line;
			}
		}
		// column & line start at 1
		column = int(location - lastLineStart) + 1;
		++line;
	}


	std::string getLocationLineAndColumn(Location location) const
	{
		int line, column;
		getLocationLineAndColumn(location, line, column);
		char buffer[18 + 16 + 16 + 1];
		sprintf(buffer, "Line %d, Column %d", line, column);
		return buffer;
	}


	std::string getFormatedErrorMessages() const
	{
		std::string formattedMessage;
		for (Errors::const_iterator itError = errors_.begin();
			itError != errors_.end();
			++itError)
		{
			const FmtErrorInfo &error = *itError;
			formattedMessage += "* " + getLocationLineAndColumn(error.token_.start_) + "\n";
			formattedMessage += "  " + error.message_ + "\n";
			if (error.extra_)
				formattedMessage += "See " + getLocationLineAndColumn(error.extra_) + " for detail.\n";
		}
		return formattedMessage;
	}

	static inline bool
		in(Char c, Char c1, Char c2, Char c3, Char c4)
	{
		return c == c1 || c == c2 || c == c3 || c == c4;
	}

	static inline bool
		in(Char c, Char c1, Char c2, Char c3, Char c4, Char c5)
	{
		return c == c1 || c == c2 || c == c3 || c == c4 || c == c5;
	}


	static bool
		containsNewLine(Location begin,
			Location end)
	{
		for (; begin < end; ++begin)
			if (*begin == '\n' || *begin == '\r')
				return true;
		return false;
	}

	static std::string codePointToUTF8(unsigned int cp)
	{
		std::string result;

		// based on description from http://en.wikipedia.org/wiki/UTF-8

		if (cp <= 0x7f)
		{
			result.resize(1);
			result[0] = static_cast<char>(cp);
		}
		else if (cp <= 0x7FF)
		{
			result.resize(2);
			result[1] = static_cast<char>(0x80 | (0x3f & cp));
			result[0] = static_cast<char>(0xC0 | (0x1f & (cp >> 6)));
		}
		else if (cp <= 0xFFFF)
		{
			result.resize(3);
			result[2] = static_cast<char>(0x80 | (0x3f & cp));
			result[1] = 0x80 | static_cast<char>((0x3f & (cp >> 6)));
			result[0] = 0xE0 | static_cast<char>((0xf & (cp >> 12)));
		}
		else if (cp <= 0x10FFFF)
		{
			result.resize(4);
			result[3] = static_cast<char>(0x80 | (0x3f & cp));
			result[2] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
			result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
			result[0] = static_cast<char>(0xF0 | (0x7 & (cp >> 18)));
		}

		return result;
	}

private:
	Nodes nodes_;
	Errors errors_;
	std::string document_;
	Location begin_;
	Location end_;
	Location current_;
	Location lastValueEnd_;
	JsonValue *lastValue_;
	std::string commentsBefore_;
	Features features_;
	bool collectComments_;
};

}
} // namespace Json

