// Modified by the Seihai project, 2026-11-16: see CHANGES-SEIHAI.md.
//
// This file is part of the Seihai fork of gs2lib and is released under the
// GNU General Public License version 3 or later (see LICENSE.md).
//
// libFuzzer target (#27): zlib / bzip2 uncompress on untrusted buffers.

#include <cstddef>
#include <cstdint>

#include <CString.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	if (size == 0 || size > 256 * 1024)
		return 0;

	CString buf;
	buf.write(reinterpret_cast<const char*>(data), static_cast<int>(size));
	(void)buf.zuncompress();
	(void)buf.bzuncompress();

	bool usedDeflate = false;
	CString again;
	again.write(reinterpret_cast<const char*>(data), static_cast<int>(size));
	(void)again.zuncompress2(usedDeflate);
	return 0;
}
