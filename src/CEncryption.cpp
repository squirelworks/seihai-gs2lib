// Modified by the Seihai project, 2026-10-31, 2026-11-13, 2026-11-15, 2026-11-21, 2026-11-25: see CHANGES-SEIHAI.md.
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

// One entry per generation, GEN_1 through GEN_6.
static_assert(ITERATOR_START.size() == static_cast<std::size_t>(EncryptionGeneration::COUNT));

static constexpr uint32_t getGenerationInt(const CEncryption* encryption) noexcept
{
	return static_cast<uint32_t>(encryption->getGeneration());
}

CEncryption::CEncryption()
{
	m_lastIterator = m_iterator = ITERATOR_START[getGenerationInt(this)];
}

void CEncryption::reset(const uint8_t key)
{
	m_key = key;
	m_lastIterator = m_iterator = ITERATOR_START[getGenerationInt(this)];
	iterationLimit = -1;
	m_compressed = false;
}

void CEncryption::decrypt(CString& buffer)
{
	// If we don't have anything, just return.
	if (buffer.isEmpty())
		return;

	// Apply the correct decryption algorithm.
	switch (m_generation)
	{
		// No encryption.
		case EncryptionGeneration::GEN_1:
		case EncryptionGeneration::GEN_2:
			return;

		// Single byte insertion/zlib compression.
		case EncryptionGeneration::GEN_3:
		{
			m_lastIterator = m_iterator;
			m_iterator *= 0x8088405;
			m_iterator += m_key;
			const auto pos = ((m_iterator & 0x0FFFF) % buffer.length());
			buffer.removeI(static_cast<int>(pos), 1);
		}
		break;

		// Partial packet encryption/none, zlib, bz2 compression methods.
		// Gen 4 is only bz2.
		case EncryptionGeneration::GEN_4:
		case EncryptionGeneration::GEN_5:
		{
			const auto iterator = reinterpret_cast<const uint8_t*>(&m_iterator);

			// Determine how many iterations we will process.
			const auto iterations = iterationsFor(buffer.length());

			// Process the iterations.
			for (int32_t i = 0; i < iterations; ++i)
			{
				// Increase the iterator.
				m_lastIterator = m_iterator;
				m_iterator *= 0x8088405;
				m_iterator += m_key;

				// Apply to the buffer.
				const auto bufferStart = i * 4;
				const auto bufferEnd = buffer.length() - bufferStart;
				for (int32_t j = 0; j < 4 && j < bufferEnd; ++j)
				{
					buffer[bufferStart + j] = static_cast<char>(buffer[bufferStart + j] ^ iterator[j]);
				}
			}
		}
		break;

		// Future encryption method.
		case EncryptionGeneration::GEN_6:
		default:
			break;
	}
}

CString CEncryption::encrypt(CString buffer)
{
	// If we don't have anything, just return.
	if (buffer.isEmpty())
		return buffer;

	switch (m_generation)
	{
		// No encryption.
		case EncryptionGeneration::GEN_1:
		case EncryptionGeneration::GEN_2:
			break;

		// Single byte insertion.
		case EncryptionGeneration::GEN_3:
		{
			m_lastIterator = m_iterator;
			m_iterator *= 0x8088405;
			m_iterator += m_key;
			const auto pos = ((m_iterator & 0x0FFFF) % buffer.length());
			const auto byte = static_cast<char>((m_iterator >> 16) & 0x7F);
			return CString() << buffer.subString(0, static_cast<int>(pos)) >> byte << buffer.subString(static_cast<int>(pos));
		}

		// Partial packet encryption/none, zlib, bz2 compression methods.
		// Gen 4 is only bz2.
		case EncryptionGeneration::GEN_4:
		case EncryptionGeneration::GEN_5:
		{
			const auto iterator = reinterpret_cast<const uint8_t*>(&m_iterator);

			// Determine how many iterations we will process.
			const auto iterations = iterationsFor(buffer.length());

			// Process the iterations.
			for (int32_t i = 0; i < iterations; ++i)
			{
				// Increase the iterator.
				m_lastIterator = m_iterator;
				m_iterator *= 0x8088405;
				m_iterator += m_key;

				// Apply to the buffer.
				const auto bufferStart = i * 4;
				const auto bufferEnd = buffer.length() - bufferStart;
				for (int32_t j = 0; j < 4 && j < bufferEnd; ++j)
				{
					buffer[bufferStart + j] = static_cast<char>(buffer[bufferStart + j] ^ iterator[j]);
				}
			}
			return buffer;
		}

		// No encryption.
		case EncryptionGeneration::GEN_6:
		default:
			break;
	}
	return buffer;
}

void CEncryption::setIterationLimitFromCompressionType(const EncryptionCompression type) noexcept
{
	constexpr std::array<std::pair<EncryptionCompression, uint8_t>, 3> limits = {{
		{EncryptionCompression::UNCOMPRESSED, 0x0C},
		{EncryptionCompression::ZLIB, 0x04},
		{EncryptionCompression::BZ2, 0x04}
	}};

	for (size_t i = 0; i < limits.size(); ++i)
	{
		if (const auto [compression, limit] = limits[i]; compression == type)
		{
			iterationLimit = limit;
			m_compressed = (type != EncryptionCompression::UNCOMPRESSED);
			return;
		}
	}
}

// Seihai: the rule both directions use. An uncompressed body steps once per started
// 4-byte group; a zlib or bz2 body does too unless allowPartialIteration is off, in which
// case it steps once per whole group and leaves a partial last group as it is. Gen 4
// (always bz2) keeps the round-up rule: no gen-4 build has been read to round down.
// Either way the steps stop at the type's limit.
int32_t CEncryption::iterationsFor(const int32_t length) const noexcept
{
	const bool partial = allowPartialIteration || !m_compressed || m_generation != EncryptionGeneration::GEN_5;
	auto iterations = partial ? (3 + length) / 4 : length / 4;
	if (iterationLimit >= 0)
		iterations = std::min(iterations, iterationLimit);
	return iterations;
}

int CEncryption::limitFromType(const uint8_t type) noexcept
{
	if (type != COMPRESS_UNCOMPRESSED && type != COMPRESS_ZLIB && type != COMPRESS_BZ2)
		return 1;
	setIterationLimitFromCompressionType(static_cast<EncryptionCompression>(type));
	return 0;
}

Gen5Steps CEncryption::gen5StepsForClient(const int versionId) noexcept
{
	// Every Graal build whose gen-5 frame cipher has been disassembled steps a
	// zlib or bz2 body floor(len/4) times (srawi ...,2), capped at the type's
	// limit of 4, and a raw body (len + 3) >> 2 times, cap 12 -- i.e. the
	// compressed step rounds DOWN. Confirmed in the binaries: the Windows
	// 4.04r2 and 4.20r8 engine DLLs, the Mac PowerPC 4.04r2, 5.12 and G3D27076
	// ("4.20r6"-labelled, a 4.10r2-era build) programs, and CLVER_5_12/_MAC
	// (Tiger lab 2026-11-13); and 6.03r7, read 2026-11-19 from the Linux x64
	// build (G3D0511C, platform-identical engine to the Windows G3D0311C):
	// an arithmetic shift right by 2, cap 4. No gen-5 build has ever been shown to
	// round up -- the old PerStartedGroup default was the legacy GS2Emu
	// behaviour, not a client read -- so round-down is the rule for every
	// gen-5 client. The two builds not machine-read (5.00r7, 5.24r4) are both
	// bracketed by read round-down builds. 2.22 predates the gen-5 cipher.
	// Upstream GServer-v2 beta4 (33d99fb) turns partial iteration off for
	// CLVER_4_0211 to CLVER_5_12 only.
	(void)versionId;
	return Gen5Steps::PerWholeGroup;
}
