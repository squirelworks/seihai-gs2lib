<!-- Seihai project, 2026-11-19: see CHANGES-SEIHAI.md. -->
# Known issues in seihai-gs2lib

A plain-language list of what we know is wrong, untested or missing in this
library, written for anyone who wants to help. gs2lib is the layer where a
Graal server first touches untrusted bytes from the network: framing, the
per-generation packet ciphers, zlib and bzip2, and the client version table. A
bug here usually shows up as a crash or a dropped client in
[seihai-gserver-v2](https://github.com/squirelworks/seihai-gserver-v2), whose
own list is in its
[KNOWN-ISSUES.md](https://github.com/squirelworks/seihai-gserver-v2/blob/seihai/KNOWN-ISSUES.md).

Last checked against branch `seihai` at the #38 merge (2026-11-19), which
includes upstream gs2lib through `9bc1a0f`.

**Short version:** there is no open, confirmed bug in this library today. What
is open is coverage: the fuzzers have run for minutes, not hours, and parts of
the version and packet tables are still unknown. If you want to find the next
bug, the fuzzing section below is the place to start.

## Contents

- [Before you start](#before-you-start)
- [Open work where bugs are likely to turn up](#open-work-where-bugs-are-likely-to-turn-up)
  1. [The fuzzers have barely run](#1-the-fuzzers-have-barely-run) (#27)
  2. [Two fuzzers cover the same entry point from different sides](#2-two-fuzzers-cover-the-same-entry-point-from-different-sides) (#27, seihai-gserver-v2#87)
  3. [Unknown client tokens are refused](#3-unknown-client-tokens-are-refused) (#11)
  4. [Packet ids still named UNHANDLED or UNKNOWN](#4-packet-ids-still-named-unhandled-or-unknown) (#11)
- [Behaviour that looks like a bug but is intended](#behaviour-that-looks-like-a-bug-but-is-intended)
- [Bugs fixed here that upstream gs2lib may still have](#bugs-fixed-here-that-upstream-gs2lib-may-still-have)

## Before you start

**Build and unit tests.** See [README.md](README.md). The tests need both
`-DGS2LIB_TESTS=ON` and `-DNET=ON`, and run with `ctest`. They are plain
in-process programs under `tests/`, one per area (`Gen5StepsTest`,
`GenerationBoundsTest`, `ParseBoundsTest`, `VersionLabelTest`,
`SocketFamilyTest`, `FileQueueSendTest`, `PacketNamesTest`). CI runs them on
Linux and Windows on every push, and on macOS at release time.

**Fuzzers.** See [fuzz/README.md](fuzz/README.md). They need Clang with
libFuzzer and are built only with `-DGS2LIB_FUZZ=ON`, which also instruments
the library itself with ASan and UBSan.

**Words used below.**

- **Generation** (cipher generation): which packet cipher a connection uses.
  It depends on the client version. `CEncryption` holds one codec per
  connection; `ENCRYPT_GEN_1` to `ENCRYPT_GEN_6` are the named ones.
- **Gen-5 step rule**: how far the gen-5 cipher's iterator moves per packet.
  Most clients round up per started 4-byte group; 5.12 rounds down. See
  `Gen5Steps` in `CEncryption`.
- **Client token**: the 8-character version string a client sends at login,
  such as `GNW03014` (2.22) or `G3D14097` (5.12). `src/IUtil.cpp` maps each to
  a `CLVER_*` id in release order, so servers can compare versions.

## Open work where bugs are likely to turn up

### 1. The fuzzers have barely run

Issue: [#27](https://github.com/squirelworks/seihai-gs2lib/issues/27).

There are four libFuzzer targets under `fuzz/`:

| Target | What it feeds untrusted bytes into |
|---|---|
| `fuzz_decrypt` | each cipher generation's decode |
| `fuzz_decompress` | zlib and bzip2 decompression |
| `fuzz_version` | the client token lookup |
| `fuzz_frame` | length-prefixed receive framing |

The workflow (`.github/workflows/fuzz.yml`) runs each target for 2 minutes once
a week, and for 30 seconds on pull requests that touch the decoders. As of
2026-11-19 it has run three times in total, all green. Only since #34
(2026-11-18) is the library itself built with sanitizers; before that, the
fuzzers could not see an overflow inside gs2lib at all.

So "no crash found" means very little yet. Useful contributions:

- Run any target locally for hours and report what you find (a crash file plus
  the commit you built).
- Grow the seed corpora in `fuzz/corpus/` from real traffic you captured
  yourself. Please do not commit captures from servers you do not run.
- Add a target for an untrusted path that has none, such as the packet queue
  (`CFileQueue`) receiving and splitting several packets, or `CString`'s
  packed-number reads.

Each crash should end up as a test under `tests/`, a fix, and a row in
`CHANGES-SEIHAI.md`.

### 2. Two fuzzers cover the same entry point from different sides

Issues: #27 here and
[seihai-gserver-v2#87](https://github.com/squirelworks/seihai-gserver-v2/issues/87).

seihai-gserver-v2 has its own weekly fuzz target over its `processBuffer`,
which calls into this library's framing, decryption and decompression before
handing the packet to a stub handler. That overlaps with `fuzz_frame`,
`fuzz_decrypt` and `fuzz_decompress` here.

The overlap is not a bug, but it matters when you find a crash: decide first
whether it is in gs2lib (it reproduces with a target here) or in the server
(it needs the server's state). File it in the matching repository, so the
regression test lands next to the code that was wrong. A crash found by the
server's fuzzer that turns out to be in gs2lib should get a target or corpus
entry here too.

### 3. Unknown client tokens are refused

Issue: [#11](https://github.com/squirelworks/seihai-gs2lib/issues/11).

A client whose login token is not in the table in `src/IUtil.cpp` gets
`CLVER_UNKNOWN`. No `allowedversions.txt` range admits that id, so the server
refuses the login. This has bitten real builds more than once (3.153, 5.24r4,
the Mac PowerPC builds and a dozen Linux builds were all added after a lab run
was refused).

If your client is refused and the server log shows an unknown version, the
token is probably missing. When you add one, put it in release order right
after its Windows twin, because servers compare ids with `>=` and `<=`, and
add a line to `tests/VersionLabelTest.cpp`. Also check the gen-5 step rule for
it (`gen5StepsForClient`): getting that wrong stops the client being read after
its first short compressed packet.

Deliberately not added: the listserver hello tokens and the 1999-era
RemoteControl tokens (`GSERV010`, `GSERV011`), which speak a protocol
GServer-v2's RC code was never run against. A comment above `rcVersions[]`
says so.

### 4. Packet ids still named UNHANDLED or UNKNOWN

Issue: [#11](https://github.com/squirelworks/seihai-gs2lib/issues/11).

`include/IEnums.h` follows upstream's names. A few ids are still called
`UNHANDLED` or `UNKNOWN` upstream, or have a name but an unknown layout (for
example the second sub-type of 108, the flag byte of 104, RC 124). These are
not bugs in the library, but a server that sends or reads them on guesswork
can be. Findings for most of them are in the
[Graal_analysis](https://github.com/squirelworks/Graal_analysis) protocol notes;
adding a comment with the evidence beside upstream's name is welcome.

## Behaviour that looks like a bug but is intended

- **Some clients round the gen-5 step down.** Stock gs2lib rounds up for every
  client. Windows 4.04r2, 4.20r8 and 5.12, and the Mac PowerPC 4.04r2, 4.10r2-era
  and 5.12 builds, really do round down on zlib and bzip2 bodies (their frame
  ciphers were read on 2026-11-13). With the stock rule the keystream falls out
  of step on the first short compressed body, and the client stops being read.
  See `Gen5StepsTest`.
- **`getVersionString` returns `"unknown"` for an unknown token**, not null.
  Upstream returns null, and its callers never check, so an unknown token used
  to abort the server.
- **`setGen` clamps out-of-range generations to `ENCRYPT_GEN_6`.** See
  [docs/generation-bounds.md](docs/generation-bounds.md).
- **5.00r7 is labelled "5.007"**, not "5.07". There is no client 5.07; the
  enum name `CLVER_5_07` is upstream's and stays.

## Bugs fixed here that upstream gs2lib may still have

If you run a server on upstream gs2lib, these are worth knowing about. Each has
a row in [CHANGES-SEIHAI.md](CHANGES-SEIHAI.md) with the details and a test.
"May still have" means: present at upstream `9bc1a0f`, our last merge. Check
upstream's current head before relying on it.

| Bug | Fixed here | What happens without the fix |
|---|---|---|
| `getVersionString` returns null for an unknown token | 2026-10-31 | The login log line and `loadAllowedVersions` use the null, and the server aborts. |
| Gen-5 step rule is round-up for every client | 2026-10-31, widened 2026-11-19 (`Gen5Steps`) | 4.04r2, 4.20r8 and 5.12 stop being read after their first short compressed body. |
| No `PLO_OTHERPLPROPS` cap | 2026-11-01 | The queue of position updates grows without bound with many walking players. |
| Socket created with the wrong address family | 2026-11-14 (`SocketFamilyTest`) | IPv6 and `ANY` addresses are passed to an IPv4 socket, so they cannot bind or connect. |
| Version-label lookup reads past the RC and NC tables | 2026-11-14 (`VersionLabelTest`) | An unknown or cross-table RC/NC token reads past the shorter table (ASan: global-buffer-overflow). |
| Short or zero-byte sends stall the encoded tail | 2026-11-14 (`FileQueueSendTest`) | After a partial or zero-byte send, the unsent tail of an already-encoded frame is not retried until unrelated new output arrives. |
| `setGen` stores one past the iterator table | 2026-11-15 (`GenerationBoundsTest`) | `reset` reads past the iterator table (ASan stops in `reset`). Whether a remote client can reach this was not established. |
| Caller-controlled lengths in string, decompression and websocket helpers | 2026-11-15 (`ParseBoundsTest`) | Reads and writes past the buffer in `base64encode`/`decode`, `encodesimple`/`decodesimple`, `find`, `read`, `write`, `right`, `setSize` and others; an empty `replaceAll` needle loops forever. |
| Large-file check compares the raw id with the enum | 2026-11-18 | `CFileQueue` compared the packet's first byte (the id plus 32) with the enum, so a large file was never seen as in progress and ids 36, 37, 52 and 70 held the file queue. |

Upstream has already taken our `poll`/`WSAPoll` socket wait and the
per-stub `unregisterSocket` fix, so those are no longer listed.
