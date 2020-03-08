#include "Network/HTTP/Serialization.h"

namespace Public {
namespace Network{
namespace HTTP {


Value& _HeaderS::operator[](const std::string& _key)
{
	std::list<HeaderInfo>::iterator iter;

	for (iter = _headers.begin(); iter != _headers.end(); iter++)
	{
		if (!String::iequals(iter->key, _key)) continue;
		break;
	}

	if (iter == _headers.end())
	{
		HeaderInfo info;
		info.key = _key;
		info.values.push_back(Value());

		_headers.push_back(info);
	}

	for (iter = _headers.begin(); iter != _headers.end(); iter++)
	{
		if (!String::iequals(iter->key, _key)) continue;
		break;
	}

	return *iter->values.begin();
}


uint32_t Header::headerSize(const std::string& _key)
{
	for (std::list<HeaderInfo>::iterator iter = headers._headers.begin(); iter != headers._headers.end(); iter++)
	{
		if (!String::iequals(iter->key, _key)) continue;

		return (uint32_t)iter->values.size();
	}

	return 0;
}
Value Header::header(const std::string& _key, uint32_t index)
{
	for (std::list<HeaderInfo>::iterator iter = headers._headers.begin(); iter != headers._headers.end(); iter++)
	{
		if (!String::iequals(iter->key, _key)) continue;

		if (index < 0 || index >= iter->values.size()) return Value();

		return iter->values[index];
	}
	return Value();
}
//放入数据，会覆盖之前相同的数据
void Header::push(const std::string& _key, const Value& val)
{
	for (std::list<HeaderInfo>::iterator iter = headers._headers.begin(); iter != headers._headers.end(); iter++)
	{
		if (!String::iequals(iter->key, _key)) continue;

		iter->values.clear();
		iter->values.push_back(val);

		return;
	}

	HeaderInfo info;
	info.key = _key;
	info.values.push_back(val);

	headers._headers.push_back(info);
}
void Header::remove(const std::string& _key)
{
	for (std::list<HeaderInfo>::iterator iter = headers._headers.begin(); iter != headers._headers.end(); iter++)
	{
		if (!String::iequals(iter->key, _key)) continue;

		headers._headers.erase(iter);
		break;
	}
}
//添加头数据，运行重复数据
void Header::add(const std::string& _key, const Value& val)
{
	for (std::list<HeaderInfo>::iterator iter = headers._headers.begin(); iter != headers._headers.end(); iter++)
	{
		if (!String::iequals(iter->key, _key)) continue;

		iter->values.push_back(val);

		return;
	}

	HeaderInfo info;
	info.key = _key;
	info.values.push_back(val);

	headers._headers.push_back(info);
}

Header::Header()
{
	statuscode = 200;
	statusmsg = "OK";
}

struct Parser::ParserInternal
{
	bool				isRequest = false;
	shared_ptr<Header>	content;

	bool parseLine(const char* startlineaddr, uint32_t linedatalen)
	{
		//连续两个SEPERATOR表示header结束
		if (linedatalen == 0)
		{
			return true;
		}
		else if (content->verinfo.protocol == "")
		{
			parseFirstLine(startlineaddr, linedatalen);
		}
		else
		{
			parseHeaderLine(startlineaddr, linedatalen);
		}

		return false;
	}
	void parseFirstLine(const char* startlienaddr, uint32_t linedatalen)
	{
		std::string versionstr;
		if (isRequest)
		{
			std::vector<std::string> tmp = String::split(startlienaddr, linedatalen, " ");
			if (tmp.size() != 3) return;

			content->method = String::trim_copy(tmp[0]);
			content->url = String::trim_copy(tmp[1]);
			versionstr = String::trim_copy(tmp[2]);
		}
		else
		{
			std::vector<std::string> tmp = String::split(startlienaddr, linedatalen, " ");
			if (tmp.size() < 2) return;

			versionstr = String::trim_copy(tmp[0]);
			content->statuscode = Value(tmp[1]).readInt();

			std::string errstr;
			for (uint32_t i = 2; i < tmp.size(); i++)
			{
				errstr += (i == 2 ? "" : " ") + String::trim_copy(tmp[i]);
			}
			content->statusmsg = errstr;
		}

		if (versionstr != "")
		{
			std::vector<std::string> tmp = String::split(versionstr, "/");
			if (tmp.size() >= 1) content->verinfo.protocol = String::trim_copy(tmp[0]);
			if (tmp.size() >= 2) content->verinfo.version = String::trim_copy(tmp[1]);
		}
	}
	void parseHeaderLine(const char* startlienaddr, uint32_t linedatalen)
	{
		std::vector<std::string> tmp = String::split(startlienaddr, linedatalen, ":");
		if (tmp.size() >= 1)
		{
			std::string strtmp;
			for (uint32_t i = 1; i < tmp.size(); i++)
			{
				strtmp += (i == 1 ? "" : "/") + tmp[i];
			}

			content->add(String::trim_copy(tmp[0]), String::trim_copy(strtmp));
		}
	}
};


Parser::Parser(bool _isRequest)
{
	internal = new ParserInternal;
	internal->isRequest = _isRequest;
}
Parser::~Parser() 
{
	SAFE_DELETE(internal);
}

shared_ptr<Header> Parser::parse(CircleBuffer& buffer, std::string* usedstr)
{
	if (internal->content == NULL) internal->content = make_shared<Header>();

	bool contentIsOk = false;
	while (!contentIsOk)
	{
		std::string linestr;
		if (!buffer.consumeLine(linestr)) break;

		if (usedstr != NULL)
		{
			*usedstr += linestr + "\r\n";
		}

		contentIsOk = internal->parseLine(linestr.c_str(), (uint32_t)linestr.length());
	}

	shared_ptr<Header> contenttmp = internal->content;
	if (contenttmp != NULL && contentIsOk)
	{
		internal->content = NULL;
		return contenttmp;
	}

	return shared_ptr<Header>();
}
shared_ptr<Header> Parser::parse(const char* data, uint32_t datalen, uint32_t& useddata)
{
	if (internal->content == NULL) internal->content = make_shared<Header>();

	useddata = 0;
	bool contentIsOk = false;
	while (!contentIsOk)
	{
		size_t pos = String::indexOf(data, datalen, HTTPSEPERATOR);
		if (pos == (size_t)-1) break;

		contentIsOk = internal->parseLine(data, (uint32_t)pos);

		{
			uint32_t poslen = (uint32_t)pos + (uint32_t)strlen(HTTPSEPERATOR);

			data += poslen;
			datalen -= poslen;
			useddata += poslen;
		}
	}
	shared_ptr<Header> contenttmp = internal->content;
	if (contenttmp != NULL && contentIsOk)
	{
		internal->content = NULL;
		return contenttmp;
	}

	return shared_ptr<Header>();
}

bool Parser::isFindFirstLine()
{
	if (internal->content == NULL || internal->content->verinfo.protocol == "") return false;

	return true;
}

std::string Builder::build(bool isRequest, const Header& headertmp)
{
	Header& header = (Header&)headertmp;

	URL url(header.url);
	{
		header.verinfo.protocol = url.protocol;
		if (header.verinfo.protocol.length() == 0) header.verinfo.protocol = "HTTP";

		if (String::iequals(header.verinfo.protocol, "http")) header.verinfo.version = "1.1";
		else if (String::iequals(header.verinfo.protocol, "rtsp")) header.verinfo.version = "1.0";
	}
	std::string cmdstr;

	if (isRequest)
	{
		std::string requrl = header.url;
		if (String::iequals(header.verinfo.protocol, "http"))
		{
			requrl = url.getPath();
		}

		cmdstr = String::toupper(header.method) + " " + requrl + " " + String::toupper(header.verinfo.protocol) + "/" + header.verinfo.version + HTTPSEPERATOR;
	}
	else
	{
		cmdstr = String::toupper(header.verinfo.protocol) + "/" + header.verinfo.version + " " + Value(header.statuscode).readString() + " " + (header.statuscode == 200 ? "OK" : header.statusmsg) + HTTPSEPERATOR;
	}
	for (std::list<HeaderInfo>::const_iterator iter = header.headers._headers.begin(); iter != header.headers._headers.end(); iter++)
	{
		for (uint32_t i = 0; i < iter->values.size(); i++)
		{
			if (iter->values[i].empty()) continue;

			cmdstr += iter->key + ": " + iter->values[i].readString() + HTTPSEPERATOR;
		}
	}
	cmdstr += HTTPSEPERATOR;

	return cmdstr;
}

}
}
}