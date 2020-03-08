//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: tcpclient.h 216 2015-12-15 11:33:55Z  $
//
#ifndef __NETWORK_Message_H__
#define __NETWORK_Message_H__

#include "Network/Socket/Socket.h"
#include "Address.h"
namespace Public{
namespace Network{
namespace Email{
	
//邮件消息
class NETWORK_API Message
{
public:
	Message();
	Message(const Message& msg);
	~Message();

	//发送者地址
	Address& from();
	const Address& from() const;

	//目的地地址
	std::vector<Address>& to();
	const std::vector<Address>& to() const;

	//抄送地址
	std::vector<Address>& copyTo();
	const std::vector<Address>& copyTo() const;

	//密送地址 blind carbon copy
	std::vector<Address>& bccTo();
	const std::vector<Address>& bccTo() const;

	//主题
	std::string& subject();
	const std::string& subject() const;

	//附件
	std::vector<std::string>& attachments();
	const std::vector<std::string>& attachments() const;

	//内容
	std::string& content();
	const std::string& content() const;
private:
	struct MessageInternal;
	MessageInternal* internal;
};


};
};
};




#endif //__NETWORK_SOCKET_H__

