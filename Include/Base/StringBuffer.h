//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//
//	Description:
//	$Id: System.h 3 2013-01-21 06:57:38Z  $


#ifndef __BASE_SYSTEM_H__
#define __BASE_SYSTEM_H__

#include "Defs.h"
#include "String.h"

namespace Public {
namespace Base {

//不连续数据buffer
class BASE_API StringBuffer
{
public:
	//不拷贝的构造体,等同于push_back
	StringBuffer(const char* str);
	StringBuffer(const char* str, uint32_t len);
	StringBuffer(const std::string& str);
	StringBuffer(const String& str);

	StringBuffer();
	StringBuffer(const StringBuffer& buffer);
	~StringBuffer();

	uint32_t length() const;

	//append相关接口会执行拷贝动作
	
	//在前面追加数据
	StringBuffer& append_front(const char* str);
	StringBuffer& append_front(const char* str,uint32_t len);
	StringBuffer& append_front(const std::string& str);
	StringBuffer& append_front(const String& str);
	StringBuffer& append_front(const StringBuffer& buffer);

	//在后面追加数据
	StringBuffer& append_back(const char* str);
	StringBuffer& append_back(const char* str, uint32_t len);
	StringBuffer& append_back(const std::string& str);
	StringBuffer& append_back(const String& str);
	StringBuffer& append_back(const StringBuffer& buffer);

	//push 相关的接口不会拷贝数据

	//在前面放入数据
	StringBuffer& push_front(const char* str);
	StringBuffer& push_front(const char* str, uint32_t len);
	StringBuffer& push_front(const std::string& str);
	StringBuffer& push_front(const String& str);
	StringBuffer& push_front(const StringBuffer& buffer);

	//在后面追加数据
	StringBuffer& push_back(const char* str);
	StringBuffer& push_back(const char* str, uint32_t len);
	StringBuffer& push_back(const std::string& str);
	StringBuffer& push_back(const String& str);
	StringBuffer& push_back(const StringBuffer& buffer);



	//读数据
	//返回实际读取长度
	uint32_t read(uint32_t offset, char* buffer, uint32_t bufferlen) const;
	std::string read(uint32_t offset = 0, uint32_t readlen = -1) const;
	StringBuffer readBuffer(uint32_t offset = 0, uint32_t readlen = -1) const;
private:
	struct StringBufferInternal;
	StringBufferInternal* internal;
};


} // namespace Base
} // namespace Public

#endif //__BASE_SYSTEM_H__


