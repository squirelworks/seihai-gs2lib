// Modified by the Seihai project, 2026-10-31: see CHANGES-SEIHAI.md.
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

#include <algorithm>
#include <array>
#include <cstdint>

#include <CString.h>

enum
{
    ENCRYPT_GEN_1 = 0, // No encryption/no compression.
    ENCRYPT_GEN_2 = 1, // No encryption/zlib compression.
    ENCRYPT_GEN_3 = 2, // Single byte insertion/zlib compression.
    ENCRYPT_GEN_4 = 3, // Partial packet encryption/bz2 compression.
    ENCRYPT_GEN_5 = 4, // Partial packet encryption/none, zlib, bz2 compression methods.
    ENCRYPT_GEN_6 = 5, // Graal v6 EILLLT.
};

// Seihai: how often the gen-5 iterator steps on a body.
// Every client steps it once per started 4-byte group (rounded up), up to the
// limit for the compression type. 5.12 steps it once per whole group only
// (rounded down) on zlib and bz2 bodies, and leaves a partial last group as it
// is; its uncompressed bodies step as everyone else's do.
enum class Gen5Steps : uint8_t
{
    PerStartedGroup, // Round up: every client but 5.12.
    PerWholeGroup,   // Round down on zlib and bz2 bodies: 5.12.
};

enum
{
    COMPRESS_UNCOMPRESSED = 0x02,
    COMPRESS_ZLIB = 0x04,
    COMPRESS_BZ2 = 0x06,
};

class CEncryption
{
public:
    CEncryption();

    void reset(uint8_t key);
    void decrypt(CString& pBuf);
    CString encrypt(CString pBuf);

    void limit(int32_t limit);
    int limitFromType(uint8_t type);

    void setGen(uint32_t gen);
    [[nodiscard]] uint32_t getGen() const noexcept;

    // Seihai: the gen-5 step rule. reset() keeps it.
    void setGen5Steps(Gen5Steps steps) noexcept { m_gen5Steps = steps; }
    [[nodiscard]] Gen5Steps getGen5Steps() const noexcept { return m_gen5Steps; }

    // Seihai: the step rule a client version id (CLVER_*) uses.
    [[nodiscard]] static Gen5Steps gen5StepsForClient(int versionId) noexcept;

    // When 2.x clients send a broken zlib packet, we need to rollback the iterator one value so the next packet is decrypted properly.
    // For some reason, the client falls 1 iteration behind.
    void rollbackIterator() { m_iterator = m_lastIterator; }

private:
    uint8_t m_key;
    uint32_t m_iterator;
    int32_t m_limit;
    uint32_t m_gen;
    uint32_t m_lastIterator;
    Gen5Steps m_gen5Steps = Gen5Steps::PerStartedGroup;
    bool m_compressed = false; // The last limitFromType() was zlib or bz2.

    void applyGen5Steps(int32_t length);
};

inline void CEncryption::setGen(const uint32_t gen)
{
    m_gen = std::clamp(gen, 0U, 6U);
}

inline uint32_t CEncryption::getGen() const noexcept
{
    return m_gen;
}

#endif
