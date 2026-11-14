# Cipher generation bounds

This note explains the generation-table clamp on pull request 26
(`cursor/gs2lib-bounds-cae0`). The code change is already on that branch.
This file only describes it.

It is a bounds fix inside this library. Remote reachability was not
established.

## What `setGen` stored

`CEncryption::setGen` records which cipher generation a codec will use.
It clamped that generation to the range 0 through 6.

The iterator table has six entries. Their indexes run from 0 through
`ENCRYPT_GEN_6`. `ENCRYPT_GEN_6` is the last named generation, so a stored
6 sits one entry past the table.

A generation of 6, or any larger value the old clamp reduced to 6, was
stored as an index the table does not contain.

## What `reset` did with that generation

`CEncryption::reset` reads the iterator table at the stored generation
when it chooses the iterator's starting value. With a stored generation
of 6, that read went past the table.

## What the patch does

`setGen` now clamps to `ENCRYPT_GEN_6`. A generation passed in from
outside the table lands on the last real entry.

`reset` folds a stored generation back into the table before that read.
A generation that was already sitting past the last entry is brought
back onto it, and then the table is read.

Named generations are unchanged. Each value from 0 through
`ENCRYPT_GEN_6` (`ENCRYPT_GEN_1` through `ENCRYPT_GEN_6`) is still
stored as given, and `reset` leaves it as given.

## How it was checked

`GenerationBoundsTest` (`tests/GenerationBoundsTest.cpp`) runs in-process.
It exercises the codec only. It opens no socket, and it includes no
remote client.

Built against the previous `setGen`, the test failed eight checks. The
same test, built from those sources with GCC address and
undefined-behaviour sanitizers, stopped in `reset` on a read past the
iterator table.

With the clamp, that sanitizer run exits 0. Generations 0 through
`ENCRYPT_GEN_6` stay put. `ENCRYPT_GEN_6 + 1`, `ENCRYPT_GEN_6 + 2`, 64,
and the maximum `uint32_t` each clamp to `ENCRYPT_GEN_6`, both before
`reset` and after it.

Local CTest was 6/6: `Gen5StepsTest`, `GenerationBoundsTest`,
`PacketNamesTest`, `VersionLabelTest`, `SocketFamilyTest`, and
`FileQueueSendTest`.

## Already on `seihai`, or left out of this pull request

The file-queue residual send is already on `seihai` in pull request 24.

The RC/NC version-label bound is already on `seihai` in merged pull
request 23.

This pull request leaves the WebSocket framer as it stands.
