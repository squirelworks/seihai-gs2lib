/*
 * include/CEncryption.h
 *
 * Copyright 2008-2019 OpenGraal
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

#ifndef ENC_CODEC_HPP
#define ENC_CODEC_HPP

#include <cstdint>
#include "CString.h"

enum
{
	ENCRYPT_GEN_1		= 0,	// No encryption/no compression.
	ENCRYPT_GEN_2		= 1,	// No encryption/zlib compression.
	ENCRYPT_GEN_3		= 2,	// Single byte insertion/zlib compression.
	ENCRYPT_GEN_4		= 3,	// Partial packet encryption/bz2 compression.
	ENCRYPT_GEN_5		= 4,	// Partial packet encryption/none, zlib, bz2 compression methods.
	ENCRYPT_GEN_6		= 5,	// Unknown (Graal v6 encryption).
};

enum
{
	COMPRESS_UNCOMPRESSED	= 0x02,
	COMPRESS_ZLIB			= 0x04,
	COMPRESS_BZ2			= 0x06,
};

class CEncryption
{
	public:
		CEncryption();

		static const uint32_t ITERATOR_START[6];
		void reset(uint8_t key);
		void decrypt(CString& pBuf);
		CString encrypt(CString pBuf);
		void limit(int32_t limit);
		int limitFromType(uint8_t type);
		void setGen(uint32_t gen)		{ m_gen = gen; if (m_gen > 6) m_gen = 6; }
		uint32_t getGen()				{ return m_gen; }

	private:
		uint8_t		m_key;
		uint32_t	m_iterator;
		int32_t		m_limit;
		uint32_t	m_gen;
};

#endif
