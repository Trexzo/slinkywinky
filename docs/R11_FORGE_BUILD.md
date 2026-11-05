# R11 Forge 1.8.9 adapter build gate

This optional adapter is **read-only** and does not send packets, move the mouse,
rotate the camera or press keys. No original Slinky components are redistributed.

## Reproducible Java 8 build

GitHub Actions workflow: \`.github/workflows/forge189.yml\`

- Temurin JDK 8
- Gradle **2.14.1** (not modern Gradle 8/9)
- ForgeGradle **2.1-SNAPSHOT** and Forge **1.8.9-11.15.1.2318-1.8.9**
- Minecraft \`stable_22\` mappings
- Command: \`gradle --no-daemon -p experimental/forge189 compileJava --stacktrace\`

This build downloads legacy Forge/Minecraft dependencies from their designated
repositories. If repositories are unavailable, that is a **dependency resolution
failure**, not evidence that the adapter's Java code compiled.

The shared runtime sources are imported from \`../../java\`; test mains are
excluded from the mod JAR. No native JNI shared library is packaged: preview
must fail closed with an explicit message when the library is absent.

A passing \`compileJava\` is still **not** a real in-game Minecraft acceptance
test. No GUI beyond the diagnostic overlay has been implemented.
