# R20: legacy ForgeGradle asset download workaround

The previous headless startup reached the Gradle `getAssets` step and repeatedly failed to download asset files over obsolete HTTP endpoints. This is an environmental incompatibility of legacy ForgeGradle 2.1, not evidence of a SlinkyWinky module defect.

For the **diagnostic CI launch only**, exclude Gradle task `getAssets` with `-x getAssets`. No unofficial asset mirrors, Minecraft redistributions or credentials are used. The Minecraft client may still fail because of missing assets, and this command is not intended as a user installation process.

Success still requires the real mod-init marker AND sustained game process until the controlled timeout; build success or `runClient` starting alone is never enough. Crash reports and full game logs remain captured.

Forge mod JAR and Windows JNI native library build separately; in-game Windows JNI and UI acceptance remain outstanding.
