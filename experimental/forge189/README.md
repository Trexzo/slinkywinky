# Experimental Minecraft Forge 1.8.9 read-only preview (R10)

**Built by the separate GitHub ForgeGradle 2.1 / JDK 8 workflow, but not
yet launched or validated in a real Minecraft 1.8.9 game session.**
The reobfuscated preview JAR is an optional, read-only integration candidate,
not a playable Slinky replacement; no proprietary game jar is bundled.
R280 checks that ForgePreview, ForgeRelationshipRosterPort and
RelationshipRosterSampler classes are actually packaged.

- Intended JVM: Java 8, Forge 1.8.9 (11.15.1.2318).
- ForgeGradle 2.1 / Gradle 2.x required; build network and dependencies are not
  available in the local audit environment.
- Requires a separately built slinkywinky_jni native library on java.library.path.
- F9 toggles the preview; outputs are text-only and **never change input, aiming,
  reach, network packets or gameplay**.
- Missing native library or invalid observations disables preview rather than
  enabling incomplete behavior.

**Acceptance pending:** actual 1.8.9 game launch,
Minecraft client-thread callbacks, world swaps, disconnect/shutdown, OptiFine
coexistence, HUD rendering and safe disable.

This integration must not be used to claim authenticity or 1:1 behavior.

## R16 event cadence

F9 input toggling remains on the fixed client tick. Read-only decision sampling now runs at the RenderTickEvent END phase (render-frequency), which avoids limiting click-edge timing observations to 20 TPS. This improves diagnostic resolution but is not yet proof of any in-game action timing or commercial Slinky parity.
