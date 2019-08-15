#pragma once

#include "Base/Base.h"
#include "Network/Network.h"
using namespace Public::Base;
using namespace Public::Network;


class RedisValue {
public:
	struct StatusTag {
		StatusTag(){}
		StatusTag(bool succ,const std::string& msg):success(succ),errmsg(msg){}
		
		bool success;
		std::string errmsg;

		inline bool operator == (const StatusTag &t) const {
			return success == t.success && errmsg == t.errmsg;
		}
	};
	RedisValue(){}
	RedisValue(RedisValue &&other): value(std::move(other.value)){}
	RedisValue(int i) : value((int64_t)i) {}
	RedisValue(uint32_t i) : value((int64_t)i) {}
	RedisValue(uint64_t i) : value((int64_t)i) {}
	RedisValue(int64_t i): value(i){}
	RedisValue(const StatusTag& err):value(err){}
	RedisValue(bool success,const std::string& errmsg):value(StatusTag(success,errmsg)){}
	RedisValue(const char *s,int len) : value(std::string(s,len)) {}
	RedisValue(const char *s) : value(std::string(s, strlen(s))) {}
	RedisValue(const std::string &s): value(s){}
	RedisValue(const String &s) : value(s) {}
	RedisValue(const std::vector<RedisValue>& array): value(std::move(array)){}

	RedisValue(const RedisValue &) = default;
	RedisValue& operator = (const RedisValue &) = default;
	RedisValue& operator = (RedisValue &&) = default;

	// Return the value as a std::string if
	// type is a byte string; otherwise returns an empty std::string.
	std::string toString() const
	{
		if (isInt()) return Value(toInt()).readString();

		static const std::string emptstr;
		if (!isString()) return emptstr;

		return getString();
	}

	// Return the value as a std::vector<RedisValue> if
	// type is an int; otherwise returns 0.
	int64_t toInt() const
	{
		if (isString()) return Value(toString()).readInt64();

		return value.get<int64_t>();
	}

	const StatusTag& toStatus()const
	{
		static const StatusTag emtptystatus;

		if (!isStatus()) return emtptystatus;

		return getStatus();
	}

	// Return the value as an array if type is an array;
	// otherwise returns an empty array.
	const std::vector<RedisValue>& toArray() const
	{
		static const std::vector<RedisValue> emtpyarray;

		if (!isArray()) return emtpyarray;

		return getArray();
	}

	// Return true if value not a error
     bool isStatus() const
	 {
		 return typeEq<StatusTag>();
	 }

	// Return true if this is a null.
	bool isNull() const
	{
		return value.empty();
	}
	// Return true if type is an int
	bool isInt() const
	{
		return typeEq<int64_t>();
	}
	// Return true if type is an array
	bool isArray() const
	{
		return typeEq< std::vector<RedisValue> >();
	}
	// Return true if type is a string/byte array. Alias for isByteArray().
	bool isString() const
	{
		return typeEq<String>();
	}

	std::vector<RedisValue> &getArray()
	{
		return value.get<std::vector<RedisValue>>();
	}

	const std::vector<RedisValue> &getArray() const
	{
		return value.get<std::vector<RedisValue>>();
	}

	StatusTag& getStatus()
	{
		return value.get<StatusTag>();
	}

	const StatusTag& getStatus() const
	{
		return value.get<StatusTag>();
	}
	std::string& getString()
	{
		return value.get<std::string>();
	}
	const std::string& getString()const
	{
		return value.get<std::string>();
	}
protected:
	template<typename T>
	bool typeEq() const
	{
		if (value.type() == typeid(T).name())
			return true;
		else
			return false;
	}

private:
	Variant value;
};


typedef Function<void(void*, const RedisValue&)> CmdResultCallback;
