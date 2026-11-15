# R194 — Compiled 66-entry module-manager verification

Date: 2026-11-10. This is an offline, non-executed native analysis track.

## Pinned sources and local R193 builds

Upstream OpenSlinky commit: fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4.
Upstream rdata SHA-256: b17dd02a9f6894d3dd366d559dcc5f3e2db71600571c8108473a1fe57618e3e6.
Toolchain: Clang/LLVM 23.1.5, CMake/Ninja, Windows AMD64.

| Metric | Unmodified OpenSlinky | Corrected candidate |
| --- | --- | --- |
| DLL size | 2,395,136 bytes | 2,395,648 bytes |
| DLL SHA-256 | 78bbf5013cd9d98650565cf0c14d01678fb88b54a26f2aacbd31152d04a91883 | 3613a466433fb1fa83d2d9e96815ece719c166f881c78642dc18d3e13ae2aa14 |
| Linked static pointers | 67/67 verified | 67/67 verified |
| PE image descriptor-table RVA | 0x8a810 | 0x8a810 |
| Compiled manager count | 67 | 66 |
| Compiled pointer copies | 536 bytes contiguous | 136 + 392 bytes (skip index 17) |
| Compiled queue capacity | 67 | 66 |

PE32+ AMD64 parser checked all 67 linked static pointer values against their exact public linker-map symbol addresses. No pointer mismatch in either build. The public static data table remains 67 entries in both DLLs. Chest Stealer remains in that data table at index 17 but is skipped when the corrected constructor builds its private 66-entry list. The compiled object also shows constructor allocation and loop bounds at 66 and both queue checks at 66.

## New independent verification tools

- tools/verify_compiled_registration_66.py: read-only PE32+ parser, link-map reconciliation, COFF disassembly checks and metadata-only audit output. Fails closed for a missing or corrupt descriptor pointer, wrong layout, missing symbol, unexpected compiler instruction pattern or source fingerprint.
- tests/test_compiled_registration_66.py: nine synthetic tests, including deliberate pointer swaps, missing symbols, duplicate descriptor map conflicts, invalid PE architecture/section, wrong original index, old 67-entry memcpy and wrong queue bound.
- .github/workflows/openslinky-reference-audit.yml runs only the synthetic tests on public CI. It does not build, load or distribute the third-party DLL.

Local Windows findings: nine of nine synthetic tests PASS, and the real 67/66 compiled comparison exits with code 0. Full evidence JSON was saved at the local temporary location:
%TEMP%/slinkywinky-r193-llvm-portable/r194_compiled_registration.json.

Run the synthetic test in the integration branch with:
python -m unittest discover -s tests -p test_compiled_registration_66.py -v

Run the comparison against independently compiled local builds with:
python tools/verify_compiled_registration_66.py --upstream UPSTREAM --original-build BUILD67 --corrected-build BUILD66 --llvm-objdump PATH_TO_LLVM_OBJDUMP --json-out r194_compiled_registration.json

UPSTREAM is the exact pinned public source checkout. BUILD67 and BUILD66 must contain slinky.dll, slinky.map and the compiled module_manager.cpp.obj file. No original commercial files are required for this static audit.

## Limits and next gates

This confirms the corrected *compiled* module constructor and linked 67-element static descriptor layout, not in-memory runtime initialization or gameplay function equivalence. The native manager has not been executed. Full 66-entry callback dispatch, equal-priority ordering, JNI/JVMTI hook lifecycle, GUI appearance, input ownership and safe unload are unverified. Only a separately authorized supervised game test could establish runtime behavior, and commercial 1:1 equivalence still requires authenticated original-release evidence.

The candidate DLLs, original/decrypted binary and upstream proprietary data remain outside GitHub. Third-party redistribution rights still require resolution. Existing module inventory remains 66 original descriptors, 171 registered callbacks, 13 first-party offline policies and 15 bound callback decisions. PR #2 stays draft/unmerged and separate from PR #1.
