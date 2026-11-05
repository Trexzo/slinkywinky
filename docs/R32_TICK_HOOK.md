# R32: bounded, read-only game-thread instrumentation

The standalone Windows injector supports `--hook-probe` as an **explicit diagnostic**, without Forge. It attaches a Java 8 agent using the standard Attach/Instrumentation APIs and modifies a single recognized Minecraft method to call a small read-only entry callback.

The method must be exactly `runTick()V` or `func_71407_l()V`. Obfuscated names are **not guessed**. The first invocation records the target JVM thread; after 20 calls, a helper thread retransforms the class again with the temporary transformer removed to restore the original bytecode. The callback never changes input, rotation, packets or game state.

Uses the BSD-3-Clause-licensed ASM 5.2 library downloaded at build time with pinned SHA-256 `3e5ea0d7da2c5155ef4f470d9092d42de34e3f53db6589c7c07d6721adf4ba3e`. The ASM library and license are bundled in the Java agent JAR. Missing or altered downloads fail configuration.

The disposable-JVM integration test checks hook installation, 20 callbacks and cleanup; real Minecraft validation is a separate gate. A passing hook probe is **not** a finished native combat or rendering client.

## Experimental admission gate

R35 adds a second explicit developer gate. To use `--hook-probe`, the attaching process must have `SLINKYWINKY_EXPERIMENTAL_HOOK=1`. The ordinary native loader and passive `--instrument-probe` do not need this environment variable. The full in-game tick-hook fix is **still unverified** after the R32 crash; only use the experimental path in a disposable test instance. The standalone ZIP checksum manifest uses paths relative to the artifact root.
