#pragma once
#include "redisclient/redisasyncclient.h"
#include "redisclient/redissyncclient.h"
#include "Base/Base.h"
#include "Network/Network.h"


using namespace Public::Base;
using namespace Public::Network;

using namespace redisclient;

namespace Public {
namespace Redis {

struct RedisSyncDbParam
{
	NetAddr						 redisaddr;
	std::string					 password;

	int index;
	shared_ptr<RedisSyncClient>		client;
	shared_ptr<IOWorker>			worker;
};


inline Value transValue(const RedisValue& val)
{
	Value valtmp;
	if (val.isInt()) valtmp = Value(val.toInt());
	else if (val.isString()) valtmp = Value(String::utf82ansi(val.toString()));


	return std::move(valtmp);
}

}
}