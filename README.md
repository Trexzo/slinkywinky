# SlinkyWinky

## Where the development source is

The default `main` branch is intentionally a landing page, **not** the full
client source tree. The actual ongoing implementation lives on
[`reconstruction/r1-core-fidelity`](https://github.com/Trexzo/slinkywinky/tree/reconstruction/r1-core-fidelity)
([draft PR #1](https://github.com/Trexzo/slinkywinky/pull/1)).

A separately reviewed **native C++ reference integration** lives on
[`integration/openslinky-fd9c592-reference`](https://github.com/Trexzo/slinkywinky/tree/integration/openslinky-fd9c592-reference)
([draft PR #2](https://github.com/Trexzo/slinkywinky/pull/2)).
It links the external source through a pinned submodule; it does **not** claim
the external reconstruction is an authenticated or working original Slinky build.

Switch GitHub's branch selector from `main` to the reconstruction branch to
browse the actual C++, Java/JNI, test and GUI sources. Neither draft PR has
been merged into `main` or accepted as a playable release.


**Status: early development; not a playable client.**

A source-grounded, independently maintained Minecraft 1.8.9 client reconstruction project. Work proceeds in small, reviewable commits with deterministic tests and transparent fidelity claims.

The immediate objective is to establish an offline-testable C++ core, then integrate a separately validated Windows/JVM runtime and interface. Comparisons against the user's supplied Java/C++ archives and candidate decrypted DLL are research inputs, **not proof of official provenance or runtime parity**.

## Safety, provenance and publication policy

- Do **not** commit original/decrypted proprietary DLLs, injectors, extracted commercial fonts/assets, memory dumps, credentials or redistribution-restricted binaries.
- Keep reference archives local and out of version control.
- Document independently authored behavior and clearly distinguish reference-backed facts from reconstruction hypotheses.
- Do not claim functioning gameplay or equivalence without in-game validation and reproducible evidence.

The initial development work is tracked through draft pull requests. No source from the reference DLL is bundled in this repository.
