# R58 — Official Slinky documentation vs supplied decrypted reference (research-only)
Date: 2026-11-01. Scope: source provenance, module inventory, and implementation requirements for SlinkyWinky. No proprietary binaries or source excerpts are committed here.

## Sources and trust boundaries
- Official site: https://slinky.gg/
- Requirements: https://slinky.gg/requirements/
- User documentation: https://docs.slinky.gg/basics/get-started/
- Module reference: https://docs.slinky.gg/modules/combat/aim-assist/ (and analogous modules)
- GUI reference: https://docs.slinky.gg/modules/visual/gui/
- Official updates: https://slinky.gg/changelog/ (latest dated entry observed: 2026-07-26)
- Local, user-supplied candidate reference archive: `decrypted files.zip`, **not** committed. SHA-256: `326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4`.
- Embedded `decrypted files/slinky.dll` SHA-256: `0809dc167275816e803362318b8679ea0be393f1b39cc94f90c68722544cd441`.
- No chain of custody establishes that the candidate DLL is an authenticated official Slinky release. Internal analysis claims are not independently equivalent to verified runtime behavior.

## Official product target
- Injectable desktop client, **not** a Forge-dependent build.
- Minecraft 1.8.9 and 1.7.10, Vanilla/Forge; third-party Lunar/Badlion compatibility best-effort.
- Windows 10/11 x64; Linux officially supports Arch Linux.
- Desktop loader, injected runtime, menu notification with default RSHIFT, category navigation, toggle/hold binds, unload, GUI.
- CTRL-click numeric slider entry; module settings saved via profiles, GUI preferences auto-saved separately.
- Current GUI exposes Compact/Wide layout, scale, accent color, allow input while open.
- Latest dated official changelog entry observed is July 26, 2026, including new Timer Range Legit mode and Criticals Boost.

## Local reference static inventory (read-only)
ZIP member count: 185; uncompressed aggregate 271,554,326 bytes.
- Embedded PE DLL: 40,210,672 bytes; x64 MZ/PE header.
- Recovered `.text`: 1,295,174 bytes, SHA-256 `35165183210c4f50cde535dc98b806b29ed7efd8fb3da3822f416915559302af`.
- Recovered `.rdata`: 1,727,316 bytes, SHA-256 `61eaa6bad74fd5cf0d374198506ec4f87fa042b5cf673e37ae01e87eb336706a`.
- Recovered `.data`: 136,712 bytes, SHA-256 `8aee98d473f5092686ea3ed080828bbb02500e4bbd8d89130a3ce7751176dea5`.
- Recovered `.slinky0` blob: 14,255,293 bytes, including incompletely understood protected runtime structures.
- `re/modules_real.json` contains 67 native module descriptors. Their names match all **65** modules documented in the official current sidebar, with exactly two additional internal/not-publicly-documented names: `Debug`, `Chest Stealer`. No conclusion is warranted about whether these are functional, deprecated or hidden.
- Count by recovered descriptor category: Combat 17, Move 10, Visual 15, Block 7, Utility 18. Public documentation counts: Combat 17, Move 10, Visual 15, Block 7, Utility 16.
- `src/gen/` contains generated native UI/settings registration functions. Assembly and low-level binary image are evidence inputs, not complete maintainable application source.

## Reproducibility caveats
The included `README.md` claims `src/logic/*`, `manifest.json`, `tools/verify.py`, `tools/verify_logic.py`, `tools/extract_logic.py` and reports proving byte-exact recovery. These exact named items are **absent** from the delivered ZIP. Its `build.sh` instead invokes `tools/build_all.py`. Do not repeat the README's '200/200 exact' claim as an independently reproduced result.
The decrypted flat image is explicitly **not a runnable executable**; the research notes say the protector's startup/import metadata was removed. No authenticated original loader EXE is supplied with this decrypted archive. The separately supplied reconstructed injector is not automatically evidence of the official loader architecture.
The original version/build date is not independently authenticated. New website features may postdate the candidate binary. Compare exact enum labels/defaults/modes before assuming 2026 behavior.

## Native injectable project status (branch evidence)
`reconstruction/r1-core-fidelity` / draft PR #1 has a Windows Java 8 Attach/Instrumentation loader and native C++ engine; Forge is only optional test infrastructure.
R48 documents **two** read-only hooks into original obfuscated vanilla 1.8.9 at the main menu, with 20 client-thread callbacks and restored fingerprints. R57 adds opt-in external HUD telemetry. Neither proves a persistent full client or game-world module behavior.
Do **not** claim real Slinky parity, live in-world behavior, clean DLL unload, Windows Lunar/Badlion compatibility, 1.7.10, or Linux until independently tested.

## Reference-driven acceptance order
1. Freeze hashes and record source provenance / feature version. Preserve original materials offline, read-only.
2. Build table per public module: official settings/modes, reference descriptor/defaults/callback evidence, independent implementation, deterministic behavioral tests, runtime state, and known changes since candidate build.
3. Validate 1.8.9 standalone injector lifecycle in disposable Vanilla JVM: identified target, attach, persistent bounded client-thread observation, safe clean stop and bytecode restoration; no silent arbitrary-process targeting.
4. Validate world/player/target state and visible lifecycle in a private offline test scene; move from zero-action engine to one independently implemented, precisely verified module at a time.
5. Implement first-party in-game GUI, profiles, binds, notification, and safe unload. Compare against official visuals and interaction rules.
6. Expand compatibility to other claimed environments only with separate reproducible evidence.
7. Do not redistribute any reference commercial binary, decrypted image, extracted assets, or fonts. Keep all claims labeled as source-derived, inferred, or runtime-verified.

Conclusion: official website plus this reference provide an excellent **specification and research baseline**, but are not sufficient proof of complete source authenticity, original loader code, or finished runtime equivalence.