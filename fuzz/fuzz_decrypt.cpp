// Modified by the Seihai project, 2026-11-16: see CHANGES-SEIHAI.md.
//
// This file is part of the Seihai fork of gs2lib and is released under the
// GNU General Public License version 3 or later (see LICENSE.md).
//
// libFuzzer target (#27): CEncryption::decrypt for every generation.

#include <cstddef>
#include <cstdint>

#include <CEncryption.h>
#include <CString.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	if (size < 3)
		return 0;

	const uint32_t gen = data[0] % (ENCRYPT_GEN_6 + 1U);
	const uint8_t key = data[1] & 0x7F;
	const Gen5Steps steps =
		(data[2] & 1) != 0 ? Gen5Steps::PerWholeGroup : Gen5Steps::PerStartedGroup;
	const uint8_t* body = data + 3;
	const size_t bodyLen = size - 3;
	if (bodyLen > 256 * 1024)
		return 0;

	CString buf;
	if (bodyLen > 0)
		buf.write(reinterpret_cast<const char*>(body), static_cast<int>(bodyLen));

	CEncryption codec;
	codec.setGen(gen);
	codec.setGen5Steps(steps);
	codec.reset(key);
	if (gen == ENCRYPT_GEN_4 || gen == ENCRYPT_GEN_5)
	{
		const uint8_t type = bodyLen > 0 ? body[0] : COMPRESS_UNCOMPRESSED;
		codec.limitFromType(type);
	}
	codec.decrypt(buf);
	return 0;
}
