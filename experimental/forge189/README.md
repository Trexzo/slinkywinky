# Experimental Minecraft Forge 1.8.9 read-only preview (R10)

**Not yet built or launched against actual Minecraft/Forge.** This is an optional
integration candidate, deliberately excluded from primary R1-R9 CI. It is not a
playable Slinky replacement, and no proprietary Minecraft/game jar is bundled.

- Intended JVM: Java 8, Forge 1.8.9 (11.15.1.2318).
- ForgeGradle 2.1 / Gradle 2.x required; build network and dependencies are not
  available in the local audit environment.
- Requires a separately built slinkywinky_jni native library on java.library.path.
- F9 toggles the preview; outputs are text-only and **never change input, aiming,
  reach, network packets or gameplay**.
- Missing native library or invalid observations disables preview rather than
  enabling incomplete behavior.

**Acceptance pending:** clean Java 8 Forge compilation, actual 1.8.9 launch,
Minecraft client-thread callbacks, world swaps, disconnect/shutdown, OptiFine
coexistence, HUD rendering and safe disable.

This integration must not be used to claim authenticity or 1:1 behavior.
