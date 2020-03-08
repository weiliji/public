//
//  Copyright (c)1998-2012, Technology
//  All Rights Reserved.
//
//  Description:
//  $Id: Defs.h 84 2013-11-28 07:12:20Z  $
//


#ifndef __PROTOCOL_DEFS_H__
#define __PROTOCOL_DEFS_H__


// 是否支持IPv6 先关闭
//#define SUPPORT_IPV6


// WIN32 Dynamic Link Library
#ifdef _MSC_VER

#ifdef MSPROTOCOL_DLL_BUILD
#define  MSProtocol_Export _declspec(dllexport)
#else
#define MSProtocol_Export _declspec(dllimport)
#endif

#else

#define MSProtocol_Export

#endif

/// 打印 Protocol 版本信息
#ifdef WIN32
#pragma warning (disable : 4251)  //禁用导出类接口中使用了STL报C4251的警告
#endif

#endif //__PROTOCOL_DEFS_H__

