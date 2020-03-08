//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: network.h 216 2015-12-15 11:33:55Z  $
//
#ifndef __PublicNETWORK_H__
#define __PublicNETWORK_H__
#include "Network/Defs.h"

#include "Network/Socket/TcpClient.h"
#include "Network/Socket/TcpServer.h"
#include "Network/Socket/Udp.h"
#include "Network/Socket/RawSocket.h"
#include "Network/Socket/SSLSocket.h"

#include "Network/Email/Client.h"

#include "Network/HTTP/Client.h"
#include "Network/HTTP/Server.h"
#include "Network/HTTP/Serialization.h"

#include "Network/ntp/ntp.h"

namespace Public{
namespace Network{

class NETWORK_API NetworkSystem
{
public:
	/// 打印 Base库 版本信息
	static void  printVersion();

	//初始化网络库
	//当maxcpuCorePerTherad ！= 0 NETWORK线程数使用 maxcpuCorePerTherad*CPUCore 
	//当maxcpuCorePerTherad == 0  ，threadNum ！= 0 Network使用线程数 threadNum
	//当maxcpuCorePerTherad == 0  ，threadNum == 0 时，network使用线程数默认为1
	static void init();

	//反初始化网络库
	static void uninit();
};

}
}



#endif //__PublicNETWORK_H__
