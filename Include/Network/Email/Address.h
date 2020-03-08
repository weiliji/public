//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: tcpclient.h 216 2015-12-15 11:33:55Z  $
//
#ifndef __NETWORK_Address_H__
#define __NETWORK_Address_H__

#include "Network/Socket/Socket.h"

namespace Public{
namespace Network{
namespace Email{
//ÓÊ¼þµØÖ·
class NETWORK_API Address
{
public:
	Address();
	Address(const std::string& emailaddr,const std::string& emailname = "");
	Address(const Address& eaddr);
	~Address();

	const std::string emailAddr() const;
	const std::string emailName() const;
	bool emtpy() const;
	std::string toString() const;
private:
	struct AddressInternal;
	AddressInternal* internal;
};

}
}
}




#endif //__NETWORK_SOCKET_H__

