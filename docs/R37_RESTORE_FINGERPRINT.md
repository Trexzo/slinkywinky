# R36/R37 — verified live Minecraft tick callback and original-bytecode fingerprint

## R36 real-runtime acceptance (2026-10-31)

On the connected Windows 11 desktop, using an **isolated temporary** Minecraft 1.8.9 Java 8 Forge development process (Forge acted only as the test game launcher), the **independent standalone injector** with R35 callback admission was tested. The Minecraft log contained:

- `CALLBACK_CLASSLOADER_VALIDATED`
- `GAME_THREAD_HOOK_INSTALLED`
- `GAME_THREAD_TICK_ENTRY_PASS thread=Client thread`
- `GAME_THREAD_TICK_20_PASS ticks=20`
- `GAME_THREAD_HOOK_CLEANUP_PASS`

No new Minecraft crash report was generated. A second experiment injected the same read-only hook **twice into one live Minecraft JVM**; each installation reached 20 game ticks and cleanup, then the game remained running, producing `R36_REPEATED_MINECRAFT_HOOK_PASS`. Existing Minecraft installation and unrelated Java processes were not modified. The test game was shut down after acceptance.

## R37 restoration assertion

The agent now captures SHA-256 of the original class bytes before modifying a single mapped method. At cleanup, a pass-through transformer observes the class bytes that are about to be restored. It requires an **exact match** to the original fingerprint and only then emits `CLASS_BYTECODE_RESTORE_PASS` and `GAME_THREAD_HOOK_CLEANUP_PASS`. On any restoration failure, it reports a failure and locks out further instrumentation in that process rather than claiming clean unload.

The disposable Java 8 integration test also requires the fingerprint-restoration marker. The pinned ASM library and parent class-loader admission remain unchanged.

**Status:** R36 live callback and repeated attach passed. R37 fingerprint check requires its own in-game acceptance before claiming a verified fingerprint match. These are diagnostics only: no combat, packet/input modifications, GUI, or authenticated original Slinky parity is implemented.

## R37 in-game acceptance — PASS (October 31, 2026)

Using the isolated Windows Minecraft 1.8.9 client, two successive opt-in hooks passed 20 client-thread callbacks each. Both cleanup passes logged `CLASS_BYTECODE_RESTORE_PASS` with identical fingerprint `3b0f153885c76604d3fa75e2d6676711ac85041f575bd5afcf9b4b6f49d73b54`, followed by `GAME_THREAD_HOOK_CLEANUP_PASS`. No new crash report was detected, and the game remained running through both cycles. The test game process was shut down afterward. The test only verifies this exact development runtime and has not been exercised against a vanilla, obfuscated production client.
