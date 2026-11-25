// Modified by the Seihai project, 2026-10-31, 2026-11-15, 2026-11-25: see CHANGES-SEIHAI.md.
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

/// @brief The generation of encryption.
enum class EncryptionGeneration
{
	/// @brief No encryption, no compression.
	GEN_1 = 0,

	/// @brief No encryption, zlib compression.
	GEN_2 = 1,

	/// @brief Single byte insertion, zlib compression.
	GEN_3 = 2,

	/// @brief Partial packet encryption, bz2 compression.
	GEN_4 = 3,

	/// @brief Partial packet encryption, configurable compression: none, zlib, bz2.
	GEN_5 = 4,

	/// @brief No encryption, configurable compression: none, zlib, bz2.
	GEN_6 = 5,

	COUNT
};
inline constexpr uint8_t EncryptionGenerationCount = static_cast<uint8_t>(EncryptionGeneration::COUNT);

enum class EncryptionCompression
{
	UNCOMPRESSED = 0x02,
	ZLIB = 0x04,
	BZ2 = 0x06,
};

// Seihai: the pre-2026-11-20 names, kept so code written against them still builds.
// New code uses EncryptionGeneration and EncryptionCompression.
enum
{
	ENCRYPT_GEN_1 = 0,
	ENCRYPT_GEN_2 = 1,
	ENCRYPT_GEN_3 = 2,
	ENCRYPT_GEN_4 = 3,
	ENCRYPT_GEN_5 = 4,
	ENCRYPT_GEN_6 = 5,
};

enum
{
	COMPRESS_UNCOMPRESSED = 0x02,
	COMPRESS_ZLIB = 0x04,
	COMPRESS_BZ2 = 0x06,
};

// Seihai: how often the gen-5 iterator steps on a zlib or bz2 body. Upstream's
// allowPartialIteration flag is the same switch: PerStartedGroup is true, PerWholeGroup
// is false. Every gen-5 build that has been disassembled rounds down (see
// gen5StepsForClient). Uncompressed bodies always step once per started group.
enum class Gen5Steps : uint8_t
{
	PerStartedGroup, // Round up: allowPartialIteration = true.
	PerWholeGroup,   // Round down on zlib and bz2 bodies: allowPartialIteration = false.
};

/// @brief Encrypts and decrypts data.
class CEncryption
{
public:
	CEncryption();

	/// @brief Resets the encryption and configures a new key.
	/// @param key The new key to use.
	void reset(uint8_t key);

	/// @brief Decrypts a buffer.
	/// @param buffer The buffer to decrypt.
	void decrypt(CString& buffer);

	/// @brief Encrypts a buffer.
	/// @param buffer The buffer to encrypt.
	/// @return An unencrypted buffer.
	CString encrypt(CString buffer);

	/// @brief Sets the iteration limit based on the buffer's compression type.
	/// @param type The compression type for the buffer.
	void setIterationLimitFromCompressionType(EncryptionCompression type) noexcept;

	/// Sets the encryption generation.
	/// @param generation The generation to use.
	void setGeneration(EncryptionGeneration generation) noexcept;

	/// Gets the encryption generation.
	/// @return The encryption generation.
	[[nodiscard]] EncryptionGeneration getGeneration() const noexcept;

	/// @brief Rolls back the encryption by one step.
	/// @details When 2.x clients send a broken zlib packet, we need to rollback the iterator one value so the next packet is decrypted properly.
	/// For some reason, the client falls 1 iteration behind.
	void rollbackIterator() noexcept;

	// Seihai: the pre-2026-11-20 interface, on top of the one above.
	void setGen(uint32_t gen) noexcept { setGeneration(static_cast<EncryptionGeneration>(std::min(gen, static_cast<uint32_t>(EncryptionGeneration::GEN_6)))); }
	[[nodiscard]] uint32_t getGen() const noexcept { return static_cast<uint32_t>(m_generation); }
	void limit(int32_t limit) noexcept { iterationLimit = limit; m_compressed = false; }
	int limitFromType(uint8_t type) noexcept;
	void setGen5Steps(Gen5Steps steps) noexcept { allowPartialIteration = (steps == Gen5Steps::PerStartedGroup); }
	[[nodiscard]] Gen5Steps getGen5Steps() const noexcept { return allowPartialIteration ? Gen5Steps::PerStartedGroup : Gen5Steps::PerWholeGroup; }

	// Seihai: the step rule a client version id (CLVER_*) uses.
	[[nodiscard]] static Gen5Steps gen5StepsForClient(int versionId) noexcept;

public:
	/// @brief Maximum number of iterations to perform over a buffer.
	int32_t iterationLimit = -1;

	/// @brief If true, a partial tail (less than 4 bytes) is processed. If false, it is skipped.
	/// @details v4-v5 clients will not process a partial tail on compressed packets, skipping any remaining bytes.
	/// This is not a problem since compression only turns on when the packet is a certain length, which is greater
	/// than the encryption limit (default of 16 bytes of data).  As such, this could only be a problem if you are modifying the server.
	/// Seihai: only zlib and bz2 bodies follow this flag. An uncompressed body always processes its partial tail,
	/// as every client does, so a caller does not have to set the flag around one.
	bool allowPartialIteration = true;

private:
	EncryptionGeneration m_generation = EncryptionGeneration::GEN_3;
	uint8_t m_key = 0;
	uint32_t m_iterator;
	uint32_t m_lastIterator;
	bool m_compressed = false; // Seihai: the last limit was set for a zlib or bz2 body.

	[[nodiscard]] int32_t iterationsFor(int32_t length) const noexcept;
};

inline void CEncryption::setGeneration(const EncryptionGeneration generation) noexcept
{
	// Seihai: generations index the iterator table, GEN_1 through GEN_6. Anything past it is GEN_6
	// (upstream maps COUNT to GEN_5 and passes larger values through unchecked).
	m_generation = std::min(generation, EncryptionGeneration::GEN_6);
}

inline EncryptionGeneration CEncryption::getGeneration() const noexcept
{
	return m_generation;
}

inline void CEncryption::rollbackIterator() noexcept
{
	m_iterator = m_lastIterator;
}

#endif
