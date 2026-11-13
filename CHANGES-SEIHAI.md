# Seihai changes to gs2lib

Mirror of [gs2lib](https://bitbucket.org/xtjoeytx/gs2lib) (GPL-3.0, see `LICENSE.md`).
`master` is a copy of upstream and can lag behind it. Our changes are on branch
`seihai`, which includes upstream through `e269a79` (the commit GServer-v2 beta4
pins since 2026-11-11). Used by
[seihai-gserver-v2](https://github.com/squirelworks/seihai-gserver-v2).

## 2026-11-13

| Change | Files | Checked with |
|---|---|---|
| The Mac PowerPC builds get their own version ids: `G3D18026` is `CLVER_4_042_MAC` ("4.042 (Mac)"), `G3D27076` is `CLVER_4_206_MAC` ("4.206 (Mac)") and `G3D15097` is `CLVER_5_12_MAC` ("5.12 (Mac)"). Each sits right after its Windows twin in the enum, so `>=`/`<=` range checks treat it as that release. Before, all three were `CLVER_UNKNOWN`: an `allowedversions.txt` that listed one unknown token let in all of them, the listserver's `PLO_SERVERWARP` (178, sent only from 2.1 on) never reached them, and GS1 text weapons went to the Mac 5.12. `gen5StepsForClient` gives the Mac 4.04r2 and 5.12 the round-down rule: both frame ciphers compute raw `(len + 3) >> 2` (at most 12) and zlib/bz2 `len >> 2` (at most 4), read in the binaries (the 4.04r2 and 5.12 Mac programs). The Mac 4.20r6 keeps round-up: its cipher was not read. | `include/IUtil.h`, `src/IUtil.cpp`, `src/CEncryption.cpp`, `tests/VersionLabelTest.cpp`, `tests/Gen5StepsTest.cpp` | Unit tests (`-DGS2LIB_TESTS=ON`, all three pass); both Mac builds ran unchanged against seihai-gserver-v2 on Mac OS X 10.4 in qemu (Tiger lab 2026-11-13), every frame of the three captures decodes under the round-down rule |

## 2026-11-12

| Change | Files | Checked with |
|---|---|---|
| 5.00r7 (token `G3D22067`) is labelled `"5.007"` in logs and the "Allowed:" message, not `"5.07"`. The build is 5.0.0.7, written like `"4.042"` for 4.04r2 and `"4.208"` for 4.20r8; there is no client 5.07. Display only: logins are matched by token. The upstream enum name `CLVER_5_07` stays. Closes #12. | `src/IUtil.cpp`, `tests/VersionLabelTest.cpp`, `CMakeLists.txt` | `VersionLabelTest` (5.00r7, 4.20r8 and 5.12 tokens map to their labels); local build with `-DGS2LIB_TESTS=ON`, all three tests pass |
| Merged upstream `ef9dc6c` and `e269a79` (2026-11-11), which GServer-v2 beta4 `fe72da9` and later need. `CFileQueue::addPacket` and `addRawPacket` now take a `PacketQueue` (`NORMAL` or `FILE`) from the caller instead of picking the queue from the packet id, which fixes upstream's file queue ordering errors. Our cap on queued position updates stays: it now applies to `PLO_OTHERPLPROPS` added to the `NORMAL` queue. | `include/CFileQueue.h`, `src/CFileQueue.cpp` | Unit tests; seihai-gserver-v2 merged with beta4 `2de276b` builds against it |

## 2026-11-11

| Change | Files | Checked with |
|---|---|---|
| Staleness pass, docs only. `README.md` and the header of this file no longer say `master` mirrors upstream exactly (it is still at `5c36ab8`; `seihai` already includes upstream through `35056f8`). The upstream-merge row below notes that the packet-names row superseded it. | `README.md`, `CHANGES-SEIHAI.md` | Read against the branches and `include/IEnums.h` |
| Packet names now follow upstream gs2lib `35056f8`, which the community has researched longer. Upstream's name is the primary one for every id both sides named, and our 2026-11-07 names are aliases with the same value: `PLI_TOWEAPON` (25; alias `PLI_WEAPONUPLOAD`), `PLI_PONG` (162; `PLI_PING`), `PLO_FILEDECRYPTKEY` (104; `PLO_FILEKEY`), `PLO_HTTPDOWNLOAD` (109; `PLO_DOWNLOADURL`), `PLO_RC_UNKNOWN124` (124; `PLO_RC_PLAYERFLAGS`), `PLO_ONLINEMODE` (168; `PLO_LOGGEDIN`), `PLO_WEAPONHASH` (193; `PLO_WEAPONCRC`). Upstream's names for 60, 81, 83, 110, 132, 133, 140, 181 and 194 become primary too, with the older names as aliases. Our comments and status tags stay; where upstream gives a layout or meaning we did not have (132, 133, 140, 181), its note is added beside ours. No id or value changes. | `include/IEnums.h`, `tests/PacketNamesTest.cpp` | `PacketNamesTest` (each primary name equals its id, each alias equals its primary) |
| Housekeeping, no code change. The README says what this fork is, where the changes are listed and how to run the unit tests. `PacketNamesTest.cpp` is a new file, so its header says "Seihai project" like `Gen5StepsTest.cpp`, not "Modified by". | `README.md`, `tests/PacketNamesTest.cpp` | Local build with `-DGS2LIB_TESTS=ON`, both tests pass |
| Merged upstream `35056f8` (upstream's own packet renames of 2026-11-06), which GServer-v2 beta4 `25396a1` needs. Our names stay the primary names (superseded the same day by the "Packet names now follow upstream" row above: upstream's names are now primary); every upstream name is added as an alias with the same value: `PLI_TOWEAPON` (25), `PLI_PONG` (162), `PLO_UNHANDLED60/81/83/110`, `PLO_FILEDECRYPTKEY` (104), `PLO_HTTPDOWNLOAD` (109), `PLO_RC_UNKNOWN124`, `PLO_WEAPONBYTECODE` (132), `PLO_CLASSBYTECODE` (133), `PLO_WEAPONBYTECODE2` (140), `PLO_ONLINEMODE` (168), `PLO_PLAYERNOUDP` (181), `PLO_WEAPONHASH` (193), `PLO_RESETCLIENT` (194). For 26 the upstream name `PLO_NPCSETSCRIPT` becomes primary (we had no finding of our own) and `PLO_NPCACTION` stays as an alias. Upstream's whitespace-only edits to the same lines are not taken. | `include/IEnums.h`, `tests/PacketNamesTest.cpp` | `PacketNamesTest`; seihai-gserver-v2 merged with beta4 `25396a1` builds against it |

## 2026-11-07

| Change | Files | Checked with |
|---|---|---|
| Named the packet ids we now understand, and kept every old `*_UNKNOWN*` name as an alias with the same value. Client to server: 25 is `PLI_WEAPONUPLOAD` (1.21 to 1.37 weapon upload), 162 is `PLI_PING` (game-client answer to 108, and an empty RC keepalive). Server to client: 104 `PLO_FILEKEY`, 108 `PLO_PING`, 109 `PLO_DOWNLOADURL`, 124 `PLO_RC_PLAYERFLAGS`, 168 `PLO_LOGGEDIN`, 193 `PLO_WEAPONCRC`. 60, 81, 83, 110, 132, 133 and 181 stay unnamed, with the comment corrected. 140, 194 and 197 comments now match the cache and clear-weapons notes. Each comment is tagged confirmed, inferred or unknown. | `include/IEnums.h`, `tests/PacketNamesTest.cpp`, `CMakeLists.txt` | `PacketNamesTest` (each new name equals its id and its old alias). No real client: this change is names and comments only. |

## 2026-11-04

| Change | Files | Checked with |
|---|---|---|
| The test workflow also builds and runs the unit tests on macOS (`macos-latest`, AppleClang, zlib and bzip2 from the SDK) and Windows (`windows-latest`, MSVC, zlib and bzip2 from the runner's vcpkg with the `x64-windows-static` triplet). The Linux job is unchanged. | `.github/workflows/seihai-tests.yml` | CI on all three platforms |

## 2026-11-01

| Change | Files | Checked with |
|---|---|---|
| The socket wait is `poll` (`WSAPoll` on Windows) instead of `select`, so a descriptor at or past `FD_SETSIZE` no longer aborts the process. `unregisterSocket` removes only the stub it was given and nulls that slot before `onUnregister`. It used to treat every closed socket as the same socket and walk a function-static set; one disconnect then destroyed the other closed players and re-entered that set. Once a client's normal queue already holds 4096 packets, further position updates (`PLO_OTHERPLPROPS`) are not queued, so a level full of walking players cannot grow that queue without bound. | `src/CSocket.cpp`, `include/CSocket.h`, `src/CFileQueue.cpp` | Synthetic 2.05 client (token `GNW03081`) on `127.0.0.1:14802` against seihai-gserver-v2. Unfixed: 1100 raw connections aborted (exit 134, `FD_SETSIZE`); 200 logged-in players died mid-disconnect. Fixed: 1100 raw connections stayed up; 1000 logged-in players, including 1000 walking, disconnected with the process still alive. RSS at 1000 walkers about 25 MB. |

## 2026-10-31

| Change | Files | Checked with |
|---|---|---|
| `getVersionString` returns `"unknown"` instead of null. Its three callers (the login log line and `loadAllowedVersions`) never check for null, so an unknown token aborted the server. | `src/IUtil.cpp` | Seihai 3.153 before the token was added |
| 3.153 token `G3D28015` added as `CLVER_3_153`, between `CLVER_3_041` and `CLVER_4_0211`, so every version comparison still holds (no GS2 bytecode or 4.x packets, flags as for 3.041). The enum and both string tables gain one entry at the same place. | `include/IUtil.h`, `src/IUtil.cpp` | Seihai 3.04 and 3.153 log in to seihai-gserver-v2 and load the level |
| Per-version gen-5 step rule (`Gen5Steps`). 5.12 steps the gen-5 iterator once per whole 4-byte group (rounded down) on zlib and bz2 bodies and leaves a partial last group in the clear; every other client, and stock gs2lib, steps once per started group (rounded up). On 5.12's short compressed bodies the two disagreed, and the server read nothing more from that client. `CEncryption::gen5StepsForClient(versionId)` gives the rule for a client, and `CEncryption::setGen5Steps` / `CFileQueue::setGen5Steps` apply it to the input and output codecs. The default is unchanged. | `include/CEncryption.h`, `src/CEncryption.cpp`, `include/CFileQueue.h` | `tests/Gen5StepsTest.cpp`; Seihai 5.12 through a Login lobby on seihai-gserver-v2 (loopback lab) |
| Unit tests behind `-DGS2LIB_TESTS=ON` (off by default), and a GitHub workflow that builds and runs them. | `CMakeLists.txt`, `tests/`, `.github/workflows/seihai-tests.yml` | CI |
