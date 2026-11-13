// Seihai project, 2026-11-12: see CHANGES-SEIHAI.md.
//
// This file is part of the Seihai fork of gs2lib and is released under the
// GNU General Public License version 3 or later (see LICENSE.md).
//
// The version labels in logs and the "Allowed:" message use the build number
// the way allowedversions.txt writes it: 5.00r7 is "5.007", like 4.20r8 "4.208".

#include <IUtil.h>
#include <IEnums.h>

#include <cstdio>
#include <cstring>

static int failures = 0;

static void check(const char* token, const char* expected)
{
	const char* got = getVersionString(CString(token), PLTYPE_CLIENT);
	if (got == nullptr || std::strcmp(got, expected) != 0)
	{
		std::printf("FAIL %s: expected \"%s\", got \"%s\"\n", token, expected, got ? got : "(null)");
		++failures;
	}
}

int main()
{
	check("G3D22067", "5.007");
	check("G3D20126", "4.208");
	check("G3D14097", "5.12");
	// Seihai: the Mac PowerPC builds have their own tokens and labels.
	check("G3D18026", "4.042 (Mac)");
	check("G3D27076", "4.206 (Mac)");
	check("G3D15097", "5.12 (Mac)");
	if (getVersionID(CString("G3D18026")) != CLVER_4_042_MAC || getVersionID(CString("G3D27076")) != CLVER_4_206_MAC
		|| getVersionID(CString("G3D15097")) != CLVER_5_12_MAC)
	{
		std::printf("FAIL: Mac token ids\n");
		++failures;
	}
	// The table and the enum stay in step: the last client entry still maps to its id.
	if (getVersionID(CString("G3D2204D")) != CLVER_WORLDS)
	{
		std::printf("FAIL: G3D2204D is not CLVER_WORLDS\n");
		++failures;
	}
	if (failures == 0)
		std::printf("VersionLabelTest: all passed\n");
	return failures == 0 ? 0 : 1;
}
