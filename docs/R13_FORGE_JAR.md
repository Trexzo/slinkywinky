# R13 Forge 1.8.9 build and artifact

The experimental read-only Forge 1.8.9 adapter now has its own CI pipeline for the full mod JAR, using the legacy Java 8 / Gradle 2.14.1 / ForgeGradle 2.1 environment.

The pipeline checks that the JAR contains ForgePreview.class, excludes standalone test mains, calculates SHA-256, and uploads the JAR only after its build and checks pass.

The Windows JNI DLL is built separately by windows-jni.yml. Neither build is an installer or a complete Minecraft client. The native library is not embedded into the JAR.

Do not claim gameplay parity or production readiness without an actual Minecraft 1.8.9 launch, mod initialization, HUD rendering, native library loading, F9 toggling, world changes, and clean game shutdown.

Reference binary material remains excluded, and OpenSlinky+ v2.2 is not used.