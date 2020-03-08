#include "Base/IntTypes.h"
#include "Base/Value.h"
#include "Base/String.h"
#include "Base/BaseTemplate.h"
#include <sstream>

namespace Public
{
namespace Base
{

struct ValueInfo
{
	Value::Type type;
	union {
		uint64_t _int;
		double _float;
		bool _bool;
		char *_str;
	} val;
	void *ptr;
	size_t len;

	ValueInfo() : type(Value::Type_Empty), ptr(NULL),len(0) { val._int = 0; }
	~ValueInfo()
	{
		clear();
	}
	void clear()
	{
		if (type == Value::Type_String)
			delete[] val._str;

		type = Value::Type_Empty;
		val._int = 0;
	}
};

struct Value::ValueInternal
{
	shared_ptr<ValueInfo> val;

	ValueInternal() { val = make_shared<ValueInfo>(); }

	void reset() { val = make_shared<ValueInfo>(); }
};
Value::Value()
{
	internal = new ValueInternal();
}
Value::Value(const std::string &val, Value::Type type)
{
	internal = new ValueInternal();
	internal->val->type = type;
	switch (type)
	{
	case Type_Char:
		sscanf(val.c_str(), "%c", (char *)&internal->val->val._int);
		break;
	case Type_Int32:
		sscanf(val.c_str(), "%d", (int *)&internal->val->val._int);
		break;
	case Type_Uint32:
		sscanf(val.c_str(), "%u", (unsigned int *)&internal->val->val._int);
		break;
	case Type_Int64:
		sscanf(val.c_str(), "%lld", (long long int *)&internal->val->val._int);
		break;
	case Type_Uint64:
		sscanf(val.c_str(), "%llu", (long long unsigned int *)&internal->val->val._int);
		break;
	case Type_String:
		internal->val->len = val.length();
		internal->val->val._str = new char[val.length() + 1];
		memcpy(internal->val->val._str, val.c_str(), internal->val->len);
		internal->val->val._str[internal->val->len] = 0;
		break;
	case Type_Double:
		sscanf(val.c_str(), "%lf", &internal->val->val._float);
		break;
	case Type_Bool:
		internal->val->val._bool = String::iequals(val, "true");
		break;
	case Type_Pointer:
		sscanf(val.c_str(), "%p", &internal->val->ptr);
		break;
	default:
		break;
	}
}
Value::Value(const char *val)
{
	internal = new ValueInternal();
	if (val != NULL && strlen(val) > 0)
	{
		internal->val->len = strlen(val);
		internal->val->val._str = new char[internal->val->len + 1];
		memcpy(internal->val->val._str, val, internal->val->len);
		internal->val->val._str[internal->val->len] = 0;
		internal->val->type = Type_String;
		internal->val->ptr = (void *)val;
	}
}
Value::Value(const std::string &val)
{
	internal = new ValueInternal();
	if (val.length() > 0)
	{
		internal->val->len = val.length();
		internal->val->val._str = new char[internal->val->len + 1];
		memcpy(internal->val->val._str, val.c_str(), internal->val->len);
		internal->val->val._str[internal->val->len] = 0;
		internal->val->type = Type_String;
	}
}
Value::Value(const String &val)
{
	internal = new ValueInternal();
	if (val.length() > 0)
	{
		internal->val->len = val.length();
		internal->val->val._str = new char[internal->val->len + 1];
		memcpy(internal->val->val._str, val.c_str(), internal->val->len);
		internal->val->val._str[internal->val->len] = 0;
		internal->val->type = Type_String;
	}
}
Value::Value(const std::vector<char> &val)
{
	internal = new ValueInternal();

	if (val.size() > 0)
	{
		internal->val->len = val.size();
		internal->val->val._str = new char[val.size() + 1];

		for (size_t i = 0; i < val.size(); i++)
			internal->val->val._str[i] = val[i];
		internal->val->val._str[val.size()] = 0;

		internal->val->type = Type_String;
	}
}

Value::Value(const unsigned char *val)
{
	internal = new ValueInternal();
	if (val != NULL && strlen((const char *)val) > 0)
	{
		internal->val->len = strlen((const char *)val);
		internal->val->val._str = new char[internal->val->len + 1];
		memcpy(internal->val->val._str, val, internal->val->len);
		internal->val->val._str[internal->val->len] = 0;
		internal->val->type = Type_String;
		internal->val->ptr = (void *)val;
	}
}
Value::Value(char val)
{
	internal = new ValueInternal();
	internal->val->type = Type_Char;
	internal->val->val._int = (uint64_t)val;
}
Value::Value(int val)
{
	internal = new ValueInternal();
	internal->val->type = Type_Int32;
	internal->val->val._int = (uint64_t)val;
}
#ifndef __linux__
Value::Value(long val)
{
	internal = new ValueInternal();
	internal->val->type = Type_Int64;
	internal->val->val._int = (uint64_t)val;
}
Value::Value(unsigned long val)
{
	internal = new ValueInternal();
	internal->val->type = Type_Uint64;
	internal->val->val._int = (uint64_t)val;
}
#endif
Value::Value(double val)
{
	internal = new ValueInternal();
	internal->val->type = Type_Double;
	internal->val->val._float = (double)val;
}
Value::Value(uint32_t val)
{
	internal = new ValueInternal();
	internal->val->type = Type_Uint32;
	internal->val->val._int = (uint64_t)val;
}
Value::Value(uint64_t val)
{
	internal = new ValueInternal();
	internal->val->type = Type_Uint64;
	internal->val->val._int = (uint64_t)val;
}
Value::Value(int64_t val)
{
	internal = new ValueInternal();
	internal->val->type = Type_Int64;
	internal->val->val._int = (uint64_t)val;
}
Value::Value(bool val)
{
	internal = new ValueInternal();
	internal->val->type = Type_Bool;
	internal->val->val._bool = val;
}

Value::Value(const Value &val)
{
	internal = new ValueInternal();
	internal->val = val.internal->val;
}
Value::Value(const void *val)
{
	_alloc(val);
}
Value::~Value()
{
	SAFE_DELETE(internal);
}

Value &Value::operator=(const Value &val)
{
	internal->reset();
	internal->val = val.internal->val;

	return *this;
}
bool Value::operator==(const Value &val) const
{
	if (internal->val == val.internal->val)
		return true;
	else if (type() == val.type() && readString() == val.readString())
		return true;

	return false;
}
Value::Type Value::type() const
{
	return internal->val->type;
}

Value::operator std::string() const { return readString(); }
Value::operator uint64_t() const { return readUint64(); }
Value::operator uint32_t() const { return readUint32(); }
Value::operator int64_t() const { return readInt64(); }
Value::operator int32_t() const { return readInt(); }
Value::operator bool() const { return readBool(); }
Value::operator double() const { return readFloat(); }
Value::operator float() const { return readFloat(); }

std::string Value::readString(const std::string &fmt) const
{
	switch (internal->val->type)
	{
	case Type_Char:
	{
		char buffer[32] = {0};
		snprintf(buffer, 31, fmt == "" ? "%c" : fmt.c_str(), (char)internal->val->val._int);
		return buffer;
	}
	case Type_Int32:
	{
		char buffer[32] = {0};
		snprintf(buffer, 31, fmt == "" ? "%d" : fmt.c_str(), (int)internal->val->val._int);
		return buffer;
	}
	case Type_Uint32:
	{
		char buffer[32] = {0};
		snprintf(buffer, 31, fmt == "" ? "%u" : fmt.c_str(), (int)internal->val->val._int);
		return buffer;
	}
	case Type_Int64:
	{
		char buffer[32] = {0};
		snprintf(buffer, 31, fmt == "" ? "%lld" : fmt.c_str(), (long long int)internal->val->val._int);
		return buffer;
	}
	case Type_Uint64:
	{
		char buffer[32] = {0};
		snprintf(buffer, 31, fmt == "" ? "%llu" : fmt.c_str(), (long long int)internal->val->val._int);
		return buffer;
	}
	case Type_String:
		return std::string(internal->val->val._str, internal->val->len);
	case Type_Double:
	{
		char buffer[32] = {0};
		snprintf(buffer, 31, fmt == "" ? "%lf" : fmt.c_str(), internal->val->val._float);
		return buffer;
	}
	case Type_Bool:
		return internal->val->val._bool ? "true" : "false";
	case Type_Pointer:
	{
		char buffer[32] = {0};
		snprintf(buffer, 31, fmt == "" ? "%p" : fmt.c_str(), internal->val->ptr);
		return buffer;
	}
	default:
		break;
	}

	return "";
}
int Value::readInt(const std::string &fmt) const
{
	switch (internal->val->type)
	{
	case Type_Char:
	case Type_Int32:
	case Type_Uint32:
	case Type_Int64:
	case Type_Uint64:
		return (int)internal->val->val._int;
	case Type_String:
	{
		int val = 0;
		sscanf(internal->val->val._str, fmt == "" ? "%d" : fmt.c_str(), &val);
		return val;
	}
	case Type_Double:
		return (int)internal->val->val._float;
	case Type_Bool:
		return internal->val->val._bool ? 1 : 0;
	case Type_Pointer:
	{
		return (int)(long)internal->val->ptr;
	}
	default:
		break;
	}

	return 0;
}
float Value::readFloat(const std::string &fmt) const
{
	switch (internal->val->type)
	{
	case Type_Char:
	case Type_Int32:
	case Type_Uint32:
	case Type_Int64:
	case Type_Uint64:
		return (float)internal->val->val._int;
	case Type_String:
	{
		float val = 0;
		sscanf(internal->val->val._str, fmt == "" ? "%f" : fmt.c_str(), &val);
		return val;
	}
	case Type_Double:
		return (float)internal->val->val._float;
	case Type_Bool:
		return internal->val->val._bool ? (float)1.0 : (float)0.0;
	case Type_Pointer:
	{
		return (float)(long long)internal->val->ptr;
	}
	default:
		break;
	}

	return (float)0.0;
}

long long Value::readInt64(const std::string &fmt) const
{
	switch (internal->val->type)
	{
	case Type_Char:
	case Type_Int32:
	case Type_Uint32:
	case Type_Int64:
	case Type_Uint64:
		return (long long)internal->val->val._int;
	case Type_String:
	{
		long long val = 0;
		sscanf(internal->val->val._str, fmt == "" ? "%lld" : fmt.c_str(), &val);
		return val;
	}
	case Type_Double:
		return (long long)internal->val->val._float;
	case Type_Bool:
		return internal->val->val._bool ? 1 : 0;
	case Type_Pointer:
	{
		return (long long)internal->val->ptr;
	}
	default:
		break;
	}

	return 0;
}

uint32_t Value::readUint32(const std::string &fmt) const
{
	switch (internal->val->type)
	{
	case Type_Char:
	case Type_Int32:
	case Type_Uint32:
	case Type_Int64:
	case Type_Uint64:
		return (uint32_t)internal->val->val._int;
	case Type_String:
	{
		uint32_t val = 0;
		sscanf(internal->val->val._str, fmt == "" ? "%u" : fmt.c_str(), &val);
		return val;
	}
	case Type_Double:
		return (uint32_t)internal->val->val._float;
	case Type_Bool:
		return internal->val->val._bool ? 1 : 0;
	case Type_Pointer:
	{
		return (uint32_t)(long)internal->val->ptr;
	}
	default:
		break;
	}

	return 0;
}

uint64_t Value::readUint64(const std::string &fmt) const
{
	switch (internal->val->type)
	{
	case Type_Char:
	case Type_Int32:
	case Type_Uint32:
	case Type_Int64:
	case Type_Uint64:
		return (uint64_t)internal->val->val._int;
	case Type_String:
	{
		uint64_t val = 0;
		sscanf(internal->val->val._str, fmt == "" ? "%llu" : fmt.c_str(), (long long unsigned int *)&val);
		return val;
	}
	case Type_Double:
		return (uint64_t)internal->val->val._float;
	case Type_Bool:
		return internal->val->val._bool ? 1 : 0;
	case Type_Pointer:
	{
		return (uint64_t)internal->val->ptr;
	}
	default:
		break;
	}

	return 0;
}
bool Value::readBool() const
{
	switch (internal->val->type)
	{
	case Type_Char:
	case Type_Int32:
	case Type_Uint32:
	case Type_Int64:
	case Type_Uint64:
		return internal->val->val._int != 0;
	case Type_String:
		return String::iequals(internal->val->val._str, "true") || atoi(internal->val->val._str) != 0;
	case Type_Double:
		return (int)internal->val->val._float != 0;
	case Type_Bool:
		return internal->val->val._bool;
	case Type_Pointer:
	{
		return internal->val->ptr != NULL;
	}
	default:
		break;
	}

	return false;
}
const void *Value::readPointer() const
{
	return internal->val->ptr;
}
bool Value::empty() const
{
	return internal->val->type == Type_Empty;
}

void Value::_alloc(const void *val)
{
	internal = new ValueInternal();

	internal->reset();
	internal->val->type = Type_Pointer;
	internal->val->val._int = 0;
	internal->val->ptr = (void *)val;
}

} // namespace Base
} // namespace Public
