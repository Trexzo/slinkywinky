# R51 — release sampled world references and serialize all instrumentation

The read-only sampler previously held static `oldWorld`/`oldPlayer` references after the 20-tick probe had ended. In an in-world attach this could retain entire old world/player object graphs unnecessarily until another attach or JVM shutdown.

R51 adds an explicit, synchronized `ObservedState.release()` that clears all reflective target accessors, world/player identities and mapping profile. The game-thread callback calls it **after native session closure** and before class-bytecode restoration. The agent emits `OBSERVATION_RELEASE_PASS`; the disposable Java fixture checks that sampling fails after release and that a subsequent clean admission succeeds.

Additionally, **passive** `--instrument-probe` and **active** `--hook-probe` now share the same admission claim, so a pass-through retransform cannot accidentally remove the active hook. The admission regression simulates both a second hook attempt and a passive probe while already claimed and asserts neither operation mutates active world/session state. Pre-transform failures clear the unused claim; uncertain post-transform failures remain locked out.

Every path remains read-only and developer gated. The new R51 source is not yet tested inside an actual Minecraft process; the historical R48 live acceptance applies to earlier binaries only.
