//
//  Copyright (c)1998-2012, Public Technology
//  All Rights Reserved.
//
//
//	Description:
//	$Id: System.h 3 2013-01-21 06:57:38Z  $


#include "Base/StringBuffer.h"
#include "Base/BaseTemplate.h"

namespace Public {
namespace Base {

struct BufferInfo
{
	String data;
	const char* buffer;
	uint32_t	bufferlen;
};

struct StringBuffer::StringBufferInternal
{
	std::list<BufferInfo> bufferlist;
	uint32_t length;

	StringBufferInternal():length(0){}
};
StringBuffer::StringBuffer(const char* str)
{
	internal = new StringBufferInternal();

	push_back(str);
}
StringBuffer::StringBuffer(const char* str, uint32_t len)
{
	internal = new StringBufferInternal();

	push_back(str, len);
}
StringBuffer::StringBuffer(const std::string& str)
{
	internal = new StringBufferInternal();

	push_back(str);
}
StringBuffer::StringBuffer(const String& str)
{
	internal = new StringBufferInternal();

	push_back(str);
}

StringBuffer::StringBuffer()
{
	internal = new StringBufferInternal();
}
StringBuffer::StringBuffer(const StringBuffer& buffer)
{
	internal = new StringBufferInternal();
	internal->bufferlist = buffer.internal->bufferlist;
	internal->length = buffer.internal->length;
}
StringBuffer::~StringBuffer()
{
	SAFE_DELETE(internal);
}

uint32_t StringBuffer::length() const
{
	return internal->length;
}

StringBuffer& StringBuffer::append_front(const char* str)
{
	return append_front(String(str));
}
StringBuffer& StringBuffer::append_front(const char* str, uint32_t len)
{
	return append_front(String(str,len));
}
StringBuffer& StringBuffer::append_front(const std::string& str)
{
	return append_front(String(str));
}
StringBuffer& StringBuffer::append_front(const String& str)
{
	BufferInfo info;
	info.data = str;
	info.buffer = info.data.c_str();
	info.bufferlen = (uint32_t)info.data.length();

	internal->bufferlist.push_front(info);
	internal->length += info.bufferlen;

	return *this;
}
StringBuffer& StringBuffer::append_front(const StringBuffer& buffer)
{
	for (std::list<BufferInfo>::reverse_iterator  iter = buffer.internal->bufferlist.rbegin(); iter != buffer.internal->bufferlist.rend(); iter++)
	{
		internal->bufferlist.push_front(*iter);
		internal->length += iter->bufferlen;
	}

	return *this;
}

//在后面追加数据
StringBuffer& StringBuffer::append_back(const char* str)
{
	return append_back(String(str));
}
StringBuffer& StringBuffer::append_back(const char* str, uint32_t len)
{
	return append_back(String(str,len));
}
StringBuffer& StringBuffer::append_back(const std::string& str)
{
	return append_back(String(str));
}
StringBuffer& StringBuffer::append_back(const String& str)
{
	BufferInfo info;
	info.data = str;
	info.buffer = info.data.c_str();
	info.bufferlen = (uint32_t)info.data.length();

	internal->bufferlist.push_back(info);
	internal->length += info.bufferlen;

	return *this;
}
StringBuffer& StringBuffer::append_back(const StringBuffer& buffer)
{
	for (std::list<BufferInfo>::iterator iter = buffer.internal->bufferlist.begin(); iter != buffer.internal->bufferlist.end(); iter++)
	{
		internal->bufferlist.push_back(*iter);
		internal->length += iter->bufferlen;
	}

	return *this;
}

//push 相关的接口不会拷贝数据

//在前面放入数据
StringBuffer& StringBuffer::push_front(const char* str)
{
	return push_front(str, (uint32_t)strlen(str));
}
StringBuffer& StringBuffer::push_front(const char* str, uint32_t len)
{
	BufferInfo info;
	info.buffer = str;
	info.bufferlen = len;

	internal->bufferlist.push_front(info);
	internal->length += info.bufferlen;

	return *this;
}
StringBuffer& StringBuffer::push_front(const std::string& str)
{
	return push_front(String(str));
}
StringBuffer& StringBuffer::push_front(const String& str)
{
	BufferInfo info;
	info.data = str;
	info.buffer = info.data.c_str();
	info.bufferlen = (uint32_t)info.data.length();

	internal->bufferlist.push_front(info);
	internal->length += info.bufferlen;

	return *this;
}
StringBuffer& StringBuffer::push_front(const StringBuffer& buffer)
{
	for (std::list<BufferInfo>::reverse_iterator iter = buffer.internal->bufferlist.rbegin(); iter != buffer.internal->bufferlist.rend(); iter++)
	{
		internal->bufferlist.push_front(*iter);
		internal->length += iter->bufferlen;
	}

	return *this;
}

//在后面追加数据
StringBuffer& StringBuffer::push_back(const char* str)
{
	return push_back(str, (uint32_t)strlen(str));
}
StringBuffer& StringBuffer::push_back(const char* str, uint32_t len)
{
	BufferInfo info;
	info.buffer = str;
	info.bufferlen = len;

	internal->bufferlist.push_back(info);
	internal->length += info.bufferlen;

	return *this;
}
StringBuffer& StringBuffer::push_back(const std::string& str)
{
	return push_back(String(str));
}
StringBuffer& StringBuffer::push_back(const String& str)
{
	BufferInfo info;
	info.data = str;
	info.buffer = info.data.c_str();
	info.bufferlen = (uint32_t)info.data.length();

	internal->bufferlist.push_back(info);
	internal->length += info.bufferlen;

	return *this;
}
StringBuffer& StringBuffer::push_back(const StringBuffer& buffer)
{
	for (std::list<BufferInfo>::iterator iter = buffer.internal->bufferlist.begin(); iter != buffer.internal->bufferlist.end(); iter++)
	{
		internal->bufferlist.push_back(*iter);
		internal->length += iter->bufferlen;
	}

	return *this;
}



//读数据
//返回实际读取长度
uint32_t StringBuffer::read(uint32_t offset, char* buffer, uint32_t bufferlen) const
{
	uint32_t nowoffset = 0;
	uint32_t nowreadlen = 0;
	for (std::list<BufferInfo>::iterator iter = internal->bufferlist.begin(); iter != internal->bufferlist.end() && bufferlen > 0; iter++)
	{
		uint32_t needoffset = min(iter->bufferlen, offset - nowoffset);
		uint32_t canreadlen = min(iter->bufferlen - needoffset,bufferlen);
		if (canreadlen > 0)
		{
			memcpy(buffer, iter->buffer + needoffset, canreadlen);

			buffer += canreadlen;
			bufferlen -= canreadlen;
			nowreadlen += canreadlen;
		}

		nowoffset += needoffset;
	}
	
	return nowreadlen;
}
StringBuffer StringBuffer::readBuffer(uint32_t offset, uint32_t readlen) const
{
	if (readlen == (uint32_t)-1) readlen = internal->length - offset;

	uint32_t nowoffset = 0;
	
	StringBuffer buffer;

	for (std::list<BufferInfo>::iterator iter = internal->bufferlist.begin(); iter != internal->bufferlist.end() && readlen > 0; iter++)
	{
		uint32_t needoffset = min(iter->bufferlen, offset - nowoffset);
		uint32_t canreadlen = min(iter->bufferlen - needoffset, readlen);
		if (canreadlen > 0)
		{
			BufferInfo info = *iter;
			info.buffer = iter->buffer + needoffset;
			info.bufferlen = canreadlen;

			buffer.internal->bufferlist.push_back(info);
			buffer.internal->length += canreadlen;

			readlen -= canreadlen;
		}

		nowoffset += needoffset;
	}

	return buffer;
}
std::string StringBuffer::read(uint32_t offset, uint32_t readlen) const
{
	uint32_t needbufferlen = (readlen == (uint32_t)-1) ? internal->length : readlen;
	char* buffer = new char[needbufferlen + 100];

	uint32_t realreadlen = read(offset, buffer, needbufferlen);

	std::string bufferstr(buffer, realreadlen);

	delete[] buffer;

	return bufferstr;
}


} // namespace Base
} // namespace Public


