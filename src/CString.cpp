/*
 * include/CString.cpp
 *
 * Copyright 2007-2019 OpenGraal
 *
 * This file is part of GS2Emu.
 *
 * GS2Emu is free software : you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * GS2Emu is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GS2Emu. If not, see <http://www.gnu.org/licenses/>.
*/

#include <cassert>
#include <stdexcept>
#include "CString.h"
#include "bzlib.h"
#include "zlib.h"

#if defined(_WIN32) || defined(_WIN64)
	#include <windows.h>

	extern "C" {
		char * strsep (char **stringp, const char *delim)
		{
			char *begin, *end;
			begin = *stringp;
			if (begin == NULL)
				return NULL;

			/* Find the end of the token.  */
			end = begin + strcspn (begin, delim);
			if (*end)
			{
				/* Terminate the token and set *STRINGP past NUL character.  */
				*end++ = '\0';
				*stringp = end;
			}
			else
				/* No more delimiters; this is the last token.  */
				*stringp = NULL;
			return begin;
		}
	}
	#ifdef _MSC_VER
		#define strncasecmp _strnicmp
	#endif
#endif

#if defined(WOLFSSL_ENABLED)
	#include <wolfssl/wolfcrypt/hash.h>
	#include <wolfssl/wolfcrypt/arc4.h>
#endif

/*
	Constructor ~ Deconstructor
*/
CString::CString(const char *pString)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), sizec(0), readc(0), writec(0)
{
	if (pString != 0)
	{
		auto length = strlen(pString);
		if (length != 0)
		{
			clear(length);
			write(pString, length);
			return;
		}
	}

	_internalBuffer[0] = 0;
}

CString::CString(const std::string& pString)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), sizec(0), readc(0), writec(0)
{
	if (pString.length())
	{
		write(pString.c_str(), pString.length());
		return;
	}

	_internalBuffer[0] = 0;
}

CString::CString(const std::string_view& pString)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), sizec(0), readc(0), writec(0)
{
	if (pString.length())
	{
		write(pString.data(), pString.length());
		return;
	}

	_internalBuffer[0] = 0;
}

CString::CString(const CString& pString)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), sizec(0), readc(0), writec(0)
{
	if (pString.length())
	{
		write(pString.text(), pString.length());
		return;
	}

	_internalBuffer[0] = 0;
}

CString::CString(double val)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), readc(0)
{
	sprintf(_internalBuffer, "%f", val);
	sizec = writec = strlen(_internalBuffer);
}

CString::CString(float val)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), readc(0)
{
	sprintf(_internalBuffer, "%.2f", val);
	sizec = writec = strlen(_internalBuffer);
}

CString::CString(int val)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), readc(0)
{
	sprintf(_internalBuffer, "%i", val);
	sizec = writec = strlen(_internalBuffer);
}

CString::CString(unsigned int val)
	: buffer(_internalBuffer), _heapBuffer(0), buffc(30), readc(0)
{
	sprintf(_internalBuffer, "%u", val);
	sizec = writec = strlen(_internalBuffer);
}

CString::CString(long val)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), readc(0)
{
	sprintf(_internalBuffer, "%ld", val);
	sizec = writec = strlen(_internalBuffer);
}

CString::CString(unsigned long val)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), readc(0)
{
	sprintf(_internalBuffer, "%lu", val);
	sizec = writec = strlen(_internalBuffer);
}

CString::CString(long long val)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), readc(0)
{
	sprintf(_internalBuffer, "%lld", val);
	sizec = writec = strlen(_internalBuffer);
}

CString::CString(unsigned long long val)
	: buffer(_internalBuffer), _heapBuffer(nullptr), buffc(30), readc(0)
{
	sprintf(_internalBuffer, "%llu", val);
	sizec = writec = strlen(_internalBuffer);
}

/*
	Data-Management
*/

bool CString::load(const CString& pString)
{
	char buff[65535];
	FILE *file = 0;

#if defined(WIN32) || defined(WIN64)
	wchar_t* wcstr = 0;

	// Determine if the filename is UTF-8 encoded.
	int wcsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pString.text(), pString.length(), 0, 0);
	if (wcsize != 0)
	{
		wcstr = new wchar_t[wcsize + 1];
		memset((void *)wcstr, 0, (wcsize + 1) * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, pString.text(), pString.length(), wcstr, wcsize);
	}
	else
	{
		wcstr = new wchar_t[pString.length() + 1];
		for (auto i = 0; i < pString.length(); ++i)
			wcstr[i] = (unsigned char)pString[i];
		wcstr[pString.length()] = 0;
	}

	// Open the file now.
	file = _wfopen(wcstr, L"rb");
	delete[] wcstr;
	if (file == 0)
		return false;
#else
	// Linux uses UTF-8 filenames.
	if ((file = fopen(pString.text(), "rb")) == nullptr)
		return false;
#endif

	int size = 0;
	clear();
	while ((size = (int)fread(buff, 1, sizeof(buff), file)) > 0)
		write(buff, size);
	fclose(file);
	return true;
}

bool CString::save(const CString& pString) const
{
	FILE *file = 0;

#if defined(WIN32) || defined(WIN64)
	wchar_t* wcstr = 0;

	// Determine if the filename is UTF-8 encoded.
	int wcsize = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pString.text(), pString.length(), 0, 0);
	if (wcsize != 0)
	{
		wcstr = new wchar_t[wcsize + 1];
		memset((void *)wcstr, 0, (wcsize + 1) * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, pString.text(), pString.length(), wcstr, wcsize);
	}
	else
	{
		wcstr = new wchar_t[pString.length() + 1];
		for (auto i = 0; i < pString.length(); ++i)
			wcstr[i] = (unsigned char)pString[i];
		wcstr[pString.length()] = 0;
	}

	// Open the file now.
	file = _wfopen(wcstr, L"wb");
	delete[] wcstr;
	if (file == 0)
		return false;
#else
	// Linux uses UTF-8 filenames.
	if ((file = fopen(pString.text(), "wb")) == 0)
		return false;
#endif

	fwrite(buffer, 1, sizec, file);
	fflush(file);
	fclose(file);
	return true;
}

CString CString::readChars(int pLength)
{
	pLength = std::clamp(pLength, 0, sizec - readc);

	CString retVal;
	retVal.write(&buffer[readc], pLength);
	readc += pLength;
	return retVal;
}

CString CString::readString(const CString& pString)
{
	CString retVal;
	if (readc > sizec) return retVal;

	int len = -1;
	if (!pString.isEmpty())
		len = find(pString, readc) - readc;
	if (len < 0 || len > bytesLeft())
		len = bytesLeft();
	retVal.write(&buffer[readc], len);

	// If len was set to bytesLeft(), it MIGHT have pString.
	int len2;
	if ((len2 = retVal.find(pString)) != -1)
		retVal.removeI(len2, pString.length());

	readc += len + pString.length();
	return retVal;
}

int CString::read(char *pDest, int pSize)
{
	int length = (sizec - readc < pSize ? sizec - readc : pSize);
	if (length <= 0)
	{
		memset((void*)pDest, 0, pSize);
		return 0;
	}
	memcpy((void*)pDest, (void*)&buffer[readc], length);
	readc += length;
	return length;
}

int CString::write(unsigned char *pSrc, int pSize, bool nullTerminate)
{
	if (!pSize)
		return 0;

	if (writec + pSize >= buffc)
	{
		resize(writec + pSize + 10 + (sizec / 3));
	}

	memcpy(&buffer[writec], pSrc, pSize);
	writec += pSize;
	if (nullTerminate)
		buffer[writec] = 0;
	sizec = (writec > sizec ? writec : sizec);
	return pSize;
}

int CString::write(const char *pSrc, int pSize, bool nullTerminate)
{
	if (!pSize)
		return 0;

	if (writec + pSize >= buffc)
	{
		resize(writec + pSize + 10 + (sizec / 3));
	}

	memcpy(&buffer[writec], pSrc, pSize);
	writec += pSize;
	if (nullTerminate)
		buffer[writec] = 0;
	sizec = (writec > sizec ? writec : sizec);
	return pSize;
}

void CString::clear(size_t size)
{
	if (_heapBuffer)
	{
		free(_heapBuffer);
		_heapBuffer = nullptr;
	}

	//
	++size;

	if (size >= INTERNAL_BUFFER_SIZE)
	{
		_heapBuffer = (char*)malloc(size);
		buffer = _heapBuffer;
	}
	else buffer = _internalBuffer;

	sizec = readc = writec = 0;
	buffc = std::max(size_t{ 1 }, size);
	buffer[0] = '\0';
}

void CString::resize(size_t size)
{
	if (size < buffc)
		return;

	++size;

	if (size >= INTERNAL_BUFFER_SIZE)
	{
		if (!_heapBuffer)
		{
			_heapBuffer = (char*)malloc(size);
			memcpy(_heapBuffer, _internalBuffer, buffc);
		}
		else
		{
			auto tmp = _heapBuffer;
			_heapBuffer = (char*)realloc(_heapBuffer, size);
			if (!_heapBuffer)
				free(tmp);
		}

		buffer = _heapBuffer;
		assert(buffer != nullptr); // allocation failure, unhandled
	}

	buffc = size;
}

/*
	Functions
*/

CString CString::escape() const
{
	CString retVal;

	for (auto i = 0; i < length(); i++)
	{
		if (buffer[i] == '\\')
			retVal << "\\\\";
		else if (buffer[i] == '\"')
			retVal << "\"\"";
		else if (buffer[i] == '\'')
			retVal << "\'\'";
		else
			retVal << buffer[i];
	}

	return retVal;
}

CString CString::unescape() const
{
	CString retVal;

	for (auto i = 0; i < length(); i++)
	{
		char cur = buffer[i];
		char nex = buffer[++i];

		if (cur == '\\' && nex == '\\')
			retVal << "\\";
		else if (cur == '\"' && nex == '\"')
			retVal << "\"";
		else if (cur == '\'' && nex == '\'')
			retVal << "\'";
		else
			retVal << buffer[--i];
	}

	return retVal;
}

CString CString::left(int pLength) const
{
	return subString(0, pLength);
}

CString CString::right(int pLength) const
{
	return subString(length() - pLength, pLength);
}

CString CString::remove(int pStart, int pLength) const
{
	CString retVal(*this);
	if (pLength == 0 || pStart < 0)
		return retVal;
	if (pStart >= sizec)
		return retVal;
	if (pLength == -1)
		pLength = retVal.length();

	pLength = std::clamp(pLength, 0, retVal.length()-pStart);
	memmove(retVal.text() + pStart, retVal.text() + pStart + pLength, retVal.length() - pStart - (pLength - 1));
	retVal.setSize(retVal.length() - pLength);
	retVal[retVal.length()] = 0;
	return retVal;
}

CString CString::removeAll(const CString& pString) const
{
	// Sanity checks.
	if (pString.length() == 0 || length() == 0) return *this;

	// First check and see if we even have the string to remove.
	// pLoc will be initially set here.
	int pLoc = find(pString);
	if (pLoc == -1) return *this;

	// Resize retVal to the current length of the class.
	// This will prevent unnecessary realloc() calls.
	CString retVal;
	retVal.clear(sizec);

	// Construct retVal.
	const int pLen = pString.length();
	int pStart = 0;
	do
	{
		int pRead = pLoc - pStart;
		retVal << subString(pStart, pRead);
		pStart += (pRead + pLen);
	}
	while ((pLoc = find(pString, pStart)) != -1);
	retVal << subString(pStart);

	// Done!
	return retVal;
}

CString CString::subString(int pStart, int pLength) const
{
	// Read Entire String?
	if (pLength == -1)
		pLength = length();

	if (pStart >= length())
		return CString();

	CString retVal;
	pStart = std::clamp(pStart, 0, length());
	pLength = std::clamp(pLength, 0, length()-pStart);

	if (pLength > 0)
		retVal.write(&buffer[pStart], pLength);
	return retVal;
}

CString CString::toLower() const
{
	CString retVal(*this);
	for (auto i = 0; i < retVal.length(); i++)
	{
		if (inrange(retVal[i], 'A', 'Z'))
			retVal[i] += 32;
	}

	return retVal;
}

CString CString::toUpper() const
{
	CString retVal(*this);
	for (auto i = 0; i < retVal.length(); i++)
	{
		if (inrange(retVal[i], 'a', 'z'))
			retVal[i] -= 32;
	}

	return retVal;
}

CString CString::trim() const
{
	return trimLeft().trimRight();
}

CString CString::trimLeft() const
{
	for (auto i = 0; i < length(); ++i)
	{
		if ((unsigned char)buffer[i] > (unsigned char)' ')
			return subString(i, length() - i);
	}

	return CString();
	//return CString(*this);
}

CString CString::trimRight() const
{
	for (auto i = length() - 1; i >= 0; --i)
	{
		if ((unsigned char)buffer[i] > (unsigned char)' ')
			return subString(0, i+1);
	}

	return CString(*this);
}

#if defined(WOLFSSL_ENABLED)
CString CString::sha1() const
{
	CString retVal;

	int hash_len = wc_HashGetDigestSize(WC_HASH_TYPE_SHA);

	byte *hash = new byte[hash_len];
	memset((void*)hash, 0, hash_len);

	wc_Hash(WC_HASH_TYPE_SHA, reinterpret_cast<const byte *>(text()), length(), hash, hash_len);

	retVal.write(hash, hash_len);
	delete [] hash;

	return retVal;
}

CString CString::rc4_encrypt(const char * key, int keylen) const
{
	CString retVal;
	byte* buf = new byte[length()];
	memset((void*)buf, 0, length());
	int error = 0;
	unsigned int clen = length();

	Arc4* enc = new Arc4();

	wc_Arc4SetKey(enc, reinterpret_cast<const byte *>(key), keylen);
	wc_Arc4Process(enc, buf, reinterpret_cast<const byte *>(buffer), clen);

	wc_Arc4Free(enc);

	retVal.write((const char*)buf, clen);
	delete [] buf;
	return retVal;
}

CString CString::rc4_decrypt(const char * key, int keylen) const
{
	CString retVal;
	byte* buf = new byte[length()];
	memset((void*)buf, 0, length());
	int error = 0;
	unsigned int clen = length();

	Arc4* dec = new Arc4();

	wc_Arc4SetKey(dec, reinterpret_cast<const byte *>(key), keylen);
	wc_Arc4Process(dec, buf, reinterpret_cast<const byte *>(buffer), clen);

	wc_Arc4Free(dec);

	retVal.write((const char*)buf, clen);
	delete [] buf;
	return retVal;
}

#endif

CString CString::encodesimple(unsigned int buffSize) const
{
	CString retVal;
	char* buf = new char[buffSize];
	memset((void*)buf, 0, buffSize);
	int error = 0;
	unsigned int clen = buffSize;

	int i = 0;

	do
	{
		buf[i] = ~(((buffer[i] + (char)i >> 6) & 0x03) + (char)(clen + 0xa) + (char)((buffer[i] + (char)i) * (char)0x04));

		i++;
	} while ((int)i != (int)clen);

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

CString CString::decodesimple(unsigned int buffSize, bool includeNullTermination) const
{
	CString retVal;
	char* buf = new char[buffSize];
	memset((void*)buf, 0, buffSize);
	int error = 0;
	unsigned int clen = buffSize;

	int i = 0;
	int c = 0;

	do
	{
		char key = (-0x0a - clen) + ~buffer[i];
		char soln = (((int)(key & 0xfc) >> 2) + (key * (char)'@' & 0xff)) - i;
		if (soln != 0x00 || includeNullTermination)
		{
			buf[c] = soln;
			c++;
		}
		i++;
	} while (i != clen);

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

CString CString::codesimplefix0(unsigned int buffSize) const
{
	CString retVal;
	char* buf = new char[buffSize];
	//memset((void*)buf, 0, buffSize);
	memcpy(buf, buffer, buffSize);
	int error = 0;
	unsigned int clen = buffSize;



	char key;
	int i = 0;

	while( true ) {
		key = (-0x0a - clen) + ~buf[i];
		if (((int)(key & 0xfc) >> 2) + (key * (char)'@' & 0xff) == i) {
			buf[i] = '\0';
		}
		if (i + 1 == clen) break;
		i++;
	}

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

const char CString::BASE64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string CString::formatBase64(std::size_t num, std::size_t len) {
	std::string str;
	for ( std::size_t i = 0; i < len; ++i ) {
		int index = (num >> (len - i - 1) * 6) & 0x3F; // 6 bit per character
		str += BASE64[index];
	}
	return str;
}

CString CString::base64encode() const
{
	CString retVal = CString("");

	static constexpr char sEncodingTable[] = {
			'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
			'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
			'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
			'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
			'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
			'w', 'x', 'y', 'z', '0', '1', '2', '3',
			'4', '5', '6', '7', '8', '9', '+', '/'
	};

	size_t in_len = length();
	size_t out_len = 4 * ((in_len + 2) / 3);
	std::string ret(out_len, '\0');
	size_t i;
	char *p = const_cast<char*>(ret.c_str());

	for (i = 0; i < in_len - 2; i += 3) {
		*p++ = sEncodingTable[(buffer[i] >> 2) & 0x3F];
		*p++ = sEncodingTable[((buffer[i] & 0x3) << 4) | ((int) (buffer[i + 1] & 0xF0) >> 4)];
		*p++ = sEncodingTable[((buffer[i + 1] & 0xF) << 2) | ((int) (buffer[i + 2] & 0xC0) >> 6)];
		*p++ = sEncodingTable[buffer[i + 2] & 0x3F];
	}
	if (i < in_len) {
		*p++ = sEncodingTable[(buffer[i] >> 2) & 0x3F];
		if (i == (in_len - 1)) {
			*p++ = sEncodingTable[((buffer[i] & 0x3) << 4)];
			*p++ = '=';
		}
		else {
			*p++ = sEncodingTable[((buffer[i] & 0x3) << 4) | ((int) (buffer[i + 1] & 0xF0) >> 4)];
			*p++ = sEncodingTable[((buffer[i + 1] & 0xF) << 2)];
		}
		*p++ = '=';
	}

	retVal.write(ret.c_str());

	ret.clear();

	return retVal;
}

CString CString::base64decode() const
{
    /* Check if last byte is null terminator */
	int in_len = buffer[length() - 1] == 0 ? length() - 1 : length();

	CString retVal = CString("");

	static constexpr unsigned char kDecodingTable[] = {
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
			52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
			64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
			15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
			64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
			41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
			64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
	};

	//size_t in_len = input.size();
	if (in_len % 4 != 0) return *this;//"Input data size is not a multiple of 4";

	size_t out_len = in_len / 4 * 3;
	std::string out(out_len, '\0');
	if (buffer[in_len - 1] == '=') out_len--;
	if (buffer[in_len - 2] == '=') out_len--;

	out.resize(out_len);

	for (size_t i = 0, j = 0; i < in_len;) {
		uint32_t a = buffer[i] == '=' ? 0 & i++ : kDecodingTable[static_cast<int>(buffer[i++])];
		uint32_t b = buffer[i] == '=' ? 0 & i++ : kDecodingTable[static_cast<int>(buffer[i++])];
		uint32_t c = buffer[i] == '=' ? 0 & i++ : kDecodingTable[static_cast<int>(buffer[i++])];
		uint32_t d = buffer[i] == '=' ? 0 & i++ : kDecodingTable[static_cast<int>(buffer[i++])];

		uint32_t triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);

		if (j < out_len) out[j++] = (triple >> 2 * 8) & 0xFF;
		if (j < out_len) out[j++] = (triple >> 1 * 8) & 0xFF;
		if (j < out_len) out[j++] = (triple >> 0 * 8) & 0xFF;
	}

	retVal.write(out.c_str(), out.length());
	out.clear();

	return retVal;
}

CString CString::bzcompress(unsigned int buffSize) const
{
	CString retVal;
	char* buf = new char[buffSize];
	memset((void*)buf, 0, buffSize);
	int error = 0;
	unsigned int clen = buffSize;

	if ((error = BZ2_bzBuffToBuffCompress(buf, &clen, buffer, length(), 1, 0, 30)) != BZ_OK)
	{
		delete [] buf;
		return retVal;
	}

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

CString CString::bzuncompress(unsigned int buffSize) const
{
	CString retVal;
	char* buf = new char[buffSize];
	memset((void*)buf, 0, buffSize);
	int error = 0;
	unsigned int clen = buffSize;

	if ((error = BZ2_bzBuffToBuffDecompress(buf, &clen, buffer, length(), 0, 0)) != BZ_OK)
	{
		delete [] buf;
		return retVal;
	}

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

CString CString::zcompress(unsigned int buffSize) const
{
	CString retVal;
	char* buf = new char[buffSize];
	memset((void*)buf, 0, buffSize);
	int error = 0;
	unsigned long clen = buffSize;

	if ((error = compress((Bytef *)buf, (uLongf *)&clen, (const Bytef *)buffer, length())) != Z_OK)
	{
		delete [] buf;
		return retVal;
	}

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

CString CString::zuncompress(unsigned int buffSize) const
{
	CString retVal;
	char* buf = new char[buffSize];
	memset((void*)buf, 0, buffSize);
	int error = 0;
	unsigned long clen = buffSize;

	if ((error = uncompress((Bytef *)buf, (uLongf *)&clen, (const Bytef *)buffer, length())) != Z_OK)
	{
		switch (error)
		{
			case Z_MEM_ERROR:
				printf("[zlib] Not enough memory to decompress.\n");
				break;
			case Z_BUF_ERROR:
				printf("[zlib] Not enough room in the output buffer to decompress.\n");
				break;
			case Z_DATA_ERROR:
				printf("[zlib] The input data was corrupted.\n");
				break;
		}
		delete [] buf;
		return retVal;
	}

	retVal.write(buf, clen);
	delete [] buf;
	return retVal;
}

int CString::find(const CString& pString, int pStart) const
{
	const char* obuffer = pString.text();
	const int olen = pString.length();
	for (auto i = pStart; i <= sizec - olen; ++i)
	{
		if (buffer[i] == 0)
		{
			if (olen == 0) return i;
			else continue;
		}
		if (memcmp(buffer + i, obuffer, olen) == 0)
			return i;
	}
	return -1;
}

int CString::find(const std::string_view& pString, int pStart) const
{
	const char* obuffer = pString.data();
	const int olen = pString.length();
	for (auto i = pStart; i <= sizec - olen; ++i)
	{
		if (buffer[i] == 0)
		{
			if (olen == 0) return i;
			else continue;
		}
		if (memcmp(buffer + i, obuffer, olen) == 0)
			return i;
	}
	return -1;
}

int CString::find(const char* pString, int pStart) const
{
	const char* obuffer = pString;
	const int olen = strlen(pString);
	for (auto i = pStart; i <= sizec - olen; ++i)
	{
		if (buffer[i] == 0)
		{
			if (olen == 0) return i;
			else continue;
		}
		if (memcmp(buffer + i, obuffer, olen) == 0)
			return i;
	}
	return -1;
}

int CString::findi(const CString& pString, int pStart) const
{
	for (auto i = pStart; i <= length() - pString.length(); ++i)
	{
		if (strncasecmp(&buffer[i], pString.text(), pString.length()) == 0)
			return i;
	}
	return -1;
}

int CString::findi(const std::string_view& pString, int pStart) const
{
	for (auto i = pStart; i <= length() - pString.length(); ++i)
	{
		if (strncasecmp(&buffer[i], pString.data(), pString.length()) == 0)
			return i;
	}
	return -1;
}

int CString::findi(const char* pString, int pStart) const
{
	auto len = strlen(pString);
	for (auto i = pStart; i <= length() - len; ++i)
	{
		if (strncasecmp(&buffer[i], pString, len) == 0)
			return i;
	}
	return -1;
}

int CString::findl(char pChar) const
{
	char* loc = strrchr(buffer, (int)pChar);
	if (loc == 0) return -1;
	return (int)(loc - buffer);
}

std::vector<CString> CString::tokenize(const CString& pString, bool keepEmpty) const
{
	std::vector<CString> strList;
	char *string = strdup(buffer);
	char *tok;

	if (!keepEmpty)
	{
		tok = strtok(string, pString.text());
		while ( tok != nullptr )
		{
			strList.emplace_back(CString{ tok });
			tok = strtok(nullptr, pString.text());
		}
	}
	else
	{
		while ( (tok = strsep(&string, pString.text())) != nullptr )
		{
			strList.emplace_back(tok);
		}
	}

	free(string);

	return strList;
}

std::vector<CString> CString::tokenizeConsole() const
{
	std::vector<CString> strList;
	bool quotes = false;
	CString str;
	for (auto i = 0; i < sizec; ++i)
	{
		switch (buffer[i])
		{
			case ' ':
				if (!quotes)
				{
					strList.push_back(str);
					str.clear(30);
				}
				else str.write(buffer + i, 1);
				break;
			case '\"':
				quotes = !quotes;
				break;
			case '\\':
				if (i + 1 < sizec)
				{
					switch (buffer[i + 1])
					{
						case '\"':
							str.write("\"", 1);
							++i;
							break;
						case '\\':
							str.write("\\", 1);
							++i;
							break;
						default:
							str.write(buffer + i, 1);
							break;
					}
				}
				else str.write(buffer + i, 1);
				break;
			default:
				str.write(buffer + i, 1);
				break;
		}
	}
	strList.push_back(str);
	return strList;
}

CString CString::join(std::vector<CString>& rows, const CString& pToken)
{
	CString join = CString();

	for (auto& row : rows) {
		join << row << pToken;
		row.clear(0);
	}

	return join;
}

std::vector<CString> CString::loadToken(const CString& pFile, const CString& pToken, bool removeCR)
{
	CString fileData;
	if (!fileData.load(pFile))
		return std::vector<CString>();

	if (removeCR) fileData.removeAllI("\r");

	// parse file
	std::vector<CString> result;
	while (fileData.bytesLeft())
		result.push_back(fileData.readString(pToken));

	// return
	return result;
}

CString CString::replaceAll(const CString& pString, const CString& pNewString) const
{
	CString retVal(*this);
	int pos = 0;
	int len = pString.length();
	int len2 = pNewString.length();

	while (true)
	{
		pos = retVal.find(pString, pos);
		if (pos == -1) break;

		// Remove the string.
		retVal.removeI(pos, len);

		// Add the new string where the removed data used to be.
		retVal = CString() << retVal.subString(0, pos) << pNewString << retVal.subString(pos);

		pos += len2;
	}
	return retVal;
}

CString CString::gtokenize() const
{
	CString self(*this);
	CString retVal;
	int pos[] = {0, 0};

	// Add a trailing \n to the line if one doesn't already exist.
	if (buffer[sizec - 1] != '\n') self.writeChar('\n');

	// Do the tokenization stuff.
	while ((pos[0] = self.find("\n", pos[1])) != -1)
	{
		CString temp(self.subString(pos[1], pos[0] - pos[1]));
		temp.removeAllI("\r");
		if (!temp.isEmpty())
		{
			// Check for a complex word.
			bool complex = false;
			if (temp[0] == '"') complex = true;
			for (auto i = 0; i < temp.length() && !complex; ++i)
			{
				if (temp[i] < 33 || temp[i] > 126 || temp[i] == 44 || temp[i] == 47)
					complex = true;
			}

			// If it is entirely whitespace, put it inside quotation marks.
			if (temp.trim().isEmpty())
				complex = true;

			// Put complex words inside quotation marks.
			if (complex)
			{
				temp.replaceAllI("\\", "\\\\");		// Escape '\'
				temp.replaceAllI( "\"", "\"\"" );	// Change all " to ""
				retVal << "\"" << temp << "\",";
			}
			else retVal << temp << ",";
		}
		else retVal << ",";
		pos[1] = pos[0] + 1;
	}

	// Kill the trailing comma and return our new string.
	retVal.removeI(retVal.length() - 1, 1);
	return retVal;
}

CString CString::guntokenize() const
{
	CString retVal;
	retVal.clear(length() + 5);
	bool is_paren = false;

	// Check to see if we are starting with a quotation mark.
	int i = 0;
	if (buffer[0] == '"')
	{
		is_paren = true;
		++i;
	}

	// Untokenize.
	for (; i < length(); ++i)
	{
		// If we encounter a comma not inside a quoted string, we are encountering
		// a new index.  Replace the comma with a newline.
		if (buffer[i] == ',' && !is_paren)
		{
			retVal << "\n";

			// Ignore whitespace.
			while (i + 1 < length() && buffer[i + 1] == ' ')
				++i;

			// Check to see if the next string is quoted.
			if (i + 1 < length() && buffer[i + 1] == '"')
			{
				is_paren = true;
				++i;
			}
		}
		// We need to handle quotation marks as they have different behavior in quoted strings.
		else if (buffer[i] == '"')
		{
			// If we are encountering a quotation mark in a quoted string, we are either
			// ending the quoted string or escaping a quotation mark.
			if (is_paren)
			{
				if (i + 1 < length())
				{
					// Escaping a quotation mark.
					if (buffer[i + 1] == '"')
					{
						retVal << "\"";
						++i;
					}
					// Ending the quoted string.
					else if (buffer[i + 1] == ',')
						is_paren = false;
				}
			}
			// A quotation mark in a non-quoted string.
			else retVal << buffer[i];
		}
		// Unescape '\' character
		else if (buffer[i] == '\\')
		{
			if (i + 1 < length())
			{
				if (buffer[i + 1] == '\\')
				{
					retVal << "\\";
					i++;
				}
			}
		}
		// Anything else gets put to the output.
		else retVal << buffer[i];
	}

	return retVal;
}

std::vector<CString> CString::gCommaStrTokens() const
{
	std::vector<CString> retData;

	CString line;
	bool is_paren = false;

	// Check to see if we are starting with a quotation mark.
	int i = 0;
	if (buffer[0] == '"')
	{
		is_paren = true;
		++i;
	}

	// Untokenize.
	for (; i < length(); ++i)
	{
		// If we encounter a comma not inside a quoted string, we are encountering
		// a new index.  Replace the comma with a newline.
		if (buffer[i] == ',' && !is_paren)
		{
			retData.push_back(line);
			line.clear();

			// Ignore whitespace.
			while (i + 1 < length() && buffer[i + 1] == ' ')
				++i;

			// Check to see if the next string is quoted.
			if (i + 1 < length() && buffer[i + 1] == '"')
			{
				is_paren = true;
				++i;
			}
		}
		// We need to handle quotation marks as they have different behavior in quoted strings.
		else if (buffer[i] == '"')
		{
			// If we are encountering a quotation mark in a quoted string, we are either
			// ending the quoted string or escaping a quotation mark.
			if (is_paren)
			{
				if (i + 1 < length())
				{
					// Escaping a quotation mark.
					if (buffer[i + 1] == '"')
					{
						line << "\"";
						++i;
					}
					// Ending the quoted string.
					else if (buffer[i + 1] == ',')
						is_paren = false;
				}
			}
			// A quotation mark in a non-quoted string.
			else line << buffer[i];
		}
		// Unescape '\' character
		else if (buffer[i] == '\\')
		{
			if (i + 1 < length())
			{
				if (buffer[i + 1] == '\\')
				{
					line << "\\";
					i++;
				}
			}
		}
		// Anything else gets put to the output.
		else line << buffer[i];
	}

	if (is_paren || !line.isEmpty())
		retData.push_back(line);
	return retData;
}

bool CString::match(const CString& pMask) const
{
	char stopstring[1];
	*stopstring = 0;
	const char* matchstring = buffer;
	const char* wildstring = pMask.text();

	while (*matchstring)
	{
		if (*wildstring == '*')
		{
			if (!*++wildstring)
				return true;
			else *stopstring = *wildstring;
		}

		if (*stopstring)
		{
			if (*stopstring == *matchstring)
			{
				wildstring++;
				matchstring++;
				*stopstring = 0;
			}
			else matchstring++;
		}
		else if ((*wildstring == *matchstring) || (*wildstring == '?'))
		{
			wildstring++;
			matchstring++;
		}
		else return false;

		if (!*matchstring && *wildstring && *wildstring != '*')
		{
			// matchstring too short
			return false;
		}
	}
	return true;
}

bool CString::comparei(const CString& pOther) const
{
	if (strncasecmp(buffer, pOther.text(), std::max(sizec, pOther.length())) == 0)
		return true;
	return false;
}

bool CString::contains(const CString& characters) const
{
	for (auto i = 0; i < sizec; ++i)
	{
		for (int j = 0; j < characters.length(); ++j)
		{
			if (buffer[i] == characters[j])
				return true;
		}
	}
	return false;
}

bool CString::onlyContains(const CString& characters) const
{
	for (auto i = 0; i < sizec; ++i)
	{
		bool test = false;
		for (int j = 0; j < characters.length(); ++j)
		{
			if (buffer[i] == characters[j])
			{
				test = true;
				j = characters.length();
			}
		}
		if (test == false)
			return false;
	}
	return true;
}

bool CString::isNumber() const
{
	const char numbers[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '.' };
	char periodCount = 0;
	for (auto i = 0; i < sizec; ++i)
	{
		bool isNum = false;
		for (int j = 0; j < 11; ++j)
		{
			if (buffer[i] == numbers[j])
			{
				if (j == 10) periodCount++;
				isNum = true;
				j = 11;
			}
		}
		if (isNum == false || periodCount > 1)
			return false;
	}
	return true;
}

bool CString::isAlphaNumeric() const
{
	return onlyContains("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
}

/*
	Operators
*/
CString& CString::operator=(const CString& pString)
{
	if (this == &pString)
		return *this;

	if (pString.length() != 0)
	{
		clear(pString.length() + 10);
		write((char*)pString.text(), pString.length());
	}
	else clear(30);
	return *this;
}

CString& CString::operator<<(const CString& pString)
{
	if (this == &pString)
		return *this << CString(pString);

	write(pString.text(), pString.length());
	return *this;
}

CString& CString::operator+=(const CString& pString)
{
	return *this << pString;
}

CString CString::operator+(const CString& pString)
{
	return CString(*this) << pString;
}

bool operator==(const CString& pString1, const CString& pString2)
{
	if (pString1.length() == pString2.length())
	{
		if (memcmp(pString1.text(), pString2.text(), pString1.length()) == 0)
			return true;
	}

	return false;
}

bool operator==(const CString& pString1, const std::string& pString2)
{
	if (pString1.length() == pString2.length())
	{
		if (memcmp(pString1.text(), pString2.data(), pString1.length()) == 0)
			return true;
	}

	return false;
}

bool operator==(const CString& pString1, const std::string_view& pString2)
{
	if (pString1.length() == pString2.length())
	{
		if (memcmp(pString1.text(), pString2.data(), pString1.length()) == 0)
			return true;
	}

	return false;
}

bool operator==(const CString& pString1, const char* pString2)
{
	if (pString1.length() == strlen(pString2))
	{
		if (memcmp(pString1.text(), pString2, pString1.length()) == 0)
			return true;
	}

	return false;
}

bool operator!=(const CString& pString1, const CString& pString2)
{
	return !(pString1 == pString2);
}

bool operator!=(const CString& pString1, const std::string& pString2)
{
	return !(pString1 == pString2);
}

bool operator!=(const CString& pString1, const std::string_view& pString2)
{
	return !(pString1 == pString2);
}

bool operator!=(const CString& pString1, const char* pString2)
{
	return !(pString1 == pString2);
}

bool operator<(const CString& pString1, const CString& pString2)
{
	if (strcmp(pString1.text(), pString2.text()) < 0) return true;
	return false;
	/*
	int len = (pString1.length() > pString2.length() ? pString2.length() : pString1.length());
	return memcmp(pString1.text(), pString2.text(), len) < 0;
	*/
}

bool operator>(const CString& pString1, const CString& pString2)
{
	int len = (pString1.length() > pString2.length() ? pString2.length() : pString1.length());
	return memcmp(pString1.text(), pString2.text(), len) > 0;
}

bool operator<=(const CString& pString1, const CString& pString2)
{
	int len = (pString1.length() > pString2.length() ? pString2.length() : pString1.length());
	return memcmp(pString1.text(), pString2.text(), len) <= 0;
}

bool operator>=(const CString& pString1, const CString& pString2)
{
	int len = (pString1.length() > pString2.length() ? pString2.length() : pString1.length());
	return memcmp(pString1.text(), pString2.text(), len) >= 0;
}

CString operator+(const CString& pString1, const CString& pString2)
{
	return CString(pString1) << pString2;
}

/*
	Additional Functions for Data-Packing
*/
CString& CString::writeChar(const char pData, bool nullTerminate)
{
	write((char*)&pData, 1, nullTerminate);
	return *this;
}

CString& CString::writeShort(const short pData, bool nullTerminate)
{
	char val[2];
	val[0] = ((pData >> 8) & 0xFF);
	val[1] = (pData & 0xFF);
	write((char*)&val, 2, nullTerminate);
	return *this;
}

CString& CString::writeInt(const int pData, bool nullTerminate)
{
	char val[4];
	val[0] = ((pData >> 24) & 0xFF);
	val[1] = ((pData >> 16) & 0xFF);
	val[2] = ((pData >> 8) & 0xFF);
	val[3] = (pData & 0xFF);
	write((char *)&val, 4, nullTerminate);
	return *this;
}


CString& CString::writeInt3(const int pData, bool nullTerminate)
{
	unsigned int t = pData;

	unsigned char val[3];

	val[0] = t >> 0x10;
	val[1] = t >> 0x8;
	val[2] = t;

	write((char *)&val,3, nullTerminate);

	return *this;
}

char CString::readChar()
{
	char val;
	read(&val, 1);
	return val;
}

short CString::readShort()
{
	unsigned char val[2];
	read((char*)val, 2);
	return (val[0] << 8) + val[1];
}

int CString::readInt()
{
	unsigned char val[4];
	read((char*)val, 4);
	return (val[0] << 24) + (val[1] << 16) + (val[2] << 8) + val[3];
}

/*
	Additional Functions for Data-Packing (GRAAL)
*/
CString& CString::writeGChar(const unsigned char pData)
{
	unsigned char val = (pData < 223 ? pData : 223) + 32;
	write((char*)&val, 1);
	return *this;
}

CString& CString::writeGCharUnsafe(const unsigned char pData)
{
	if (pData == 233)
		throw std::runtime_error("Tried to Graal-pack byte value 233, which will evaluate to a newline.");

	unsigned char val = pData + 32;
	write((char*)&val, 1);
	return *this;
}

CString& CString::writeGShort(const unsigned short pData)
{
	unsigned short t = pData;
	if (t > 28767) t = 28767;

	unsigned char val[2];
	val[0] = t >> 7;
	if (val[0] > 223) val[0] = 223;
	val[1] = t - (val[0] << 7);

	val[0] += 32;
	val[1] += 32;
	write((char*)&val, 2);

	return *this;
}

CString& CString::writeGInt(const unsigned int pData)
{
	unsigned int t = pData;
	if (t > 3682399) t = 3682399;

	unsigned char val[3];
	val[0] = t >> 14;
	if (val[0] > 223) val[0] = 223;
	t -= val[0] << 14;
	val[1] = t >> 7;
	if (val[1] > 223) val[1] = 223;
	val[2] = t - (val[1] << 7);

	for (int a = 0; a < 3; ++a) val[a] += 32;
	write((char *)&val,3);

	return *this;
}

CString& CString::writeGInt4(const unsigned int pData)
{
	unsigned int t = pData;
	if (t > 471347295) t = 471347295;

	unsigned char val[4];
	val[0] = t >> 21;
	if (val[0] > 223) val[0] = 223;
	t -= val[0] << 21;
	val[1] = t >> 14;
	if (val[1] > 223) val[1] = 223;
	t -= val[1] << 14;
	val[2] = t >> 7;
	if (val[2] > 223) val[2] = 223;
	val[3] = t - (val[2] << 7);

	for (int a = 0; a < 4; ++a) val[a] += 32;
	write((char *)&val,4);

	return *this;
}

CString& CString::writeGInt5(const unsigned long long pData)
{
	unsigned long long t = pData;
	if (t > 0xFFFFFFFF) t = 0xFFFFFFFF;

	unsigned char val[5];

	val[0] = (t >> 28) & 0x0F;
	t -= ((unsigned long long)val[0] << 28);
	val[1] = (t >> 21) & 0x7F;
	if (val[1] > 223) val[1] = 223;
	t -= ((unsigned long long)val[1] << 21);
	val[2] = (t >> 14) & 0x7F;
	if (val[2] > 223) val[2] = 223;
	t -= ((unsigned long long)val[2] << 14);
	val[3] = (t >> 7) & 0x7F;
	if (val[3] > 223) val[3] = 223;
	val[4] = (t - ((unsigned long long)val[3] << 7)) & 0x7F;

	for (int a = 0; a < 5; ++a) val[a] += 32;
	write((char *)&val,5);

	return *this;
}

// max: 0xDF 223
char CString::readGChar()
{
	char val;
	read(&val, 1);
	return val - 32;
}

// max: 0x705F 28767
short CString::readGShort()
{
	// 0x1020 = (32 << 7) + 32
	// Instead of adding every single individual byte, we can just do a single add at the end.
	unsigned char val[2];
	read((char*)val, 2);
	return (val[0] << 7) + val[1] - 0x1020;
}

// max: 0x38305F 3682399
int CString::readGInt()
{
	unsigned char val[3];
	read((char*)val, 3);
	return (((val[0] << 7) + val[1]) << 7) + val[2] - 0x81020;
}

// max: 0x1C18305F 471347295
int CString::readGInt4()
{
	unsigned char val[4];
	read((char*)val, 4);
	return (((((val[0] << 7) + val[1]) << 7) + val[2]) << 7) + val[3] - 0x4081020;
}

// max: 0xFFFFFFFF 4294967295
// The client stores a full uint32_t.
unsigned int CString::readGInt5()
{
	unsigned char val[5];
	read((char*)val, 5);
	return ((((((static_cast<unsigned int>((val[0]) << 7) + val[1]) << 7) + val[2]) << 7) + val[3]) << 7) + val[4] - 0x4081020;
}

/*
	Friend Functions
*/
CString getExtension(const CString& pStr)
{
	int pos = pStr.findl('.');
	if (pos >= 0)
		return pStr.subString(pos);
	return {};
}

CString getPath(const CString& pStr, char separator)
{
	int pos = pStr.findl(separator);
	if (pos >= 0)
		return pStr.subString(0,pos+1);
	return {};
}


CString getFilename(const CString& pStr, char separator)
{
	int pos = pStr.findl(separator);
	if (pos >= 0)
		return pStr.subString(pos+1);
	return pStr;
}

uint32_t calculateCrc32Checksum(const CString& pStr)
{
	uint32_t checksum = crc32(0L, Z_NULL, 0);
	checksum = crc32(checksum, (const uint8_t *)pStr.text(), pStr.length());
	return checksum;
}

#if defined(WOLFSSL_ENABLED)
CString& CString::sha1I()
{
	*this = sha1();
	return *this;
}

CString& CString::rc4_encryptI(const char * key, int keylen)
{
	*this = rc4_encrypt(key, keylen);
	return *this;
}

CString& CString::rc4_decryptI(const char * key, int keylen)
{
	*this = rc4_decrypt(key, keylen);
	return *this;
}
#endif

CString& CString::encodesimpleI(unsigned int buffSize)
{
	*this = encodesimple(buffSize);
	return *this;
}

CString& CString::codesimplefix0I(unsigned int buffSize)
{
	*this = codesimplefix0(buffSize);
	return *this;
}

CString& CString::decodesimpleI(unsigned int buffSize, bool includeNullTermination)
{
	*this = decodesimple(buffSize, includeNullTermination);
	return *this;
}

CString& CString::base64encodeI()
{
	*this = base64encode();
	return *this;
}

CString& CString::base64decodeI()
{
	*this = base64decode();
	return *this;
}

CString& CString::bzcompressI(unsigned int buffSize)
{
	*this = bzcompress(buffSize);
	return *this;
}

CString& CString::bzuncompressI(unsigned int buffSize)
{
	*this = bzuncompress(buffSize);
	return *this;
}

CString& CString::zcompressI(unsigned int buffSize)
{
	*this = zcompress(buffSize);
	return *this;
}

CString& CString::zuncompressI(unsigned int buffSize)
{
	*this = zuncompress(buffSize);
	return *this;
}
