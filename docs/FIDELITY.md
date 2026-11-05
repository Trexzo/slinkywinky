# Fidelity and acceptance criteria

## Reference inputs (not distributed)

- Candidate decrypted native DLL supplied in chat; official provenance unverified.
- Java OpenSlinky archive (previous reconstruction).
- Native C++ Slinky archive (previous reconstruction).
- The OpenSlinky+ v2.2 project is intentionally **excluded**.

These materials guide independent behavioral testing; none of their proprietary executables, fonts, binary memory images, or original recovered tables should be committed here.

## Evidence tiers

1. **Data match:** constants/settings are equal to the supplied reference.
2. **Logic match:** deterministic state transitions agree under identical inputs.
3. **Integration match:** JNI/JVMTI callbacks, frame sequence and packet semantics agree.
4. **Runtime match:** actual Minecraft 1.8.9 offline playthrough, shutdown and reentry pass.
5. **Official equivalence:** requires independently authenticated commercial version; not claimed.

Current branch is **tier 2 only** for tested Reach/RNG state models. The rotation and synthetic-input arbitrators are original defensive infrastructure, not claimed recovered code.

## Next development gates

- Implement a standalone Windows build with declared toolchain and no hidden binary dependencies.
- Record reproducible offline replay fixtures for click press/release edges, Aim Assist target/rotation decisions, Velocity packets, Backtrack hold/flush, AutoBlock and Reach.
- Compare equivalent inputs at module boundaries to avoid misattributing language/runtime effects.
- Add input ownership, packet queue flush and world-disconnect regression tests.
- Validate initial settings independently from saved profiles.
- Test Minecraft 1.8.9 only in authorized isolated worlds with non-sensitive accounts.
- Verify safe shutdown and reinjection; never assume passing compilation implies runtime safety.

## Building the offline core

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

A green result proves the offline decision models build and pass, **not** that this repository is an injectable or playable Minecraft client.
