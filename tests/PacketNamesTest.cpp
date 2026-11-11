// Seihai project, 2026-11-07: see CHANGES-SEIHAI.md.
//
// This file is part of the Seihai fork of gs2lib and is released under the
// GNU General Public License version 3 or later (see LICENSE.md).
//
// The new packet names and the old UNKNOWN aliases are the same values.
// A failed static_assert does not link.

#include <IEnums.h>

static_assert(PLI_WEAPONUPLOAD == 25);
static_assert(PLI_UNKNOWN25 == PLI_WEAPONUPLOAD);
static_assert(PLI_WEAPONADD == 33);
static_assert(PLI_PING == 162);
static_assert(PLI_RC_UNKNOWN162 == PLI_PING);

static_assert(PLO_UNKNOWN60 == 60);
static_assert(PLO_UNKNOWN81 == 81);
static_assert(PLO_UNKNOWN83 == 83);
static_assert(PLO_FILEKEY == 104);
static_assert(PLO_UNKNOWN104 == PLO_FILEKEY);
static_assert(PLO_PING == 108);
static_assert(PLO_UNKNOWN108 == PLO_PING);
static_assert(PLO_DOWNLOADURL == 109);
static_assert(PLO_UNKNOWN109 == PLO_DOWNLOADURL);
static_assert(PLO_UNKNOWN110 == 110);
static_assert(PLO_RC_PLAYERFLAGS == 124);
static_assert(PLO_UNKNOWN124 == PLO_RC_PLAYERFLAGS);
static_assert(PLO_UNKNOWN132 == 132);
static_assert(PLO_UNKNOWN133 == 133);
static_assert(PLO_NPCWEAPONSCRIPT == 140);
static_assert(PLO_LOGGEDIN == 168);
static_assert(PLO_UNKNOWN168 == PLO_LOGGEDIN);
static_assert(PLO_UNKNOWN181 == 181);
static_assert(PLO_WEAPONCRC == 193);
static_assert(PLO_UNKNOWN193 == PLO_WEAPONCRC);
static_assert(PLO_CLEARWEAPONS == 194);
static_assert(PLO_LOADSCRIPT == 197);

int main()
{
	return 0;
}
