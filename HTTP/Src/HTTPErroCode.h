#pragma once
#include "Base/Base.h"
using namespace Public::Base;

namespace Public {
namespace HTTP {

class HTTPErrorCode
{
	struct ErrorMsgInfo
	{
		int			code;
		const char* msg;
	};
public:
	static const char* getErrorMsg(int code)
	{
		static ErrorMsgInfo msginfo[] = {
			{101 ,"Switching Protocols"},
			{200 ,"OK"},
			{201 ,"Created"},
			{202 ,"Accepted"},
			{203 ,"Non-Authoritative Information (for DNS)"},
			{204 ,"No Content"},
			{205 ,"Reset Content"},
			{206 ,"Partial Content"},
			{300 ,"Multiple Choices"},
			{301 ,"Moved Permanently"},
			{302 ,"Moved Temporarily"},
			{303 ,"See Other"},
			{304 ,"Not Modified"},
			{305 ,"Use Proxy"},
			{307 ,"Redirect Keep Verb"},
			{400 ,"Bad Request"},
			{401 ,"Unauthorized"},
			{402 ,"Payment Required"},
			{403 ,"Forbidden"},
			{404 ,"Not Found"},
			{405 ,"Bad Request"},
			{406 ,"Not Acceptable"},
			{407 ,"Proxy Authentication Required"},
			{408 ,"Request Timed-Out"},
			{409 ,"Conflict"},
			{410 ,"Gone"},
			{411 ,"Length Required"},
			{412 ,"Precondition Failed"},
			{413 ,"Request Entity Too Large"},
			{414 ,"Request, URI Too Large"},
			{415 ,"Unsupported Media Type"},
			{500 ,"Internal Server Error"},
			{501 ,"Not Implemented"},
			{502 ,"Bad Gateway"},
			{503 ,"Server Unavailable"},
			{504 ,"Gateway Timed-Out"},
			{505 ,"HTTP Version not supported"},
		};

		for (size_t i = 0; i < sizeof(msginfo) / sizeof(ErrorMsgInfo); i++)
		{
			if (code == msginfo[i].code) return msginfo[i].msg;
		}

		return "";
	}
};

}
}

