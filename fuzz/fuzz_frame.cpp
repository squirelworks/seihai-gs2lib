// Modified by the Seihai project, 2026-11-16: see CHANGES-SEIHAI.md.
//
// This file is part of the Seihai fork of gs2lib and is released under the
// GNU General Public License version 3 or later (see LICENSE.md).
//
// libFuzzer target (#27): gen-2/3/5 receive framing as CFileQueue writes it
// (u16be length, optional compression byte, then body) plus decrypt and
// decompress. In-process only.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <CEncryption.h>
#include <CString.h>

static uint16_t read_be16(const uint8_t* p)
{
	return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	// key, gen, steps, then one or more length-prefixed frames.
	if (size < 5)
		return 0;

	const uint8_t key = data[0] & 0x7F;
	const uint32_t gen = data[1] % (ENCRYPT_GEN_6 + 1U);
	const Gen5Steps steps =
		(data[2] & 1) != 0 ? Gen5Steps::PerWholeGroup : Gen5Steps::PerStartedGroup;

	CEncryption codec;
	codec.setGen(gen);
	codec.setGen5Steps(steps);
	codec.reset(key);

	size_t off = 3;
	int frames = 0;
	while (off + 2 <= size && frames < 8)
	{
		const uint16_t len = read_be16(data + off);
		off += 2;
		if (len == 0 || off + len > size)
			break;

		CString body;
		body.write(reinterpret_cast<const char*>(data + off), static_cast<int>(len));
		off += len;
		++frames;

		if (gen == ENCRYPT_GEN_1 || gen == ENCRYPT_GEN_2)
		{
			(void)body.zuncompress();
			continue;
		}

		if (gen == ENCRYPT_GEN_3)
		{
			codec.decrypt(body);
			(void)body.zuncompress();
			continue;
		}

		// Gen 4/5: first byte is the compression type after decrypt of the
		// remainder, matching sendCompress's (len+1) << type << payload.
		if (body.length() < 1)
			continue;
		const auto type = static_cast<uint8_t>(body[0]);
		body.removeI(0, 1);
		codec.limitFromType(type);
		codec.decrypt(body);
		if (type == COMPRESS_ZLIB)
			(void)body.zuncompress();
		else if (type == COMPRESS_BZ2)
			(void)body.bzuncompress();
	}
	return 0;
}
