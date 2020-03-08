//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: tcpclient.h 216 2015-12-15 11:33:55Z  $
//
#ifndef __NETWORK_EMAILCLIENT_H__
#define __NETWORK_EMAILCLIENT_H__

#include "Network/Socket/Socket.h"
#include "Address.h"
#include "Message.h"

namespace Public{
namespace Network{
namespace Email{


class NETWORK_API Client
{
public:
	enum ProtocolType
	{
		ProtocolType_IMAP = 0,	//收邮件，搜索，文件夹等
		ProtocolType_SMTP,		//发送邮件
		ProtocolType_POP3,		//收邮件

		ProtocolType_IMAP_SSL,	//收邮件，搜索，文件夹等
		ProtocolType_SMTP_SSL,	//发送邮件
		ProtocolType_POP3_SSL,	//收邮件
	};
public:
	Client(const shared_ptr<IOWorker>& worker, const std::string& emailserveraddr, uint32_t emailserverport, ProtocolType type = ProtocolType_SMTP);
	virtual ~Client();

	//登录
	virtual ErrorInfo login(const std::string& email, const std::string& password, uint32_t timeout = 5000);
	virtual void loginout();

	//发送消息
	virtual ErrorInfo submit(const shared_ptr<Message>& msg);
	
	//接受消息
	virtual ErrorInfo fetch(uint32_t index, shared_ptr<Message>& msg);
private:
	struct ClientInternal;
	ClientInternal* internal;
};
}
}
}




#endif //__NETWORK_SOCKET_H__

