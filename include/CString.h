/*
 * include/CString.h
 *
 * Copyright 2019 OpenGraal
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

#ifndef CSTRING_H
#define CSTRING_H

/* The following line work around this compiling error:
CSocket.cpp
In file included from ../../server/include/CString.h:7:0,
                 from ../../server/include/CLog.h:6,
                 from ../../server/src/CSocket.cpp:71:
c:\mingw\bin\../lib/gcc/mingw32/4.6.2/include/c++/cstdio:166:11: erreur: '::snprintf' has not been declared
*/

#define _WINSOCKAPI_
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

/*
#include "bzlib.h"
#include "zlib.h"
*/

#ifndef MAX
	#define MAX(a, b)	((a) > (b) ? (a) : (b))
	#define MIN(a, b)	((a) < (b) ? (a) : (b))
#endif
#define clip(a, b, c) MAX(MIN((a), (c)), (b))
#define inrange(a, b, c) ((a) >= (b) && (a) <= (c))

#define strtofloat(a) std::strtof(a.text(), nullptr) //atof(a.text())
#define strtoint(a) atoi(a.text())
#define strtolong(a) atol(a.text())

const int INTERNAL_BUFFER_SIZE = 32;

class CString
{
	public:
		/* Constructor ~ Deconstructor */
		CString();
		CString(const char *pString);
		CString(const CString& pString);
		CString(const std::string& pString);
		CString(char value);
		CString(double value);
		CString(float value);
		CString(int value);
		CString(unsigned int value);
		CString(long value);
		CString(unsigned long value);
		CString(long long value);
		CString(unsigned long long value);
		~CString();

		/*
		* Move Semantics
		*/
		CString(CString&& o) noexcept;
		CString& operator= (CString&& o) noexcept;

		/* Retrieve Information */
		bool isEmpty() const;
		char * text();
		const char * text() const;
		std::string toString() const;
		int bytesLeft() const;
		int length() const;
		int readPos() const;
		int writePos() const;
		void setRead(int pRead);
		void setSize(int pSize);
		void setWrite(int pWrite);

		/* Data-Management */
		bool load(const CString& pString);
		bool save(const CString& pString) const;
		CString readChars(int pLength);
		CString readString(const CString& pString = " ");
		int read(char *pDest, int pSize);
		int write(unsigned char *pSrc, int pSize, bool nullTerminate = true);
		int write(const char *pSrc, int pSize, bool nullTerminate = true);
		int write(const CString& pString);
		void clear(size_t size = 30);
		void resize(size_t size);

		/* Functions */
		CString escape() const;
		CString unescape() const;
		CString left(int pLength) const;
		CString right(int pLength) const;
		CString remove(int pStart, int pLength = -1) const;
		CString removeAll(const CString& pString) const;
		CString subString(int pStart, int pLength = -1) const;
		CString toLower() const;
		CString toUpper() const;
		CString trim() const;
		CString trimLeft() const;
		CString trimRight() const;

#if defined(WOLFSSL_ENABLED)
		CString sha1() const;
		CString rc4_encrypt(const char * key, int keylen) const;
		CString rc4_decrypt(const char * key, int keylen) const;
#endif
		CString decodesimple(unsigned int buffSize = 65536, bool includeNullTermination = false) const;
		CString codesimplefix0(unsigned int buffSize = 65536) const;
		CString encodesimple(unsigned int buffSize = 65536) const;
		static std::string formatBase64(std::size_t num, std::size_t len = 2);
		CString base64encode() const;
		CString base64decode() const;
		CString bzcompress(unsigned int buffSize = 65536) const;
		CString bzuncompress(unsigned int buffSize = 65536) const;
		CString zcompress(unsigned int buffSize = 65536) const;
		CString zuncompress(unsigned int buffSize = 65536) const;

		int find(const CString& pString, int pStart = 0) const;
		int findi(const CString& pString, int pStart = 0) const;
		int findl(char pChar) const;
		std::vector<CString> tokenize(const CString& pString = " ", bool keepEmpty = false) const;
		std::vector<CString> tokenizeConsole() const;
		static std::vector<CString> loadToken(const CString& pFile, const CString& pToken = "\n", bool removeCR = false);
		static CString join(std::vector<CString>& rows, const CString& pToken = "\n");
		CString replaceAll(const CString& pString, const CString& pNewString) const;
		CString gtokenize() const;
		CString guntokenize() const;
		std::vector<CString> gCommaStrTokens() const;
		bool match(const CString& pMask) const;
		bool comparei(const CString& pOther) const;
		bool contains(const CString& characters) const;
		bool onlyContains(const CString& characters) const;
		bool isNumber() const;
		bool isAlphaNumeric() const;

		/* In-Functions */
		inline CString& escapeI();
		inline CString& unescapeI();
		inline CString& removeI(int pStart, int pLength = -1);
		inline CString& removeAllI(const CString& pString);
		inline CString& toLowerI();
		inline CString& toUpperI();
		inline CString& trimI();

#if defined(WOLFSSL_ENABLED)
		CString& sha1I();
		CString& rc4_encryptI(const char * key, int keylen);
		CString& rc4_decryptI(const char * key, int keylen);
#endif
		CString& encodesimpleI(unsigned int buffSize = 65536);
		CString& codesimplefix0I(unsigned int buffSize = 65536);
		CString& decodesimpleI(unsigned int buffSize = 65536, bool includeNullTermination = false);
		CString& base64encodeI();
		CString& base64decodeI();
		CString& bzcompressI(unsigned int buffSize = 65536);
		CString& bzuncompressI(unsigned int buffSize = 65536);
		CString& zcompressI(unsigned int buffSize = 65536);
		CString& zuncompressI(unsigned int buffSize = 65536);

		inline CString& replaceAllI(const CString& pString, const CString& pNewString);
		inline CString& gtokenizeI();
		inline CString& guntokenizeI();

		/* Operators */
		const char& operator[](const int& pIndex) const;
		char& operator[](const int& pIndex);

		CString& operator=(const CString& pString);
		CString& operator<<(const CString& pString);
		CString& operator+=(const CString& pString);
		CString operator+(const CString& pString);
		//operator const char*() const;

		friend bool operator==(const CString& pString1, const CString& pString2);
		friend bool operator!=(const CString& pString1, const CString& pString2);
		friend bool operator<(const CString& pString1, const CString& pString2);
		friend bool operator>(const CString& pString1, const CString& pString2);
		friend bool operator<=(const CString& pString1, const CString& pString2);
		friend bool operator>=(const CString& pString1, const CString& pString2);
		friend CString operator+(const CString& pString1, const CString& pString2);

		/* Data-Packing Functions */
		CString& operator>>(const char pData);
		CString& operator>>(const short pData);
		CString& operator>>(const int pData);
		CString& operator>>(const long long pData);
		CString& operator<<(const char pData);
		CString& operator<<(const short pData);
		CString& operator<<(const int pData);

		CString& writeChar(const char pData, bool nullTerminate = true);
		CString& writeShort(const short pData, bool nullTerminate = true);
		CString& writeInt(const int pData, bool nullTerminate = true);
		CString& writeInt3(const int pData, bool nullTerminate = true);
		char readChar();
		short readShort();
		int readInt();

		// Graal Packing <.<.
		CString& writeGChar(const unsigned char pData);
		CString& writeGShort(const unsigned short pData);
		CString& writeGInt(const unsigned int pData);
		CString& writeGInt4(const unsigned int pData);
		CString& writeGInt5(const unsigned long long pData);
		char readGChar();
		short readGShort();
		int readGInt();
		int readGInt4();
		unsigned int readGInt5();

		inline unsigned char readGUChar();
		inline unsigned short readGUShort();
		inline unsigned int readGUInt();
		inline unsigned int readGUInt4();
		inline unsigned int readGUInt5();

	protected:
		char *buffer;
		int buffc, sizec, readc, writec;
		char * _heapBuffer;
		char _internalBuffer[INTERNAL_BUFFER_SIZE];
	private:
		static const char BASE64[];
};

/*
	Inline Functions
*/

inline CString::CString() : _heapBuffer(0), buffer(_internalBuffer), buffc(30), sizec(0), readc(0), writec(0)
{
	_internalBuffer[0] = 0;
}

inline CString::CString(char pChar) : _heapBuffer(0), buffer(_internalBuffer), buffc(30), sizec(1), readc(0), writec(1)
{
	_internalBuffer[0] = pChar;
	_internalBuffer[1] = 0;
}

inline CString::CString(CString&& o) noexcept : _heapBuffer(nullptr)
{
	*this = std::move(o);
}

inline CString& CString::operator=(CString&& o) noexcept
{
	// copy internal buffer if its being used
	if (!o._heapBuffer)
	{
		memcpy(_internalBuffer, o._internalBuffer, o.buffc);
		buffer = _internalBuffer;
	}
	else buffer = o._heapBuffer;

	// swap both states
	std::swap(_heapBuffer, o._heapBuffer);
	buffc = o.buffc;
	sizec = o.sizec;
	readc = o.readc;
	writec = o.writec;
	o.clear(30);

	return *this;
}

inline CString::~CString()
{
	if (_heapBuffer)
	{
		free(_heapBuffer);
	}
}

inline bool CString::isEmpty() const
{
	return (length() < 1);
}

inline const char& CString::operator[](const int& idx) const
{
	return buffer[idx];
}

inline char& CString::operator[](const int& idx)
{
	return buffer[idx];
}

inline int CString::write(const CString& pString)
{
	return write(pString.text(), pString.length());
}

inline CString& CString::operator>>(const char pData)
{
	return writeGChar(pData);
}

inline CString& CString::operator>>(const short pData)
{
	return writeGShort(pData);
}

inline CString& CString::operator>>(const int pData)
{
	return writeGInt(pData);
}

inline CString& CString::operator>>(const long long pData)
{
	return writeGInt5(pData);
}

inline CString& CString::operator<<(const char pData)
{
	return writeChar(pData);
}

inline CString& CString::operator<<(const short pData)
{
	return writeShort(pData);
}

inline CString& CString::operator<<(const int pData)
{
	return writeInt(pData);
}

inline char * CString::text()
{
	return buffer;
}

inline const char * CString::text() const
{
	return buffer;
}

inline std::string CString::toString() const
{
	return std::string(text(), length());
}

inline int CString::bytesLeft() const
{
	return std::max(0, length()-readPos());
}

inline int CString::length() const
{
	return sizec;
}

inline int CString::readPos() const
{
	return readc;
}

inline int CString::writePos() const
{
	return writec;
}

inline void CString::setRead(int pRead)
{
	readc = std::clamp(pRead, 0, sizec);
}

inline void CString::setSize(int pSize)
{
	sizec = std::max(0, pSize);
	readc = std::min(readc, sizec);
	writec = std::min(writec, sizec);
}

inline void CString::setWrite(int pWrite)
{
	writec = std::clamp(pWrite, 0, sizec);
}

inline unsigned char CString::readGUChar()
{
	return (unsigned char)readGChar();
}

inline unsigned short CString::readGUShort()
{
	return (unsigned short)readGShort();
}

inline unsigned int CString::readGUInt()
{
	return (unsigned int)readGInt();
}

inline unsigned int CString::readGUInt4()
{
	return (unsigned int)readGInt4();
}

inline unsigned int CString::readGUInt5()
{
	return (unsigned int)readGInt5();
}

/*
	Inline Inside-Functions
*/
inline CString& CString::escapeI()
{
	*this = escape();
	return *this;
}

inline CString& CString::unescapeI()
{
	*this = unescape();
	return *this;
}

inline CString& CString::removeI(int pStart, int pLength)
{
	*this = remove(pStart, pLength);
	return *this;
}

inline CString& CString::removeAllI(const CString& pString)
{
	*this = removeAll(pString);
	return *this;
}

inline CString& CString::toLowerI()
{
	*this = toLower();
	return *this;
}

inline CString& CString::toUpperI()
{
	*this = toUpper();
	return *this;
}

inline CString& CString::trimI()
{
	*this = trim();
	return *this;
}

inline CString& CString::replaceAllI(const CString& pString, const CString& pNewString)
{
	*this = replaceAll(pString, pNewString);
	return *this;
}

inline CString& CString::gtokenizeI()
{
	*this = gtokenize();
	return *this;
}

inline CString& CString::guntokenizeI()
{
	*this = guntokenize();
	return *this;
}

/*
	Friend Functions
*/
CString getExtension(const CString& pStr);
CString getPath(const CString& pStr, char separator =
#ifdef WIN32
'\\'
#else
'/'
#endif
);
CString getFilename(const CString& pStr, char separator =
#ifdef WIN32
'\\'
#else
'/'
#endif
);
uint32_t calculateCrc32Checksum(const CString& pStr);

#endif // CSTRING_H
