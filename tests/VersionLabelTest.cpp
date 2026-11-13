// Seihai project, 2026-11-12, 2026-11-13: see CHANGES-SEIHAI.md.
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
	// The G3D27076 build was archived as "4.20r6"; it is a 4.10r2/4.11-era build.
	check("G3D27076", "4.102 (Mac)");
	check("G3D21126", "4.206 (Mac)");
	check("G3D15097", "5.12 (Mac)");
	// Seihai: 5.24r3/5.24r4 (and 6.00 beta 2) send G3D15010.
	check("G3D15010", "5.24");
	if (getVersionID(CString("G3D15010")) != CLVER_5_24)
	{
		std::printf("FAIL: G3D15010 is not CLVER_5_24\n");
		++failures;
	}
	if (getVersionID(CString("G3D18026")) != CLVER_4_042_MAC || getVersionID(CString("G3D27076")) != CLVER_4_102_MAC
		|| getVersionID(CString("G3D21126")) != CLVER_4_206_MAC
		|| getVersionID(CString("G3D15097")) != CLVER_5_12_MAC)
	{
		std::printf("FAIL: Mac token ids\n");
		++failures;
	}
	// Seihai: the game tokens of Graal_analysis version-token-catalogue.md that
	// stock gs2lib did not know, each with its id and label.
	struct Row { const char* token; int id; const char* label; };
	const Row catalogue[] = {
		{ "GNW23090", CLVER_1_4, "1.40" },
		{ "GNW08011", CLVER_1_42_NEWMAIN, "1.42 (NewMain)" },
		{ "G3D27015", CLVER_3_151_LINUX, "3.151 (Linux)" },
		{ "G3D11125", CLVER_4_031_LINUX, "4.031 (Linux)" },
		{ "G3D19026", CLVER_4_042_LINUX, "4.042 (Linux)" },
		{ "G3D28076", CLVER_4_110_LINUX, "4.110 (Linux)" },
		{ "G3D22126", CLVER_4_208_LINUX, "4.208 (Linux)" },
		{ "G3D03048", CLVER_6_00_BETA1, "6.00 beta1" },
		{ "G3D27090", CLVER_6_004_MAC, "6.004 (Mac)" },
		{ "G3D28090", CLVER_6_00_LINUX_BETA, "6.00 beta (Linux)" },
		{ "G3D3107A", CLVER_6_015_MAC, "6.015 (Mac)" },
		{ "G3D0108A", CLVER_6_01_LINUX, "6.01 (Linux)" },
	};
	for (const Row& r : catalogue)
	{
		check(r.token, r.label);
		if (getVersionID(CString(r.token)) != r.id)
		{
			std::printf("FAIL: %s has id %d, expected %d\n", r.token, getVersionID(CString(r.token)), r.id);
			++failures;
		}
	}
	// Range checks in allowedversions.txt and GServer-v2 compare ids, so each
	// platform build sits next to its release: after the older release, before the newer one.
	if (!(CLVER_1_411 < CLVER_1_42_NEWMAIN && CLVER_1_42_NEWMAIN < CLVER_2_0
		  && CLVER_3_041 < CLVER_3_151_LINUX && CLVER_3_151_LINUX < CLVER_3_153
		  && CLVER_4_110 < CLVER_4_102_MAC && CLVER_4_206_MAC < CLVER_4_208
		  && CLVER_5_12_MAC < CLVER_6_00_BETA1 && CLVER_6_00_BETA1 < CLVER_5_24
		  && CLVER_5_31x < CLVER_6_004_MAC && CLVER_6_00_LINUX_BETA < CLVER_6_015
		  && CLVER_6_015 < CLVER_6_01_LINUX && CLVER_6_01_LINUX < CLVER_6_034))
	{
		std::printf("FAIL: enum order\n");
		++failures;
	}
	// Non-game hello tokens stay unknown to the game table: the listserver hellos
	// never reach a game server, and GSERV010 is the 1.25 RemoteControl.
	for (const char* t : { "GNW30123", "G3D30123", "GSERV010" })
	{
		if (getVersionID(CString(t)) != CLVER_UNKNOWN)
		{
			std::printf("FAIL: %s should not be a client version\n", t);
			++failures;
		}
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
