# R21 independent Windows x64 test and native warning cleanup

An independent Windows desktop with Visual Studio 2026, Temurin Java 8, CMake and Minecraft Forge 1.8.9 installed was used to clone the published draft branch into a temporary workspace.

The native Release build passed and CTest reported **8/8 PASS**, including Java 8/JNI smoke and 25,000-frame replay. No files under the existing `.minecraft` installation were changed.

The compiler found two nonfatal issues: native Reach returning optional<double> implicitly into optional<float>, and bridge symbols flagged as imported while linked into the same JNI DLL.

This commit adds an explicit finite and float-range conversion for Reach and marks each shared-library target as an exporting C API build using the CMake target macro. The header no longer relies on a single translation unit's hardcoded macro.

This is a source-quality change only. No production Minecraft action adapter, release or official provenance claim is implied.