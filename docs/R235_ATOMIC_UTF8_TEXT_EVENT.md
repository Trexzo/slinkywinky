# R235 — Atomic bounded UTF-8 source text events

Date: 2026-11-16. Chat 1; draft PR #1 only.

## Verified regression in R234

The retained WGL recovered text field accepted up to four source-event bytes and validated every UTF-8 scalar before writing, but it appended each scalar independently. At a 127/128-byte field, an event containing two printable bytes could write the first byte and reject the second. That violates all-or-nothing event handling and can surprise the original GUI operator.

## Change

- Add a MenuState method that validates a complete, bounded source InEv.ch[4] payload, including NUL handling, printable UTF-8 scalar boundaries, exact field ownership, and 128-byte total capacity before a single append.
- Keep the existing one-scalar API and forward it to the same admission routine without changing object layout or JNI-facing data structures.
- Route retained WGL synthetic typed events through the centralized atomic method. Invalid UTF-8, overlong input, stale focus and full fields do not mutate the preview or revoke painted hit geometry.

## Regressions

- Portable native core test covers a real recovered Auto Tool text field, ASCII and UTF-8, invalid suffixes, rejected over-capacity multi-character event at 127 bytes, exact 128-byte fill, overflow refusal, UTF-8-safe deletion, and blur.
- Windows retained WGL test covers paired source key down/up events with 127-byte and 128-byte frames, no partial prefix after rejected input, and unchanged original profile snapshots.

## Limits

These are first-party source-informed preview model and disposable WGL regressions only. This change does not install physical Minecraft input hooks, activate gameplay modules, or imply original Slinky parity. Chat 2 PR #2 is untouched. Exact-commit CI verification is recorded separately; until complete, it is not a tested release.

## Exact-commit GitHub CI

On corrected R235 code commit e36248808bfefc1aa31a26d1276e89e6bc247f9e, all **8/8 GitHub checks completed successfully**, across three workflows:

- Offline C++ core: Windows Debug/Release, Ubuntu Debug/Release, JNI smoke and sanitizers — 6/6 successful: https://github.com/Trexzo/slinkywinky/actions/runs/43894739443
- Windows x64 native JNI preview bridge — successful: https://github.com/Trexzo/slinkywinky/actions/runs/43894740645
- Injectable Windows Java 8 agent / standalone native loader — successful: https://github.com/Trexzo/slinkywinky/actions/runs/43894739409

The first R235 commit failed one Windows compile fixture because the test variable `near` is an MSVC reserved identifier. The corrected commit renamed that local variable to `nearFrame` without changing production code. Every check passed on the corrected source.

Passing fixture tests does not establish real original-client user input, visual interaction, original gameplay modules, or full injection fidelity. PR #1 remains draft and unmerged.
