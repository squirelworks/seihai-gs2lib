# Seihai changes to gs2lib

Mirror of [gs2lib](https://bitbucket.org/xtjoeytx/gs2lib) (GPL-3.0, see `LICENSE.md`).
`master` is upstream's. Our changes are on branch `seihai`, based on upstream
`5c36ab8` (the commit GServer-v2 beta4 pins). Used by
[seihai-gserver-v2](https://github.com/squirelworks/seihai-gserver-v2).

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
