// Modified by the Seihai project, 2026-10-31: see CHANGES-SEIHAI.md.
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

#include <CEncryption.h>
#include <IUtil.h>

static constexpr std::array ITERATOR_START = {0U, 0U, 0x04A80B38U, 0x4A80B38U, 0x4A80B38U, 0U};

CEncryption::CEncryption()
    : m_key(0), m_limit(-1), m_gen(ENCRYPT_GEN_3)
{
    m_lastIterator = m_iterator = ITERATOR_START[m_gen];
}

void CEncryption::reset(const uint8_t key)
{
    m_key = key;
    m_lastIterator = m_iterator = ITERATOR_START[m_gen];
    m_limit = -1;
}

void CEncryption::decrypt(CString& pBuf)
{
    // If we don't have anything, just return.
    if (pBuf.isEmpty())
        return;

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
            applyGen5Steps(pBuf.length());
            const auto iterator = reinterpret_cast<const uint8_t*>(&m_iterator);

            for (int32_t i = 0; i < pBuf.length(); ++i)
            {
                if (i % 4 == 0)
                {
                    if (m_limit == 0)
                        return;
                    m_lastIterator = m_iterator;
                    m_iterator *= 0x8088405;
                    m_iterator += m_key;
                    if (m_limit > 0)
                        m_limit--;
                }

                pBuf[i] = static_cast<char>(pBuf[i] ^ iterator[i % 4]);
            }
        }
        break;

        // Future encryption method.
        case ENCRYPT_GEN_6:
        default:
            break;
    }
}

CString CEncryption::encrypt(CString pBuf)
{
    // If we don't have anything, just return.
    if (pBuf.isEmpty())
        return pBuf;

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
            applyGen5Steps(pBuf.length());
            const auto iterator = reinterpret_cast<const uint8_t*>(&m_iterator);

            for (int32_t i = 0; i < pBuf.length(); ++i)
            {
                if (i % 4 == 0)
                {
                    if (m_limit == 0)
                        return pBuf;
                    m_lastIterator = m_iterator;
                    m_iterator *= 0x8088405;
                    m_iterator += m_key;
                    if (m_limit > 0)
                        m_limit--;
                }

                pBuf[i] = static_cast<char>(pBuf[i] ^ iterator[i % 4]);
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
    m_compressed = false;
}

// Seihai: with the round-down rule, a zlib or bz2 body steps once per whole
// 4-byte group only, still at most the type's limit. A body shorter than
// 4 bytes is then not XORed at all and does not step the iterator.
void CEncryption::applyGen5Steps(const int32_t length)
{
    if (m_gen != ENCRYPT_GEN_5 || m_gen5Steps != Gen5Steps::PerWholeGroup || !m_compressed)
        return;

    const int32_t groups = length / 4;
    if (m_limit < 0 || groups < m_limit)
        m_limit = groups;
}

Gen5Steps CEncryption::gen5StepsForClient(const int versionId) noexcept
{
    return versionId == CLVER_5_12 ? Gen5Steps::PerWholeGroup : Gen5Steps::PerStartedGroup;
}

int CEncryption::limitFromType(const uint8_t type)
{
    m_compressed = (type == COMPRESS_ZLIB || type == COMPRESS_BZ2);

    // { type, limit, type2, limit2, ... }
    static int limits[] = {0x02, 0x0C, 0x04, 0x04, 0x06, 0x04};
    for (unsigned int i = 0; i < sizeof(limits) / sizeof(int); i += 2)
    {
        // If we found a valid type, set the limit.
        if (limits[i] == type)
        {
            m_limit = (int32_t)limits[i + 1];
            return 0;
        }
    }

    // Error.
    return 1;
}
