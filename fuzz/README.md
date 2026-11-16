<!-- Modified by the Seihai project, 2026-11-16: see CHANGES-SEIHAI.md. -->
# Fuzz targets for seihai-gs2lib (#27)

Clang/libFuzzer harnesses for the paths that first touch untrusted bytes:
cipher generations, zlib/bzip2, version-token lookup, and length-prefixed
receive framing. Built only with `-DGS2LIB_FUZZ=ON` (not part of the default
or `GS2LIB_TESTS` builds).

## Build

```sh
CC=clang-18 CXX=clang++-18 cmake -S . -B build-fuzz -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DGS2LIB_FUZZ=ON -DSTATIC=ON -DNET=ON
cmake --build build-fuzz -j
```

## Run

```sh
./build-fuzz/fuzz_decrypt -max_total_time=60 fuzz/corpus/decrypt
./build-fuzz/fuzz_decompress -max_total_time=60 fuzz/corpus/decompress
./build-fuzz/fuzz_version -max_total_time=60 fuzz/corpus/version
./build-fuzz/fuzz_frame -max_total_time=60 fuzz/corpus/frame
```

Crashers should become a `tests/` regression, a fix, and a `CHANGES-SEIHAI.md`
row (and an upstream offer when #10 applies).
