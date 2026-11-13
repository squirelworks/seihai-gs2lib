<!-- Modified by the Seihai project, 2026-11-11 (README rewritten 2026-11-13): see CHANGES-SEIHAI.md. -->
# seihai-gs2lib

The Seihai fork of [gs2lib](https://bitbucket.org/xtjoeytx/gs2lib), the C++
support library that the OpenGraal GServer-v2 (GS2Emu) game server is built on.
It holds the nuts and bolts a Graal server needs: sockets, the packet ciphers,
the packet queues, packet names, and the table of Graal client version tokens.

Part of the Seihai server stack:
[seihai-gserver](https://github.com/squirelworks/seihai-gserver) ·
[seihai-npcserver](https://github.com/squirelworks/seihai-npcserver) ·
[seihai-listserver](https://github.com/squirelworks/seihai-listserver) ·
[seihai-rc](https://github.com/squirelworks/seihai-rc) ·
[seihai-servertools](https://github.com/squirelworks/seihai-servertools) ·
[seihai-gserver-v2](https://github.com/squirelworks/seihai-gserver-v2) (GPL fork of GServer-v2, the main user of this library) ·
client: [seihai](https://github.com/squirelworks/seihai).

## What is in it

| Part | Files | What it does |
|---|---|---|
| Strings | `CString` | The byte string every packet is built in, with Graal's packed-number reads and writes and zlib/bzip2 compression. |
| Sockets | `CSocket` | A socket class and a socket manager that waits on many connections with `poll` (`WSAPoll` on Windows). |
| Encryption | `CEncryption` | The packet ciphers of each protocol generation, with the per-client gen-5 step rule (`Gen5Steps`). |
| Packet queues | `CFileQueue` | One connection's outgoing normal and file queues, with a cap on queued position updates. |
| Packet ids | `IEnums.h` | Names for every client-to-server (`PLI_*`) and server-to-client (`PLO_*`) packet id, with older names kept as aliases. |
| Client versions | `IUtil` | Maps each client's login token (for example `GNW03081` or `G3D15097`) to a version id and label, in release order, so a server can compare versions with `>=` and `<=`. |
| Support | `CSettings`, `CLog`, `CTranslationManager`, `CTimeout`, `md5` | Settings files, logging, translations, timers and MD5. |

Where the version tokens and packet layouts come from is written up in
[Graal_analysis](https://github.com/squirelworks/Graal_analysis).

## Branches and changes

`master` is a copy of upstream and can lag behind it. The Seihai changes are on
the default branch `seihai`, which includes upstream through `e269a79`. Every
change, with the files it touched and how it was checked, is listed in
[`CHANGES-SEIHAI.md`](CHANGES-SEIHAI.md).
[seihai-gserver-v2](https://github.com/squirelworks/seihai-gserver-v2) pins a
commit of `seihai` in its `server/CMakeLists.txt`.

## Building

You need CMake 3.27 or newer, a C++ compiler (GCC 14, Clang or MSVC), Ninja,
zlib and bzip2. Two options change the build: `-DSTATIC=OFF` links the C++
runtime dynamically, and `-DNET=OFF` leaves out the networking code.

```sh
cmake -S . -B build -G Ninja
cmake --build build
```

Unit tests are off by default:

```sh
cmake -S . -B build -G Ninja -DGS2LIB_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

GitHub CI builds and runs the tests on Linux and Windows
(`.github/workflows/seihai-tests.yml`).

### macOS builds

macOS is built and tested on the lab's Sequoia (macOS 15) VM with the macOS
CMake preset and the tests the Linux job runs.

## Using it from CMake

Add it as a subdirectory (or fetch it with `FetchContent`, pinned to a commit
of `seihai`) and link the `gs2lib` target. Its `include` folder comes with it.

```cmake
add_subdirectory(gs2lib)
target_link_libraries(myserver PRIVATE gs2lib)
```

## Licence

GPL-3.0, as upstream: see [`LICENSE.md`](LICENSE.md). The original code is by
the OpenGraal developers. Files the Seihai project changed say so in their
header.

> **Not affiliated.** This is an independent fan project. It is not affiliated
> with, endorsed by, or connected to GraalOnline. "Graal" is used only to
> identify the software this library works with.
