//
//  Copyright (c)1998-2012,  Public Technology
//  All Rights Reserved.
//
//	Description:
//	$Id: Base64.cpp 11 2013-01-22 08:42:03Z  $
//
#include "Base/IntTypes.h"
#include "Base/Base64.h"
#include "Base/BaseTemplate.h"
namespace Public{
namespace Base {
static char b64_to_ascii(char str)
{
	if (str >= 0 && str <= 25) {
		return str + 65;
	}
	else if (str >= 26 && str <= 51) {
		return str + 71;
	}
	else if (str >= 52 && str <= 61) {
		return str - 4;
	}
	else if (str == 62) {
		return '+';
	}
	else {
		return '/';
	}
}

static char ascii_to_b64(char str)
{
	if (str >= 65 && str <= 90) {
		return str - 65;
	}
	else if (str >= 97 && str <= 122) {
		return str - 71;
	}
	else if (str >= 48 && str <= 57) {
		return str + 4;
	}
	else if (str == '+') {
		return 62;
	}
	else {
		return 63;
	}
}

static int check_byte(char byte)
{
	if (!((byte >= 'a' && byte <= 'z') || (byte >= 'A' && byte <= 'Z') ||
		(byte >= '0' && byte <= '9') || byte == '+' || byte == '/')) return 1;

	return 0;
}

static int get_decode_len_from_b64_string(const char *string)
{
	size_t len = 0;
	size_t i;

	if (!string) return 0;

	len = strlen(string);

	for (i = 0; i < (len - 2); i++) {
		if (string[i] == '=') return 0;

		if (check_byte(string[i]) == 1) return 0;
	}

	if (string[i] == '=') {
		if (string[i + 1] != '=') return 0;

		return (int)(len / 4 * 3 - 2);
	}
	else {
		if (check_byte(string[i]) == 1) return 0;

		if (string[i + 1] != '=') {
			if (check_byte(string[i + 1]) == 1) return 0;

			return (int)(len / 4 * 3);
		}

		return (int)(len / 4 * 3 - 1);
	}
}

static char *base64_encode(const char *string,int slen, size_t & count)
{
	int encode_len = 0;
	int slen_quotient = 0;
	int slen_remainder = 0;
	int i;

	char *encode_string = NULL;
	count = 0;

	slen_quotient = slen / 3;
	slen_remainder = slen % 3;
	encode_len = slen_quotient * 4 + (slen_remainder ? 4 : 0);

	encode_string = (char*)malloc(encode_len + 1);
	memset(encode_string,0, encode_len + 1);

	for (i = 0; i < slen_quotient * 3; i += 3) {
		encode_string[count++] = b64_to_ascii((string[i] & 0xFF) >> 2);
		encode_string[count++] = b64_to_ascii(((string[i] & 0x03) << 4) | ((string[i + 1] & 0xFF) >> 4));
		encode_string[count++] = b64_to_ascii(((string[i + 1] & 0x0F) << 2) | ((string[i + 2] & 0xFF) >> 6));
		encode_string[count++] = b64_to_ascii(string[i + 2] & 0x3F);
	}

	if (slen_remainder == 1) {
		encode_string[count++] = b64_to_ascii((string[i] & 0xFF) >> 2);
		encode_string[count++] = b64_to_ascii((string[i] & 0x03) << 4);
		encode_string[count++] = '=';
		encode_string[count++] = '=';
	}
	else if (slen_remainder == 2) {
		encode_string[count++] = b64_to_ascii((string[i] & 0xFF) >> 2);
		encode_string[count++] = b64_to_ascii(((string[i] & 0x03) << 4) | ((string[i + 1] & 0xFF) >> 4));
		encode_string[count++] = b64_to_ascii((string[i + 1] & 0x0F) << 2);
		encode_string[count++] = '=';
	}

	return encode_string;
}

static char *base64_decode(const char *encode_str,int encode_len, size_t & count)
{
	int decode_len = 0;
	int i;
	char *decode_str = NULL;
	count = 0;


	if (encode_len < 4 || encode_len % 4 != 0) {
		return NULL;
	}

	decode_len = get_decode_len_from_b64_string(encode_str);

	if (decode_len == 0) {
		return NULL;
	}
	
	decode_str = (char*)malloc(decode_len + 1);
	memset(decode_str, 0,decode_len + 1);

	for (i = 0; i < encode_len; i += 4) {
		char encode_byte1;
		char encode_byte2;
		char encode_byte3;
		char encode_byte4;

		encode_byte1 = ascii_to_b64(encode_str[i]) & 0xFF;
		encode_byte2 = ascii_to_b64(encode_str[i + 1]) & 0xFF;
		decode_str[count++] = (encode_byte1 << 2) | (encode_byte2 >> 4);

		if (encode_str[i + 2] == '=') break;

		encode_byte3 = ascii_to_b64(encode_str[i + 2]) & 0xFF;
		decode_str[count++] = (encode_byte2 << 4) | (encode_byte3 >> 2);

		if (encode_str[i + 3] == '=') break;

		encode_byte4 = ascii_to_b64(encode_str[i + 3]) & 0xFF;
		decode_str[count++] = (encode_byte3 << 6) | encode_byte4;
	}
	return decode_str;
}

/// base46 encode
std::string Base64::encode(const std::string& src)
{
	size_t len = 0;
	char* enstrbuf = base64_encode(src.c_str(), (int)src.length(),len);
	if (enstrbuf == NULL) return"";

	std::string destsrc(enstrbuf,len);
	SAFE_DELETEARRAY(enstrbuf);

	return Public::Base::move(destsrc);
}


/// base64 decode
std::string Base64::decode(const std::string& src)
{
	size_t len = 0;
	char* decstr = base64_decode(src.c_str(), (int)src.length(), len);
	if (decstr == NULL) return "";

	std::string outputstr(decstr, len);
	SAFE_DELETEARRAY(decstr);

	return Public::Base::move(outputstr);
}



// https://en.wikipedia.org/wiki/Base32
static const char* s_base32_enc = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
static const uint8_t s_base32_dec[256] = {
	0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0, 0, 0, 0, 0, 0, 0, 0, /* 0 - 9 */
	0,    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, /* A - F */
	0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0, 0, 0, 0, 0,
	0,    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, /* a - f */
	0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0, 0, 0, 0, 0,
};

// RFC4648
std::string Base32::encode(const std::string& src)
{
	size_t i, j;

	size_t bytes = src.length();
	const uint8_t *ptr = (const uint8_t*)src.c_str();
	char* target = new char[bytes*2 + 100];

	for (j = i = 0; i < bytes / 5 * 5; i += 5)
	{
		target[j++] = s_base32_enc[(ptr[i] >> 3) & 0x1F]; /* c1 */
		target[j++] = s_base32_enc[((ptr[i] & 0x07) << 2) | ((ptr[i + 1] >> 6) & 0x03)]; /*c2*/
		target[j++] = s_base32_enc[(ptr[i + 1] >> 1) & 0x1F];/*c3*/
		target[j++] = s_base32_enc[((ptr[i + 1] & 0x01) << 4) | ((ptr[i + 2] >> 4) & 0x0F)]; /*c4*/
		target[j++] = s_base32_enc[((ptr[i + 2] & 0x0F) << 1) | ((ptr[i + 3] >> 7) & 0x01)]; /*c5*/
		target[j++] = s_base32_enc[(ptr[i + 3] >> 2) & 0x1F];/*c6*/
		target[j++] = s_base32_enc[((ptr[i + 3] & 0x03) << 3) | ((ptr[i + 4] >> 5) & 0x07)]; /*c7*/
		target[j++] = s_base32_enc[ptr[i + 4] & 0x1F]; /* c8 */
	}

	if (i + 1 == bytes)
	{
		target[j++] = s_base32_enc[(ptr[i] >> 3) & 0x1F]; /* c1 */
		target[j++] = s_base32_enc[((ptr[i] & 0x07) << 2)]; /*c2*/
	}
	else if (i + 2 == bytes)
	{
		target[j++] = s_base32_enc[(ptr[i] >> 3) & 0x1F]; /* c1 */
		target[j++] = s_base32_enc[((ptr[i] & 0x07) << 2) | ((ptr[i + 1] >> 6) & 0x03)]; /*c2*/
		target[j++] = s_base32_enc[(ptr[i + 1] >> 1) & 0x1F];/*c3*/
		target[j++] = s_base32_enc[((ptr[i + 1] & 0x01) << 4)]; /*c4*/
	}
	else if (i + 3 == bytes)
	{
		target[j++] = s_base32_enc[(ptr[i] >> 3) & 0x1F]; /* c1 */
		target[j++] = s_base32_enc[((ptr[i] & 0x07) << 2) | ((ptr[i + 1] >> 6) & 0x03)]; /*c2*/
		target[j++] = s_base32_enc[(ptr[i + 1] >> 1) & 0x1F];/*c3*/
		target[j++] = s_base32_enc[((ptr[i + 1] & 0x01) << 4) | ((ptr[i + 2] >> 4) & 0x0F)]; /*c4*/
		target[j++] = s_base32_enc[((ptr[i + 2] & 0x0F) << 1)]; /*c5*/
	}
	else if (i + 4 == bytes)
	{
		target[j++] = s_base32_enc[(ptr[i] >> 3) & 0x1F]; /* c1 */
		target[j++] = s_base32_enc[((ptr[i] & 0x07) << 2) | ((ptr[i + 1] >> 6) & 0x03)]; /*c2*/
		target[j++] = s_base32_enc[(ptr[i + 1] >> 1) & 0x1F];/*c3*/
		target[j++] = s_base32_enc[((ptr[i + 1] & 0x01) << 4) | ((ptr[i + 2] >> 4) & 0x0F)]; /*c4*/
		target[j++] = s_base32_enc[((ptr[i + 2] & 0x0F) << 1) | ((ptr[i + 3] >> 7) & 0x01)]; /*c5*/
		target[j++] = s_base32_enc[(ptr[i + 3] >> 2) & 0x1F];/*c6*/
		target[j++] = s_base32_enc[((ptr[i + 3] & 0x03) << 3)]; /*c7*/
	}

	while (0 != (j % 8))
	{
		target[j++] = '=';
	}

	std::string encstr(target, j);
	delete[] target;

	return encstr;
}

std::string Base32::decode(const std::string& src)
{
	size_t i, j;
	
	size_t bytes = src.length();
	const uint8_t* source = (const uint8_t*)src.c_str();
	const uint8_t* end = source + bytes;
	
	char* target = new char[bytes * 2 + 100];
	uint8_t* p = (uint8_t*)target;
	

	i = 0;
	for (j = 1; j < bytes / 8; j++)
	{
		p[i++] = (s_base32_dec[source[0]] << 3) | (s_base32_dec[source[1]] >> 2);
		p[i++] = (s_base32_dec[source[1]] << 6) | (s_base32_dec[source[2]] << 1) | (s_base32_dec[source[3]] >> 4);
		p[i++] = (s_base32_dec[source[3]] << 4) | (s_base32_dec[source[4]] >> 1);
		p[i++] = (s_base32_dec[source[4]] << 7) | (s_base32_dec[source[5]] << 2) | (s_base32_dec[source[6]] >> 3);
		p[i++] = (s_base32_dec[source[6]] << 5) | s_base32_dec[source[7]];
		source += 8;
	}

	if (source < end)
	{
#define S(i) ((source+i < end) ? source[i] : '=')
		p[i++] = (s_base32_dec[S(0)] << 3) | (s_base32_dec[S(1)] >> 2);
		if ('=' != S(2)) p[i++] = (s_base32_dec[S(1)] << 6) | (s_base32_dec[S(2)] << 1) | (s_base32_dec[S(3)] >> 4);
		if ('=' != S(4)) p[i++] = (s_base32_dec[S(3)] << 4) | (s_base32_dec[S(4)] >> 1);
		if ('=' != S(5)) p[i++] = (s_base32_dec[S(4)] << 7) | (s_base32_dec[S(5)] << 2) | (s_base32_dec[S(6)] >> 3);
		if ('=' != S(7)) p[i++] = (s_base32_dec[S(6)] << 5) | s_base32_dec[S(7)];
#undef S
	}

	std::string decstr(target, i);

	delete[]target;

	return decstr;
}


static const char* s_base16_enc = "0123456789ABCDEF";
static const uint8_t s_base16_dec[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 0 - 9 */
	0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* A - F */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,10,11,12,13,14,15, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* a - f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

// RFC3548 6. Base 16 Encoding (p8)
std::string Base16::encode(const std::string& src)
{
	size_t i;
	size_t bytes = src.length();
	const uint8_t* p = (const uint8_t*)src.c_str();
	char* target = new char[bytes*2 + 100];

	for (i = 0; i < bytes; i++)
	{
		target[i * 2] = s_base16_enc[(*p >> 4) & 0x0F];
		target[i * 2 + 1] = s_base16_enc[*p & 0x0F];
		++p;
	}

	std::string encstr(target, bytes * 2);
	delete[] target;

	return encstr;
}

std::string Base16::decode(const std::string& src)
{
	size_t i;
	size_t bytes = src.length();
	const char *source = src.c_str();
	char* target = new char[src.length() + 100];
	uint8_t* p = (uint8_t*)target;


	for (i = 0; i < bytes / 2; i++)
	{
		p[i] = s_base16_dec[(unsigned char)source[i * 2]] << 4;
		p[i] |= s_base16_dec[(unsigned char)source[i * 2 + 1]];
	}

	std::string decstr(target, i);
	delete[] target;

	return decstr;
}

}//namespace Base
}//namespace Public

