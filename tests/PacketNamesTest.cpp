// Seihai project, 2026-11-07 (names flipped to upstream 2026-11-11): see CHANGES-SEIHAI.md.
//
// This file is part of the Seihai fork of gs2lib and is released under the
// GNU General Public License version 3 or later (see LICENSE.md).
//
// The primary packet names and their aliases are the same values.
// A failed static_assert does not link.

#include <IEnums.h>

// Primary names follow upstream gs2lib 35056f8.
static_assert(PLI_TOWEAPON == 25);
static_assert(PLI_WEAPONADD == 33);
static_assert(PLI_PONG == 162);
static_assert(PLO_NPCSETSCRIPT == 26);
static_assert(PLO_UNHANDLED60 == 60);
static_assert(PLO_UNHANDLED81 == 81);
static_assert(PLO_UNHANDLED83 == 83);
static_assert(PLO_FILEDECRYPTKEY == 104);
static_assert(PLO_PING == 108);
static_assert(PLO_HTTPDOWNLOAD == 109);
static_assert(PLO_UNHANDLED110 == 110);
static_assert(PLO_RC_UNKNOWN124 == 124);
static_assert(PLO_WEAPONBYTECODE == 132);
static_assert(PLO_CLASSBYTECODE == 133);
static_assert(PLO_WEAPONBYTECODE2 == 140);
static_assert(PLO_ONLINEMODE == 168);
static_assert(PLO_PLAYERNOUDP == 181);
static_assert(PLO_WEAPONHASH == 193);
static_assert(PLO_RESETCLIENT == 194);
static_assert(PLO_LOADSCRIPT == 197);

// Seihai 2026-11-07 names, kept as aliases.
static_assert(PLI_WEAPONUPLOAD == PLI_TOWEAPON);
static_assert(PLI_PING == PLI_PONG);
static_assert(PLO_FILEKEY == PLO_FILEDECRYPTKEY);
static_assert(PLO_DOWNLOADURL == PLO_HTTPDOWNLOAD);
static_assert(PLO_RC_PLAYERFLAGS == PLO_RC_UNKNOWN124);
static_assert(PLO_LOGGEDIN == PLO_ONLINEMODE);
static_assert(PLO_WEAPONCRC == PLO_WEAPONHASH);

// Older gs2lib names, kept as aliases.
static_assert(PLI_UNKNOWN25 == PLI_TOWEAPON);
static_assert(PLI_RC_UNKNOWN162 == PLI_PONG);
static_assert(PLO_NPCACTION == PLO_NPCSETSCRIPT);
static_assert(PLO_UNKNOWN60 == PLO_UNHANDLED60);
static_assert(PLO_UNKNOWN81 == PLO_UNHANDLED81);
static_assert(PLO_UNKNOWN83 == PLO_UNHANDLED83);
static_assert(PLO_UNKNOWN104 == PLO_FILEDECRYPTKEY);
static_assert(PLO_UNKNOWN108 == PLO_PING);
static_assert(PLO_UNKNOWN109 == PLO_HTTPDOWNLOAD);
static_assert(PLO_UNKNOWN110 == PLO_UNHANDLED110);
static_assert(PLO_UNKNOWN124 == PLO_RC_UNKNOWN124);
static_assert(PLO_UNKNOWN132 == PLO_WEAPONBYTECODE);
static_assert(PLO_UNKNOWN133 == PLO_CLASSBYTECODE);
static_assert(PLO_NPCWEAPONSCRIPT == PLO_WEAPONBYTECODE2);
static_assert(PLO_UNKNOWN168 == PLO_ONLINEMODE);
static_assert(PLO_UNKNOWN181 == PLO_PLAYERNOUDP);
static_assert(PLO_UNKNOWN193 == PLO_WEAPONHASH);
static_assert(PLO_CLEARWEAPONS == PLO_RESETCLIENT);

int main()
{
	return 0;
}
