# R12 Windows JNI preview artifact

The optional slinkywinky_jni.dll is built from this repository's original C++ test core. It is **not** an original Slinky DLL, vendor source recovery, or injector.

The Windows GitHub job uses MSVC x64, Temurin Java 8, CMake Release, and CTest. An artifact is uploaded only if all tests pass.
CTest receives the correct Windows multi-config Release DLL directory using the CMake TARGET_FILE_DIR generator expression. The smoke Java source uses Java 8 compatible -source/-target flags.

For the experimental read-only Forge preview, users must make this independently built DLL available to their Java JVM using a trusted java.library.path setting. It must not be renamed as or substituted for the vendor DLL.

Neither DLL compilation nor successful JNI smoke testing establishes a functioning Minecraft 1.8.9 client. Full in-game acceptance remains required.
