# R39: actual Java game-thread → C++ engine zero-action bridge

The opt-in `--hook-probe` now passes the absolute path of the separately built first-party `slinkywinky_jni.dll` to the Java 8 Instrumentation agent. **Before installing a callback**, it loads that JNI library and invokes `nativeZeroActionTick()` with a no-target frame and all modules disabled, rejecting any nonzero gameplay action. The JNI method is exported by the project's C++ source and calls its existing SWContext decision engine.

During a bounded 20-tick test the same read-only native proof runs on the actual Minecraft callback thread. Logs `NATIVE_CXX_PREFLIGHT_PASS` and `NATIVE_GAME_THREAD_CXX_PASS` distinguish native execution from the earlier Java-only callback. Any JNI load/ABI failure rejects installation or triggers cleanup without applying gameplay actions.

The standalone CI package now includes and hashes `slinkywinky_jni.dll`. The ordinary native `attach` and passive `--instrument-probe` are unchanged. Experimental hook use still requires both the explicit `--hook-probe` option and `SLINKYWINKY_EXPERIMENTAL_HOOK=1`.

**Not gameplay ready:** This executes a disabled decision model only; it does not read live combat state, change aiming/inputs/packets, render GUI/HUD or establish original commercial Slinky equivalence. Needs its own Minecraft 1.8.9 test.
