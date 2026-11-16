// Modified by the Seihai project, 2026-11-16: see CHANGES-SEIHAI.md.
//
// This file is part of the Seihai fork of gs2lib and is released under the
// GNU General Public License version 3 or later (see LICENSE.md).
//
// libFuzzer target (#27): client / RC / NC version token lookup.

#include <cstddef>
#include <cstdint>

#include <CString.h>
#include <IUtil.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
	if (size > 64)
		return 0;

	CString token;
	if (size > 0)
		token.write(reinterpret_cast<const char*>(data), static_cast<int>(size));

	(void)getVersionID(token);
	for (int type = 0; type < 4; ++type)
		(void)getVersionString(token, type);
	return 0;
}
