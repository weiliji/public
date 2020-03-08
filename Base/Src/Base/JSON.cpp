#include "Base/JSON.h"
#include "Base/Shared_ptr.h"
#include "Base/BaseTemplate.h"
#include "Base/File.h"
#include "JSONWriter.h"
#include "JSONReader.h"
namespace Public{
namespace Base {

struct JsonValueData
{
	JsonValue::ValueType								type = JsonValue::nullValue;
	Value												value;
	std::vector<JsonValue>								array;
	std::map<std::string, JsonValue>					object;
	std::map<JsonValue::CommentPlacement, std::string>	comments;
};

static JsonValue emptyValue;

struct JsonValue::JsonValueInternal
{
	shared_ptr<JsonValueData> data;
};

JsonValue::JsonValue(ValueType type)
{
	internal = new JsonValueInternal;
	internal->data = make_shared<JsonValueData>();
	internal->data->type = type;
}
JsonValue::JsonValue(const JsonValue& other)
{
	internal = new JsonValueInternal;
	internal->data = other.internal->data;
}
JsonValue::JsonValue(const Value& val)
{
	internal = new JsonValueInternal;
	internal->data = make_shared<JsonValueData>();
	internal->data->value = val;
	switch (val.type())
	{
	case Value::Type_Int32:
	case Value::Type_Uint32:
		internal->data->type = JsonValue::intValue;
		break;
	case Value::Type_Double:
		internal->data->type = JsonValue::realValue;
		break;
	case Value::Type_Bool:
		internal->data->type = JsonValue::booleanValue;
		break;
	default:
		internal->data->type = JsonValue::stringValue;
		break;
	}
}
JsonValue::JsonValue(const std::vector<JsonValue>& val)
{
	internal = new JsonValueInternal;
	internal->data = make_shared<JsonValueData>();
	internal->data->array = val;
	internal->data->type = JsonValue::arrayValue;
}
JsonValue::JsonValue(const std::map<std::string, JsonValue>& val)
{
	internal = new JsonValueInternal;
	internal->data = make_shared<JsonValueData>();
	internal->data->object = val;
	internal->data->type = JsonValue::objectValue;
}
JsonValue::~JsonValue()
{
	SAFE_DELETE(internal);
}

JsonValue& JsonValue::operator=(const JsonValue& other)
{
	internal->data = other.internal->data;
	return *this;
}

JsonValue::ValueType JsonValue::type() const{return internal->data->type;}
JsonValue::operator ValueType() const { return type(); }

bool JsonValue::operator <(const JsonValue& other) const
{
	return compare(other) < 0;
}
bool JsonValue::operator <=(const JsonValue& other) const
{
	return compare(other) <= 0;
}
bool JsonValue::operator >=(const JsonValue& other) const
{
	return compare(other) >= 0;
}
bool JsonValue::operator >(const JsonValue& other) const
{
	return compare(other) > 0;
}

bool JsonValue::operator ==(const JsonValue& other) const
{
	return compare(other) == 0;
}
bool JsonValue::operator != (const JsonValue& other) const
{
	return compare(other) != 0;
}
bool JsonValue::operator!() const { return isNull(); }

	// < return -1, > return 1,== return 0
int JsonValue::compare(const JsonValue& other) const
{
	if (internal->data == other.internal->data) return 0;

	int typeDelta = internal->data->type - other.internal->data->type;
	if (typeDelta != 0) return typeDelta;


	switch (internal->data->type)
	{
	case JsonValue::intValue:
	case JsonValue::booleanValue:
	case JsonValue::realValue:
	case JsonValue::stringValue:
	{
		return strcmp(internal->data->value.readString().c_str(), other.internal->data->value.readString().c_str());
	}
	case JsonValue::arrayValue:
	{
		size_t arrayDelta = internal->data->array.size() - other.internal->data->array.size();
		if (arrayDelta != 0) return (int)arrayDelta;

		for (size_t i = 0; i < internal->data->array.size(); i++)
		{
			int comval = internal->data->array[i].compare(other.internal->data->array[i]);
			if (comval != 0) return comval;
		}
		return 0;
	}
	case JsonValue::objectValue:
	{
		size_t objDelta = internal->data->object.size() - other.internal->data->object.size();
		if (objDelta != 0) return (int)objDelta;

		std::map<std::string, JsonValue>::const_iterator iter1 = internal->data->object.begin();
		std::map<std::string, JsonValue>::const_iterator iter2 = other.internal->data->object.begin();

		for (; iter1 != internal->data->object.end() && iter2 != other.internal->data->object.end(); iter1++,iter2++)
		{
			int comval = strcmp(iter1->first.c_str(),iter2->first.c_str());
			if (comval != 0) return comval;
			comval = iter1->second.compare(iter2->second);
			if (comval != 0) return comval;
		}
		return 0;
	}
	default:
		break;
	}
	return 0;
}

bool JsonValue::isNull() const {return type() == JsonValue::nullValue; }
bool JsonValue::isBool() const { return type() == JsonValue::booleanValue; }
bool JsonValue::isInt() const { return type() == JsonValue::intValue; }
bool JsonValue::isIntegral() const { return type() == JsonValue::intValue; }
bool JsonValue::isDouble() const { return type() == JsonValue::realValue; }
bool JsonValue::isNumeric() const { return isInt() || isIntegral() || isDouble(); }
bool JsonValue::isString() const {return type() == JsonValue::stringValue;}
bool JsonValue::isArray() const { return type() == JsonValue::arrayValue; }
bool JsonValue::isObject() const { return type() == JsonValue::objectValue; }

	//以下接口为value不为array和object时有效
Value* JsonValue::operator->() { return &internal->data->value; }
const Value* JsonValue::operator->()const { return &internal->data->value; }

Value& JsonValue::operator*() { return internal->data->value; }
const Value& JsonValue::operator*()const { return internal->data->value; }

JsonValue::operator Value() { return internal->data->value; }

Value& JsonValue::data() { return internal->data->value; }
const Value& JsonValue::data() const { return internal->data->value; }


	//以下接口为数组专用

size_t JsonValue::size() const 
{
	if (type() != JsonValue::arrayValue) return 0;

	return internal->data->array.size();
}
bool JsonValue::empty() const
{
	return size() == 0;
}
	
void JsonValue::clear()
{
	internal->data->array.clear();
}
void JsonValue::resize(size_t size)
{
	if (type() != JsonValue::arrayValue) return;

	internal->data->array.resize(size);
}

JsonValue& JsonValue::operator[](size_t index)
{
	if (index < 0 || index >= size()) return emptyValue;

	return internal->data->array[index];
}
const JsonValue& JsonValue::operator[](size_t index) const
{
	if (index < 0 || index >= size()) return emptyValue;

	return internal->data->array[index];
}
const JsonValue& JsonValue::get(size_t index, const JsonValue& defaultValue) const
{
	if (index < 0 || index >= size()) return defaultValue;

	return internal->data->array[index];
}
bool JsonValue::isValidIndex(size_t index) const
{
	if (index < 0 || index >= size()) return false;
	return true;
}
JsonValue& JsonValue::append(const JsonValue& value)
{
	if (type() != JsonValue::arrayValue) return emptyValue;

	internal->data->array.push_back(value);
	return internal->data->array.back();
}

std::vector<JsonValue>& JsonValue::array() { return internal->data->array; }
const std::vector<JsonValue>& JsonValue::array()const {return internal->data->array;}


	//以下接口专为对象使用
JsonValue& JsonValue::operator[](const std::string& key)
{
	if (type() != JsonValue::objectValue || key.length() <= 0) return emptyValue;

	std::map<std::string, JsonValue>::iterator iter = internal->data->object.find(key);
	if (iter == internal->data->object.end())
	{
		internal->data->object[key] = JsonValue();
		iter = internal->data->object.find(key);
	}

	return iter->second;
}
JsonValue& JsonValue::operator[](const char* key)
{
	if (key == NULL || strlen(key) <= 0) return emptyValue;
	if (type() != JsonValue::objectValue) return emptyValue;

	std::map<std::string, JsonValue>::iterator iter = internal->data->object.find(key);
	if (iter == internal->data->object.end())
	{
		internal->data->object[key] = JsonValue();
		iter = internal->data->object.find(key);
	}

	return iter->second;
}
const JsonValue& JsonValue::operator[](const std::string& key) const
{
	if (type() != JsonValue::objectValue || key.length() <= 0) return emptyValue;

	std::map<std::string, JsonValue>::const_iterator iter = internal->data->object.find(key);
	if (iter == internal->data->object.end()) return emptyValue;

	return iter->second;
}
const JsonValue& JsonValue::operator[](const char* key) const
{
	if (key == NULL || strlen(key) <= 0) return emptyValue;
	if (type() != JsonValue::objectValue) return emptyValue;

	std::map<std::string, JsonValue>::const_iterator iter = internal->data->object.find(key);
	if (iter == internal->data->object.end()) return emptyValue;

	return iter->second;
}
JsonValue JsonValue::get(const std::string& key, const JsonValue& defaultValue) const
{
	if (type() != JsonValue::objectValue) return defaultValue;

	std::map<std::string, JsonValue>::const_iterator iter = internal->data->object.find(key);
	if (iter == internal->data->object.end()) return defaultValue;

	return iter->second;
}
JsonValue JsonValue::removeMember(const std::string& key)
{
	JsonValue removeval;

	std::map<std::string, JsonValue>::iterator iter = internal->data->object.find(key);
	if (iter == internal->data->object.end()) return removeval;

	removeval = iter->second;
	internal->data->object.erase(iter);

	return removeval;
}
bool JsonValue::isMember(const std::string& key) const
{
	if (type() != JsonValue::objectValue) return false;

	std::map<std::string, JsonValue>::const_iterator iter = internal->data->object.find(key);
	return iter != internal->data->object.end();
}
std::map<std::string, JsonValue>& JsonValue::object() { return internal->data->object; }
const  std::map<std::string, JsonValue>& JsonValue::object()const { return internal->data->object; }

//一下为注释
void JsonValue::setComment(const std::string &comment, CommentPlacement placement)
{
	internal->data->comments[placement] = comment;
}
bool JsonValue::hasComment(CommentPlacement placement) const
{
	return internal->data->comments.find(placement) != internal->data->comments.end();
}
std::string JsonValue::getComment(CommentPlacement placement) const
{
	std::map<JsonValue::CommentPlacement, std::string>::const_iterator iter = internal->data->comments.find(placement);
	if (iter == internal->data->comments.end()) return "";

	return iter->second;
}
const std::map<JsonValue::CommentPlacement, std::string> JsonValue::comments() const
{
	return internal->data->comments;
}

std::string JsonValue::write(WriteStyle style) const
{
	if (style == WriteStyle_Normal) return JsonStyleWriter(*this).document_;
	else return JsonFastWriter(*this).document_;
}
std::string JsonValue::toStyledString() const
{
	return JsonStyleWriter(*this).document_;
}
ErrorInfo JsonValue::readFile(const std::string& filename)
{
	return read(File::load(filename));
}
ErrorInfo JsonValue::read(const std::string& stream)
{
	return JsonReader().parse(stream.c_str(),(int)stream.length(), *this, false);
}
ErrorInfo JsonValue::read(const char* bufferaddr, int buferlen)
{
	if (bufferaddr == NULL || buferlen < 0) return ErrorInfo(Error_Code_Param);

	return JsonReader().parse(bufferaddr, buferlen, *this, false);
}


}
} // namespace Json

