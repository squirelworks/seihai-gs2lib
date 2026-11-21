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

static constexpr std::array ITERATOR_START = {0U, 0U, 0x04A80B38U, 0x4A80B38U, 0x4A80B38U, 0U};

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
			auto iterations = allowPartialIteration ? ((3 + buffer.length()) / 4) : buffer.length() / 4;
			if (iterationLimit >= 0)
				iterations = std::min(iterations, iterationLimit);

			// Process the iterations.
			for (size_t i = 0; i < iterations; ++i)
			{
				// Increase the iterator.
				m_lastIterator = m_iterator;
				m_iterator *= 0x8088405;
				m_iterator += m_key;

				// Apply to the buffer.
				const auto bufferStart = i * 4;
				const auto bufferEnd = buffer.length() - bufferStart;
				for (size_t j = 0; j < 4 && j < bufferEnd; ++j)
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
			auto iterations = allowPartialIteration ? ((3 + buffer.length()) / 4) : buffer.length() / 4;
			if (iterationLimit >= 0)
				iterations = std::min(iterations, iterationLimit);

			// Process the iterations.
			for (size_t i = 0; i < iterations; ++i)
			{
				// Increase the iterator.
				m_lastIterator = m_iterator;
				m_iterator *= 0x8088405;
				m_iterator += m_key;

				// Apply to the buffer.
				const auto bufferStart = i * 4;
				const auto bufferEnd = buffer.length() - bufferStart;
				for (size_t j = 0; j < 4 && j < bufferEnd; ++j)
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
			return;
		}
	}
}
