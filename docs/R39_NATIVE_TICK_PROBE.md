# R39: actual Java game-thread → C++ engine zero-action bridge

The opt-in `--hook-probe` now passes the absolute path of the separately built first-party `slinkywinky_jni.dll` to the Java 8 Instrumentation agent. **Before installing a callback**, it loads that JNI library and invokes `nativeZeroActionTick()` with a no-target frame and all modules disabled, rejecting any nonzero gameplay action. The JNI method is exported by the project's C++ source and calls its existing SWContext decision engine.

During a bounded 20-tick test the same read-only native proof runs on the actual Minecraft callback thread. Logs `NATIVE_CXX_PREFLIGHT_PASS` and `NATIVE_GAME_THREAD_CXX_PASS` distinguish native execution from the earlier Java-only callback. Any JNI load/ABI failure rejects installation or triggers cleanup without applying gameplay actions.

The standalone CI package now includes and hashes `slinkywinky_jni.dll`. The ordinary native `attach` and passive `--instrument-probe` are unchanged. Experimental hook use still requires both the explicit `--hook-probe` option and `SLINKYWINKY_EXPERIMENTAL_HOOK=1`.

**Not gameplay ready:** This executes a disabled decision model only; it does not read live combat state, change aiming/inputs/packets, render GUI/HUD or establish original commercial Slinky equivalence. Needs its own Minecraft 1.8.9 test.

## R39 real Minecraft 1.8.9 acceptance — PASS

In the isolated Windows Java 8 Minecraft 1.8.9 development client, two consecutive `--hook-probe` attachments to the same JVM both logged `NATIVE_CXX_PREFLIGHT_PASS`, `NATIVE_GAME_THREAD_CXX_PASS thread=Client thread`, `GAME_THREAD_TICK_20_PASS ticks=20`, and `CLASS_BYTECODE_RESTORE_PASS`. Each attachment generated no gameplay actions; no new crash was detected. Both restoration digests matched within the same JVM: `1f81b87d379f71aee9ad84af845be3c47efd6c4234e58113eba07029641f13df`. The game remained running through both tests and the isolated launcher was shut down afterward.

GitHub's standalone injector, Windows JNI, and C++ core workflows all passed on commit `4fe077c`. The CI download ZIP was independently checked: its five deployed files passed CRC and the internal SHA-256 manifest. The new `tests/windows_minecraft_tick_acceptance.py` requirement for native markers is a manual-only in-game gate, not part of ordinary hosted CI.
