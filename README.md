<!-- Modified by the Seihai project, 2026-11-11: see CHANGES-SEIHAI.md. -->
# GS2Lib

Common library for OpenGraal software

## Seihai fork

This is the Seihai mirror of [gs2lib](https://bitbucket.org/xtjoeytx/gs2lib).
`master` mirrors upstream; the Seihai changes are on the default branch
`seihai` and are listed in [`CHANGES-SEIHAI.md`](CHANGES-SEIHAI.md).
[seihai-gserver-v2](https://github.com/squirelworks/seihai-gserver-v2) pins a
commit of `seihai` in its `server/CMakeLists.txt`.

Unit tests are off by default:

```sh
cmake -S . -B build -G Ninja -DGS2LIB_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```
