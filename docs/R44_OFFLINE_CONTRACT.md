# R44 — offline-only JNI observation contract and fail-closed session consistency

This milestone does **not launch Minecraft**. It adds strict consistency checks in Java and JNI for the read-only game observation path: player-without-world flags are rejected before native state evaluation. The sampler also rejects such inconsistent snapshots before JNI dispatch.

A new **standalone disposable Java 8** fixture invokes the first-party JNI library directly through the already-built InstrumentationProbe class. It verifies a persistent single-thread native context through menu, world, player, target, disconnect and new-session frames. It checks malformed flag masks, impossible player/world states, null and incorrectly sized arrays, negative timestamps, zero generation, NaN/Infinity values, cross-thread access denial, context survival after rejected calls and correct owner-thread closure.

The fixture is incorporated into CTest `native_agent_attach`; no Minecraft, Forge, actual game process or user's `.minecraft` folder is involved. The test does **not** certify a real Minecraft session or original Slinky parity.

All combat settings remain disabled. The experimental `--hook-probe` remains developer-gated.
