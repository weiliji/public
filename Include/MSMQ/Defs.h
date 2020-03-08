//
//  Copyright (c)1998-2012, Technology
//  All Rights Reserved.
//
//  Description:
//  $Id: Defs.h 84 2013-11-28 07:12:20Z  $
//


#ifndef __MSMQ_DEFS_H__
#define __MSMQ_DEFS_H__

//MSMQ Milesight Message Queue


// WIN32 Dynamic Link Library
#ifdef _MSC_VER

#ifdef MSMQ_DLL_BUILD
#define  MSMQ_API _declspec(dllexport)
#else
#define MSMQ_API _declspec(dllimport)
#endif

#else

#define MSMQ_API

#endif


#endif //__PROTOCOL_DEFS_H__

