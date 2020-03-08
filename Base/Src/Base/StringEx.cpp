//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: String.cpp 227 2013-10-30 01:10:30Z  $
//
#ifdef WIN32
#include <windows.h>
#else
#include <errno.h>
#include <string.h>
#include <iconv.h>
#endif
#include <stdarg.h>
#include <stdio.h>

#include "Base/ByteOrder.h"
#include "Base/String.h"
#include "Base/PrintLog.h"
#include "snprintf_x.h"
#include "unicode.h"

namespace Public
{
namespace Base
{

size_t strncat(char *dest, size_t dstBufLen, const char *src, const size_t srcCopyCount)
{
	size_t len = strlen(dest);
	len = dstBufLen - len - 1 > srcCopyCount ? srcCopyCount : dstBufLen - len - 1;
	::strncat(dest, src, len);
#ifdef WIN32
	dest[dstBufLen - 1] = '\0';
#endif
	return len;
}

size_t strncpy(char *dst, size_t dstBufLen, const char *src, size_t srcCopyCount)
{
	size_t count = dstBufLen - 1 > srcCopyCount ? srcCopyCount : dstBufLen - 1;
	::strncpy(dst, src, count);
	dst[count] = '\0';
	return strlen(dst);
}

std::string String::tolower(const std::string &src)
{
	std::string tmp;
	const char *tmpbuf = src.c_str();
	size_t len = src.length();
	for (size_t i = 0; i < len; i++)
	{
		char tmpc = ::tolower(tmpbuf[i]);
		tmp.push_back(tmpc);
	}

	return (tmp);
}

//×Ö·û´®×ª´óÐ´
std::string String::toupper(const std::string &src)
{
	std::string tmp;
	const char *tmpbuf = src.c_str();
	size_t len = src.length();
	for (size_t i = 0; i < len; i++)
	{
		char tmpc = ::toupper(tmpbuf[i]);
		tmp.push_back(tmpc);
	}

	return (tmp);
}

std::string String::ansi2utf8(const std::string &inString)
{
#if 0
	int len = MultiByteToWideChar(CP_ACP, 0, inString.c_str(), (int)inString.length(), NULL, 0);
		wchar_t *wstr = new wchar_t[len + 1];
		int tmplen = MultiByteToWideChar(CP_ACP, 0, inString.c_str(), (int)inString.length(), wstr, len);
		len = WideCharToMultiByte(CP_UTF8, 0, wstr, tmplen, NULL, 0, NULL, NULL);
		char *buf = new char[len + 1];
		WideCharToMultiByte(CP_UTF8, 0, wstr, tmplen, buf, len, NULL, NULL);
		delete[] wstr;
		std::string tmp(buf, len);
		delete[] buf;
		return (tmp);
#else


	size_t mbcslen = inString.length() + 100;
	wchar_t* mscharbuf = new wchar_t[mbcslen];
	memset(mscharbuf, 0, sizeof(wchar_t)*mbcslen);
	int tolen = unicode_from_gb18030(inString.c_str(), inString.length(), mscharbuf, sizeof(wchar_t)*mbcslen);

	size_t utflen = mbcslen * 4 + 100;
	char* utfbuf = new char[utflen];
	memset(utfbuf, 0, utflen);
	int toutflen = unicode_to_utf8(mscharbuf, tolen, utfbuf, utflen);

	std::string utfstr(utfbuf, toutflen);

	delete[]mscharbuf;
	delete[]utfbuf;

	return utfstr;
#endif
}
std::string String::utf82ansi(const std::string &inString)
{
#if 0
int len = MultiByteToWideChar(CP_UTF8, 0, inString.c_str(), (int)inString.length(), NULL, 0);
		wchar_t *wstr = new wchar_t[len + 1];
		int lentmp = MultiByteToWideChar(CP_UTF8, 0, inString.c_str(), (int)inString.length(), wstr, len);
		len = WideCharToMultiByte(CP_ACP, 0, wstr, lentmp, NULL, 0, NULL, NULL);
		char *buf = new char[len + 1];
		WideCharToMultiByte(CP_ACP, 0, wstr, lentmp, buf, len, NULL, NULL);
		delete[] wstr;
		std::string tmp(buf, len);

		delete[] buf;
		return (tmp);
#else
	size_t uniclen = inString.length() + 100;
	wchar_t* unicchar = new wchar_t[uniclen];
	memset(unicchar, 0, sizeof(wchar_t)*uniclen);
	int touniclen = unicode_from_utf8(inString.c_str(),inString.length(),unicchar, sizeof(wchar_t)*uniclen);

	int gblen = touniclen * 4 + 100;
	char* gbchar = new char[gblen];
	memset(gbchar, 0, gblen);
	int togblen = unicode_to_gb18030(unicchar, touniclen, gbchar, gblen);

	std::string gbstr(gbchar, togblen);

	delete[] unicchar;
	delete[] gbchar;

	return gbstr;
	#endif
}

std::vector<std::string> String::split(const std::string &src, const std::string &howmany)
{
	return (split(src.c_str(), src.length(), howmany.c_str()));
}

const char *mystrstr(const char *src, size_t len, const char *findstr)
{
	const char *tmp = src;
	const char *tmpend = src + len;
	size_t findlend = strlen(findstr);

	while (tmp < tmpend && size_t(tmpend - tmp) >= findlend)
	{
		if (memcmp(tmp, findstr, findlend) == 0)
		{
			return tmp;
		}
		tmp++;
	}

	return NULL;
}

std::vector<std::string> String::split(const char *src, size_t len, const std::string &howmany)
{
	if (src == NULL || len <= 0)
		return std::vector<std::string>();

	std::vector<std::string> values;

	const char *tmp = src;
	const char *tmpend = src + len;

	std::string valuestr;

	while (tmp < tmpend)
	{
		const char *findstr = mystrstr(tmp, tmpend - tmp, howmany.c_str());
		if (findstr == NULL)
		{
			valuestr = std::string(tmp, tmpend - tmp);
		}
		else
		{
			valuestr = std::string(tmp, findstr - tmp);
			tmp = findstr + howmany.length();
		}

		if (valuestr.length() > 0)
			values.push_back(valuestr);
		if (findstr == NULL)
			break;
	}

	return (values);
}

static bool isDefaultCanTrim(char c)
{
	if (c == ' ' || c == '\r' || c == '\n' || c == '\t')
		return true;

	return false;
}

void String::trim(std::string &src)
{
	trim_if(src, isDefaultCanTrim);
}

void String::trim_if(std::string &src, const Function<bool(char)> &iffunc)
{
	ltrim_if(src, iffunc);
	rtrim_if(src, iffunc);
}

std::string String::trim_copy(const std::string &src)
{
	return (trim_copy_if(src, isDefaultCanTrim));
}
std::string String::trim_copy_if(const std::string &src, const Function<bool(char)> &iffunc)
{
	std::string tmp = src;
	trim_if(tmp, iffunc);

	return (tmp);
}

void String::ltrim(std::string &src)
{
	ltrim_if(src, isDefaultCanTrim);
}
void String::ltrim_if(std::string &src, const Function<bool(char)> &iffunc)
{
	Function<bool(char)> iffunctmp = iffunc;

	const char *tmp = src.c_str();
	const char *starttmp = tmp;
	uint32_t tmplen = (uint32_t)src.length();

	{
		while (tmplen > 0)
		{
			if (iffunctmp(*starttmp))
			{
				starttmp++;
				tmplen--;
			}
			else
				break;
		}
	}

	if (starttmp != tmp)
	{
		memcpy((void *)tmp, starttmp, tmplen);
	}

	src.resize(tmplen);
}
std::string String::ltrim_copy(const std::string &src)
{
	std::string tmp = src;
	ltrim_if(tmp, isDefaultCanTrim);

	return (tmp);
}
std::string String::ltrim_copy_if(const std::string &src, const Function<bool(char)> &iffunc)
{
	std::string tmp = src;
	ltrim_if(tmp, iffunc);

	return (tmp);
}

void String::rtrim(std::string &src)
{
	rtrim_if(src, isDefaultCanTrim);
}
void String::rtrim_if(std::string &src, const Function<bool(char)> &iffunc)
{
	Function<bool(char)> iffunctmp = iffunc;

	const char *tmp = src.c_str();
	uint32_t tmplen = (uint32_t)src.length();

	{
		while (tmplen > 0)
		{
			if (iffunctmp(tmp[tmplen - 1]))
			{
				tmplen--;
			}
			else
				break;
		}
	}

	src.resize(tmplen);
}
std::string String::rtrim_copy(const std::string &src)
{
	return (rtrim_copy_if(src, isDefaultCanTrim));
}
std::string String::rtrim_copy_if(const std::string &src, const Function<bool(char)> &iffunc)
{
	std::string tmp = src;
	rtrim_if(tmp, iffunc);

	return (tmp);
}

size_t String::indexOf(const std::string &src, const std::string &fromindex)
{
	return indexOf(src.c_str(), src.length(), fromindex);
}
size_t String::indexOf(const char *src, size_t len, const std::string &fromindex)
{
	const char *tmp = src;
	const char *tmpend = tmp + len;
	const char *fromtmp = fromindex.c_str();
	size_t fromlen = fromindex.length();
	while (size_t(tmpend - tmp) >= fromlen)
	{
		if (strncmp(tmp, fromtmp, fromlen) == 0)
			return tmp - src;
		tmp++;
	}

	return -1;
}
size_t String::indexOfByCase(const std::string &src, const std::string &fromindex)
{
	return indexOfByCase(src.c_str(), src.length(), fromindex);
}
size_t String::indexOfByCase(const char *src, size_t len, const std::string &fromindex)
{
	const char *tmp = src;
	const char *tmpend = tmp + len;
	const char *fromtmp = fromindex.c_str();
	size_t fromlen = fromindex.length();
	while (size_t(tmpend - tmp) >= fromlen)
	{
		if (strncasecmp(tmp, fromtmp, (int)fromlen) == 0)
			return tmp - src;
		tmp++;
	}

	return -1;
}
size_t String::lastIndexOf(const std::string &src, const std::string &fromindex)
{
	return lastIndexOf(src.c_str(), src.length(), fromindex);
}
size_t String::lastIndexOf(const char *src, size_t len, const std::string &fromindex)
{
	const char *tmp = src;
	const char *tmpend = tmp + len;
	const char *fromtmp = fromindex.c_str();
	size_t fromlen = fromindex.length();
	while (size_t(tmpend - tmp) >= fromlen)
	{
		if (strncmp(tmpend - fromlen, fromtmp, fromlen) == 0)
			return tmpend - fromlen - tmp;
		tmpend--;
		;
	}

	return -1;
}
size_t String::lastIndexOfByCase(const std::string &src, const std::string &fromindex)
{
	return lastIndexOfByCase(src.c_str(), src.length(), fromindex);
}
size_t String::lastIndexOfByCase(const char *src, size_t len, const std::string &fromindex)
{
	const char *tmp = src;
	const char *tmpend = tmp + len;
	const char *fromtmp = fromindex.c_str();
	size_t fromlen = fromindex.length();
	while (size_t(tmpend - tmp) >= fromlen)
	{
		if (strncasecmp(tmpend - fromlen, fromtmp, (int)fromlen) == 0)
			return tmpend - tmp;
		tmpend--;
		;
	}

	return -1;
}

std::string String::replace(const std::string &src, const std::string &substr, const std::string &replacement)
{
	const char *srctmp = src.c_str();
	std::string strtmp;

	while (1)
	{
		const char *tmp = strstr(srctmp, substr.c_str());
		if (tmp == NULL)
		{
			strtmp += srctmp;
			break;
		}
		strtmp += std::string(srctmp, tmp - srctmp);
		strtmp += replacement;
		srctmp = tmp + substr.length();
	}

	return (strtmp);
}

void String::_snprintf_x(char *buf, int maxsize, const char *fmt, const std::vector<Value> &values)
{
	portable_vsnprintf1(buf, maxsize, fmt, values);
}

int String::strcasecmp(const std::string &s1, const std::string &s2)
{
	return strcasecmp(s1.c_str(), s2.c_str());
}
int String::strcasecmp(const char *s1, const std::string &s2)
{
	return strcasecmp(s1, s2.c_str());
}
int String::strcasecmp(const std::string &s1, const char *s2)
{
	return strcasecmp(s1.c_str(), s2);
}
int String::strcasecmp(const char *s1, const char *s2)
{
	if (s1 == NULL && s2 != NULL)
		return -1;
	else if (s1 != NULL && s2 == NULL)
		return 1;
	else if (s1 == NULL && s2 == NULL)
		return 0;

#ifdef WIN32
	return ::_stricmp(s1, s2);
#else
	return ::strcasecmp(s1, s2);
#endif
}

int String::strncasecmp(const std::string &s1, const std::string &s2, int len)
{
	return strncasecmp(s1.c_str(), s2.c_str(), len);
}
int String::strncasecmp(const char *s1, const std::string &s2, int len)
{
	return strncasecmp(s1, s2.c_str(), len);
}
int String::strncasecmp(const std::string &s1, const char *s2, int len)
{
	return strncasecmp(s1.c_str(), s2, len);
}
int String::strncasecmp(const char *s1, const char *s2, int len)
{
	if (s1 == NULL && s2 != NULL)
		return -1;
	else if (s1 != NULL && s2 == NULL)
		return 1;
	else if (s1 == NULL && s2 == NULL)
		return 0;

#ifdef WIN32
	return ::_strnicmp(s1, s2, len);
#else
	return ::strncasecmp(s1, s2, len);
#endif
}

bool String::iequals(const std::string &s1, const std::string &s2)
{
	return strcasecmp(s1, s2) == 0;
}
bool String::iequals(const char *s1, const std::string &s2)
{
	return strcasecmp(s1, s2) == 0;
}
bool String::iequals(const std::string &s1, const char *s2)
{
	return strcasecmp(s1, s2) == 0;
}
bool String::iequals(const char *s1, const char *s2)
{
	return strcasecmp(s1, s2) == 0;
}

} // namespace Base
} // namespace Public
