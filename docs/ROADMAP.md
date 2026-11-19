# seihai-gs2lib roadmap

The live checklist is issue #11; the suite-wide plan is squirelworks/seihai#353.
This library is used only by seihai-gserver-v2, so the roadmap stays short:
stay close to upstream and keep each change tested. Last checked 2026-11-19
against branch `seihai`.

## Phase 1: to functional (current priority: bugs and crashes)

- [ ] Fuzz the framing, cipher and decompression paths under sanitizers (#27); scaffold and weekly smoke are in, longer runs and crasher→unit-test still open.
- [ ] New client tokens as real builds turn up that upstream refuses (as 3.153 and iPhone Classic+ 1.13 were).
- [ ] Comments beside upstream's `UNHANDLED`/`UNKNOWN` names when Graal_analysis settles an id (squirelworks/Graal_analysis#254).

## Phase 2: polish and release

- [x] macOS build and tests at release time only (#28): a `macos` job on tags and manual runs, and seihai-gserver-v2's release runs the tests for its pin.
- [ ] Follow upstream: merge it into `seihai`, keep each Seihai row, bump the pin in seihai-gserver-v2 in the same round; fast-forward `master` to upstream's head.
- [x] Offer the crash and socket fixes upstream if the owner agrees (#10), which shrinks the fork.

## Phase 3: after it works

- **Reference only**: when seihai-gserver-v2 retires to a reference (squirelworks/seihai#353 phase 7), this fork takes bug fixes only.
- **Wire-format test vectors**: publish round-trip vectors (per generation, our own lab traffic only) that the clean-room Rust stack can test against without reading this code.
