<!-- Modified by the Seihai project, 2026-11-11: see CHANGES-SEIHAI.md. -->
# GS2Lib

Common library for OpenGraal software

## Seihai fork

This is the Seihai mirror of [gs2lib](https://bitbucket.org/xtjoeytx/gs2lib).
`master` is a copy of upstream (it can lag behind upstream); the Seihai
changes are on the default branch `seihai`, which includes upstream through
`e269a79`, and are listed in [`CHANGES-SEIHAI.md`](CHANGES-SEIHAI.md).
[seihai-gserver-v2](https://github.com/squirelworks/seihai-gserver-v2) pins a
commit of `seihai` in its `server/CMakeLists.txt`.

Unit tests are off by default:

```sh
cmake -S . -B build -G Ninja -DGS2LIB_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## macOS builds

macOS is built and tested on the lab's Sequoia (macOS 15) VM with the macOS CMake preset and the tests the Linux job runs; GitHub CI covers Linux and Windows.
