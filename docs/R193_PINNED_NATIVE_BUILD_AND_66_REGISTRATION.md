# R193 — Pinned native OpenSlinky builds and 66-registration correction

Date: November 10, 2026. Scope: offline compilation and static inspection only.

## Toolchain and source provenance

- Pinned upstream: Skid-Technology/OpenSlinky @ fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4.
- Official LLVM 23.1.5 portable Windows x64 archive (not an installer). Verified SHA-256:
  b2be33385ddc86e83511417a060de927ae8b09d1f3017cc3cdce5c73cd5029d7.
- The toolchain was extracted into Windows Temp with installed 7-Zip 26.02 after Windows bsdtar refused the large Zstandard decompression window.
- LLVM binaries used: clang, clang-cl, lld-link, llvm-rc, llvm-mt, llvm-lib, llvm-dlltool. Ninja was provided by Visual Studio 2026 and CMake 4.4.2 was already available. No global PATH or MSI changes.
- The strict native-toolchain preflight passed with all required source sentinels and the exact upstream SHA.

## Unmodified native reference

- Isolated Windows x64 CMake/Ninja configure: PASS.
- Build: 219/219 commands, exit 0, produced slinky.dll (2,395,136 bytes).
- DLL SHA-256: 78bbf5013cd9d98650565cf0c14d01678fb88b54a26f2aacbd31152d04a91883.
- llvm-readobj confirmed a PE32+ AMD64 DLL, six sections and a nonzero entrypoint. Import library names: kernel32.dll, user32.dll, shell32.dll, ucrtbase.dll. Export table absent.
- The unmodified native manager still has an observed registration-count discrepancy: 67 versus 66 original-reference entries.

## Source-guarded registration correction

- The independent script at tools/prepare_openslinky_registration_66.py verifies the upstream SHA, expected module_manager.cpp SHA-256 (d65288ae2c890db7dd09b9783ac5cad7090bd5c32908c07dc3d50061f837de96), 67-entry descriptor table and the extra chest_stealer_module_descriptor at index 17.
- It writes only an adjusted manager source file *outside* the upstream tree, with no upstream code checked in.
- A private 66-entry manager copy now concatenates the first 17 original pointers and the remaining 49 after skipping index 17 (136 + 392 = 528 bytes), and changes loop, allocation and queue-capacity bounds to 66. The original upstream static data array remains unchanged.
- The resulting manager source SHA-256: 771faf3bbd2c027a56110f6b9ac4941cc0dcf1a026bd8069bf4451f3bd5fbf3a.
- Six synthetic drift, bounds, source-integrity and anti-overwrite tests PASS on Windows and are included in the separate public reference-audit workflow.

## Independently compiled corrected native DLL

- A distinct disposable clone of the same pinned source was used. Only src/engine/jni/module_manager.cpp differed.
- Patched candidate CMake configure: PASS. Build: 219/219, exit 0.
- Resulting slinky.dll size: 2,395,648 bytes.
- DLL SHA-256: 3613a466433fb1fa83d2d9e96815ece719c166f881c78642dc18d3e13ae2aa14.
- Static PE inspection: x64 Windows DLL, six sections, same four import-library names as the unmodified binary. Not a runtime or behavior test.
- The upstream checkout remains clean; the adapted source and DLL reside only in local Windows Temp. No original encrypted/decrypted executable, proprietary table or upstream implementation source has been committed.

## Reproduction overview

1. Obtain and verify the exact pinned public upstream commit in a clean UPSTREAM directory, and clone it separately into a disposable CANDIDATE directory.
2. Run the synthetic tests: python -m unittest discover -s tests -p test_openslinky_registration_66.py -v.
3. Generate the adjusted file: python tools/prepare_openslinky_registration_66.py --reference-root UPSTREAM --output-file PATCHED_MANAGER_CPP.
4. Copy PATCHED_MANAGER_CPP to the module_manager.cpp location in CANDIDATE only. Do not modify UPSTREAM.
5. Prepend the verified LLVM bin directory and Ninja to the *process-local* PATH. Configure with the pinned upstream Windows clang-cl toolchain; build with CMake/Ninja.
6. Inspect the DLL only with llvm-readobj. Do NOT load or inject the binary merely to claim success.

## Unresolved acceptance

The patched program has not been loaded, injected or executed in Minecraft; 66 active handlers and the priority ordering have not been authenticated at runtime. Third-party source redistribution rights remain unresolved. Further work requires independent private decrypted-reference checks, isolated lifecycle and real Minecraft acceptance, including JNI/JVMTI hooks, GUI, input, unload, profile persistence and module functionality. Compilation alone is not 1:1 parity.

The inventory remains 66 original descriptors / 171 native subscriptions, with 13 first-party offline decisions and 15 bound callbacks. No gameplay module was promoted by this build. Keep PR #2 draft and separate from PR #1.
