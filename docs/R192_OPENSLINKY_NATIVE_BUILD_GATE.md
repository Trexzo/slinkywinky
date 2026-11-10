# R191 Windows acceptance and R192 pinned native-build preflight

Date: 2026-11-10. This report describes a disposable, isolated Windows
checkout; it does not certify commercial Slinky or Minecraft runtime parity.

## R191 actual compiler and test evidence

- Exact integration commit tested before R192 additions:
  `5008e47ace08de347ca957596f504cc1bb551768`.
- Windows x64, Visual Studio 2026 MSVC 19.51, CMake 4.4.2.
- The independent `reference_compat/` project configured and compiled.
- **Debug: 69/69 CTest PASS, exit code 0.**
- **Release: 69/69 CTest PASS, exit code 0.**
- Includes the R191 observer reentrancy, mid-batch teardown, and exception
  cleanup scenarios embedded in `r174_native_event_replay`.
- An isolated checkout was created under Windows Temp. No pre-existing
  SlinkyWinky working tree, live Minecraft JVM, injector or game input
  path was modified or exercised.

## R192 source and prerequisite evidence

- Independently cloned `Skid-Technology/OpenSlinky` from GitHub into
  another disposable Windows Temp directory, without invoking its DLL.
- HEAD matched pinned `fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4`.
- All six required build-source sentinel paths were present; no original
  commercial executable or private decrypted candidate was needed.
- `python -m unittest discover -s tests -p test_openslinky_native_toolchain.py -v`
  **6/6 synthetic cases PASS** on Python 3.12.10.
- Tool presence probe found `cmake`; the following *exact tool names* were
  not found on the Windows **PATH**: `ninja`, `clang`, `clang-cl`,
  `lld-link`, `llvm-rc`, `llvm-mt`, `llvm-lib`, `llvm-dlltool`.
  Absence from PATH does not prove the tools are nowhere on disk.
- The upstream Git submodule helper also hit a local Git/MSYS fork failure;
  the separate direct clone successfully established the pinned source SHA.
  This does **not** imply the upstream C++ source failed to compile.
- `toolchain_ready=false`; the upstream DLL was neither linked nor run.

## Reproduction and acceptance boundaries

Use the **existing integration branch**, not main or the other chat's
active R-series runtime branch. From the repository checkout:

```powershell
python -m unittest discover -s tests -p 'test_openslinky_native_toolchain.py' -v
python tools/probe_openslinky_native_toolchain.py --root references/OpenSlinky --json-out native_toolchain_preflight.json
# When all tools have been provisioned and the pinned source checked out:
python tools/probe_openslinky_native_toolchain.py --root references/OpenSlinky --strict
cmake -S reference_compat -B build-reference
cmake --build build-reference --config Debug
ctest --test-dir build-reference -C Debug --output-on-failure
cmake --build build-reference --config Release
ctest --test-dir build-reference -C Release --output-on-failure
```

The informational probe deliberately returns zero when tools are missing;
the `--strict` variant returns nonzero until prerequisites are present.
`toolchain_ready=true` means prerequisites *appear available*, not that an
actual DLL was compiled. Any native build must still be validated separately.

## Not established

- No matching code-generation equivalence to the original decrypted DLL.
- No approved reuse/redistribution rights for the third-party source tables.
- No injected OpenSlinky DLL, JNI/JVMTI functional test or 1:1 GUI parity.
- No original Minecraft 1.8.9 gameplay module execution.
- GitHub Actions status must be checked on the final R192 head.

Existing R190 coverage accounting remains **66 candidate descriptors, 171
native callback subscriptions, 13 first-party offline module decisions, 15
executable callback bindings, and 156 subscriptions without implementations**.
PR #2 stays draft and unmerged.
