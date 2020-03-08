#pragma once
#include "Base/Base.h"
#include "../Defs.h"
using namespace Public::Base;

namespace Public {
namespace Network{
namespace HTTP {

#define Content_Length			"Content-Length"
#define Content_Type			"Content-Type"

#define Transfer_Encoding		"Transfer-Encoding"

#define CHUNKED					"chunked"
#define CONNECTION				"Connection"
#define CONNECTION_Close		"Close"
#define CONNECTION_KeepAlive	"keep-alive"
#define CONNECTION_Upgrade		"Upgrade"

#define HTTPSEPERATOR			"\r\n"

struct HeaderInfo
{
	std::string key;
	std::vector<Value> values;
};

struct NETWORK_API _HeaderS {
	std::list<HeaderInfo> _headers;

	Value& operator[](const std::string& _key);
};

struct NETWORK_API Header
{
	std::string		method;
	std::string		url;
	struct {
		std::string protocol;
		std::string	version;
	}verinfo;

	int				statuscode;
	std::string		statusmsg;

	_HeaderS		headers;

	uint32_t headerSize(const std::string& _key);
	Value header(const std::string& _key, uint32_t index = 0);
	//放入数据，会覆盖之前相同的数据
	void push(const std::string& _key, const Value& val);
	void remove(const std::string& _key);
	//添加头数据，运行重复数据
	void add(const std::string& _key, const Value& val);

	Header();
};

class NETWORK_API Parser
{
public:
	Parser(bool _isRequest);
	~Parser();

	shared_ptr<Header> parse(CircleBuffer& buffer, std::string* usedstr = NULL);
	shared_ptr<Header> parse(const char* data, uint32_t datalen, uint32_t& useddata);

	bool isFindFirstLine();
private:
	struct ParserInternal;
	ParserInternal* internal;
};

class NETWORK_API Builder
{
public:
	static std::string build(bool isRequest, const Header& headertmp);
};

}
}
}