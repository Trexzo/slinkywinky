# SlinkyWinky

**Status: early development; not a playable client.**

A source-grounded, independently maintained Minecraft 1.8.9 client reconstruction project. Work proceeds in small, reviewable commits with deterministic tests and transparent fidelity claims.

The immediate objective is to establish an offline-testable C++ core, then integrate a separately validated Windows/JVM runtime and interface. Comparisons against the user's supplied Java/C++ archives and candidate decrypted DLL are research inputs, **not proof of official provenance or runtime parity**.

## Safety, provenance and publication policy

- Do **not** commit original/decrypted proprietary DLLs, injectors, extracted commercial fonts/assets, memory dumps, credentials or redistribution-restricted binaries.
- Keep reference archives local and out of version control.
- Document independently authored behavior and clearly distinguish reference-backed facts from reconstruction hypotheses.
- Do not claim functioning gameplay or equivalence without in-game validation and reproducible evidence.

The initial development work is tracked through draft pull requests. No source from the reference DLL is bundled in this repository.
