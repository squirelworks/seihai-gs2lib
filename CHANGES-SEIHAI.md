# Seihai changes to gs2lib

Mirror of [gs2lib](https://bitbucket.org/xtjoeytx/gs2lib) (GPL-3.0, see `LICENSE.md`).
`master` is upstream's. Our changes are on branch `seihai`, based on upstream
`35056f8` (the commit GServer-v2 beta4 pins since 2026-11-09). Used by
[seihai-gserver-v2](https://github.com/squirelworks/seihai-gserver-v2).

## 2026-11-11

| Change | Files | Checked with |
|---|---|---|
| Housekeeping, no code change. The README says what this fork is, where the changes are listed and how to run the unit tests. `PacketNamesTest.cpp` is a new file, so its header says "Seihai project" like `Gen5StepsTest.cpp`, not "Modified by". | `README.md`, `tests/PacketNamesTest.cpp` | Local build with `-DGS2LIB_TESTS=ON`, both tests pass |
| Merged upstream `35056f8` (upstream's own packet renames of 2026-11-06), which GServer-v2 beta4 `25396a1` needs. Our names stay the primary names; every upstream name is added as an alias with the same value: `PLI_TOWEAPON` (25), `PLI_PONG` (162), `PLO_UNHANDLED60/81/83/110`, `PLO_FILEDECRYPTKEY` (104), `PLO_HTTPDOWNLOAD` (109), `PLO_RC_UNKNOWN124`, `PLO_WEAPONBYTECODE` (132), `PLO_CLASSBYTECODE` (133), `PLO_WEAPONBYTECODE2` (140), `PLO_ONLINEMODE` (168), `PLO_PLAYERNOUDP` (181), `PLO_WEAPONHASH` (193), `PLO_RESETCLIENT` (194). For 26 the upstream name `PLO_NPCSETSCRIPT` becomes primary (we had no finding of our own) and `PLO_NPCACTION` stays as an alias. Upstream's whitespace-only edits to the same lines are not taken. | `include/IEnums.h`, `tests/PacketNamesTest.cpp` | `PacketNamesTest`; seihai-gserver-v2 merged with beta4 `25396a1` builds against it |

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
