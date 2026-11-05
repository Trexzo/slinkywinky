# R40: game-thread to C++ end-to-end acceptance gate

The Windows acceptance test now requires **two occurrences each** of `NATIVE_CXX_PREFLIGHT_PASS` and `NATIVE_GAME_THREAD_CXX_PASS` in addition to the 20-tick and bytecode-restoration markers. It refuses ambiguous JVM targets, requires two identical restored class fingerprints and a live Minecraft test process, then terminates only its isolated test process tree.

The observed R39 live result was successful in a ForgeGradle-launched Minecraft 1.8.9 test JVM. **The injector itself has no Forge dependency.** The first-party C++ engine was invoked from the actual Minecraft `Client thread`, with all gameplay actions disabled. This is not a production combat client, and neither a native ClickGUI nor original-Slinky parity has been established.
