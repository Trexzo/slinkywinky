# R14 experimental headless Forge launch

The core and Forge JAR compile successfully, and Windows x64 JNI smoke passed.
R14 separately tests whether a real Minecraft 1.8.9 Forge development client can launch in a Linux virtual display and execute the SlinkyWinky read-only mod initializer.

The workflow requires both a console initialization marker and a process that remains active until a controlled six-minute timeout. A successful compile is insufficient.
Runtime logs are attached even on failure. Old ForgeGradle, LWJGL, game assets and OpenJDK 8 compatibility failures should be analyzed independently of the core code.

The game is not authenticated with user credentials, and no real-world packet or combat action is enabled. The F9 preview remains read-only.
Passing headless initialization still does not verify camera rendering, native DLL loading on Windows, manual F9 interaction or actual combat behavior.
