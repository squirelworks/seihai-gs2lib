# Seihai changes to gs2lib

Mirror of [gs2lib](https://bitbucket.org/xtjoeytx/gs2lib) (GPL-3.0, see `LICENSE.md`).
`master` is upstream's. Our changes are on branch `seihai`, based on upstream
`5c36ab8` (the commit GServer-v2 beta4 pins). Used by
[seihai-gserver-v2](https://github.com/squirelworks/seihai-gserver-v2).

## 2026-10-31

| Change | Files | Checked with |
|---|---|---|
| `getVersionString` returns `"unknown"` instead of null. Its three callers (the login log line and `loadAllowedVersions`) never check for null, so an unknown token aborted the server. | `src/IUtil.cpp` | Seihai 3.153 before the token was added |
| 3.153 token `G3D28015` added as `CLVER_3_153`, between `CLVER_3_041` and `CLVER_4_0211`, so every version comparison still holds (no GS2 bytecode or 4.x packets, flags as for 3.041). The enum and both string tables gain one entry at the same place. | `include/IUtil.h`, `src/IUtil.cpp` | Seihai 3.04 and 3.153 log in to seihai-gserver-v2 and load the level |
