/*
 * include/CEncryption.cpp
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
#include "CEncryption.h"

const uint32_t CEncryption::ITERATOR_START[6] = {0, 0, 0x04A80B38, 0x4A80B38, 0x4A80B38, 0};

CEncryption::CEncryption()
: m_key(0), m_limit(-1), m_gen(ENCRYPT_GEN_3)
{
	m_lastIterator = m_iterator = ITERATOR_START[m_gen];
}

void CEncryption::reset(const uint8_t key) {
	m_key = key;
	m_lastIterator = m_iterator = ITERATOR_START[m_gen];
	m_limit = -1;
}

void CEncryption::decrypt(CString& pBuf) {
	// If we don't have anything, just return.
	if (pBuf.isEmpty()) return;

	// Apply the correct decryption algorithm.
	switch (m_gen)
	{
		// No encryption.
		case ENCRYPT_GEN_1:
		case ENCRYPT_GEN_2:
			return;

		// Single byte insertion/zlib compression.
		case ENCRYPT_GEN_3:
		{
			m_lastIterator = m_iterator;
			m_iterator *= 0x8088405;
			m_iterator += m_key;
			const auto pos = ((m_iterator & 0x0FFFF) % pBuf.length());
			pBuf.removeI(static_cast<int>(pos), 1);
		}
		break;

		// Partial packet encryption/none, zlib, bz2 compression methods.
		// Gen 4 is only bz2.
		case ENCRYPT_GEN_4:
		case ENCRYPT_GEN_5:
		{
			const auto iterator = reinterpret_cast<const uint8_t*>(&m_iterator);

			for (int32_t i = 0; i < pBuf.length(); ++i)
			{
				if (i % 4 == 0)
				{
					if (m_limit == 0) return;
					m_lastIterator = m_iterator;
					m_iterator *= 0x8088405;
					m_iterator += m_key;
					if (m_limit > 0) m_limit--;
				}

				pBuf[i] = static_cast<char>(pBuf[i] ^ iterator[i%4]);
			}
		}
		break;

		// Future encryption method.
		case ENCRYPT_GEN_6:
		default:
			return;
	}
}

CString CEncryption::encrypt(CString pBuf)
{
	// If we don't have anything, just return.
	if (pBuf.isEmpty()) return pBuf;

	switch (m_gen)
	{
		// No encryption.
		case ENCRYPT_GEN_1:
		case ENCRYPT_GEN_2:
			break;

		// Single byte insertion.
		case ENCRYPT_GEN_3:
		{
			m_lastIterator = m_iterator;
			m_iterator *= 0x8088405;
			m_iterator += m_key;
			const auto pos = ((m_iterator & 0x0FFFF) % pBuf.length());
			return CString() << pBuf.subString(0, static_cast<int>(pos)) << ")" << pBuf.subString(static_cast<int>(pos));
			break;
		}

		// Partial packet encryption/none, zlib, bz2 compression methods.
		// Gen 4 is only bz2.
		case ENCRYPT_GEN_4:
		case ENCRYPT_GEN_5:
		{
			const auto iterator = reinterpret_cast<const uint8_t*>(&m_iterator);

			for (int32_t i = 0; i < pBuf.length(); ++i)
			{
				if (i % 4 == 0)
				{
					if (m_limit == 0) return pBuf;
					m_lastIterator = m_iterator;
					m_iterator *= 0x8088405;
					m_iterator += m_key;
					if (m_limit > 0) m_limit--;
				}

				pBuf[i] = static_cast<char>(pBuf[i] ^ iterator[i%4]);
			}
			return pBuf;
			break;
		}

		default:;
	}
	return pBuf;
}

void CEncryption::limit(const int32_t limit)
{
	m_limit = limit;
}

int CEncryption::limitFromType(const uint8_t type)
{
	// { type, limit, type2, limit2, ... }
	static int limits[] = { 0x02, 0x0C, 0x04, 0x04, 0x06, 0x04 };
	for (unsigned int i = 0; i < sizeof(limits) / sizeof(int); i+=2)
	{
		// If we found a valid type, set the limit.
		if (limits[i] == type)
		{
			m_limit = (int32_t)limits[i+1];
			return 0;
		}
	}

	// Error.
	return 1;
}
