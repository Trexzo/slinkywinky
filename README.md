# SlinkyWinky — independent reconstruction laboratory

**Status: R2 candidate — offline C++ state models and a host-callable library. Not a Minecraft client yet.**

This is a new C++17 implementation intended to make the behavior and failures of a Slinky-like Minecraft 1.8.9 client inspectable. Its algorithms are **experimental reference-inspired models**; passing tests is not proof that the official product has been reproduced. The user-supplied Java, C++ and candidate-decrypted archives are not published here. OpenSlinky+ v2.2 is intentionally excluded.

## What exists

- Buildable static C++ library plus a shared C ABI library (`slinkywinky_bridge`).
- Reference-profile starting values, deterministic xoshiro128+ random generator, Reach cooldown.
- Tick-based Auto Clicker, Aim Assist, AutoBlock, HitSelect and Velocity decision models.
- Bounded thread-safe command mailbox, single-owner consumer, generation fencing on world changes.
- Bounded packet-notice queue with explicit expiry/release/drop semantics, coordinated through a host-session lifecycle.
- User/manual versus synthetic mouse-button ownership, safe disable and world reset.
- Validated text profile with versioning, forward-compatible unknown keys and backup on replacement.
- Standalone offline trace demo, portable C ABI contract, GCC/Clang/Windows/Linux regression builds.

## Build and test

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

`build/slinkywinky_offline_demo [profile.ini]` emits an example CSV tick trace. This does not send Minecraft packets or change another process. Shared library entrypoints are in `include/slinkywinky/bridge.h` and require a caller to provide validated world snapshots from the correct game thread.

## Not yet implemented or certified

A production Minecraft 1.8.9 host adapter, JVM injection/JVMTI hook registration, accurate original packet codecs, game GUI/HUD, the remaining combat/visual/world modules, Minecraft-specific runtime playthrough, real game launch and unload/reinject acceptance. A **compiled shared library cannot be used as a client without the missing host adapter**.

The goal is a reliable independently authored client; never commit an original/decrypted commercial binary, protected asset, commercial font, or account credential. See [fidelity criteria](docs/FIDELITY.md), [R2 limitations](docs/R2_ACCEPTANCE.md), and [all candidate module coverage](docs/MODULE_MATRIX.md).
