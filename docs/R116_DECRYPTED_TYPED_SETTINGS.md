# R116 – All 378 recovered setting controls, typed and previewed

**Source:** user-provided `decrypted files.zip`, SHA-256 `326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4`. Recovered control inventory: `decrypted files/re/mods_block.js`, SHA-256 `c39a268dd50516f70bf9eae56192512ade6f3f3b9ba1b4026a7aac204182d707`. This user-supplied candidate archive is internally consistent but **not independently authenticated as an official Slinky release**.

No unknown JS, native binaries, injectors or mods from the archive were executed. Parsing was performed with a restricted tokenizer supporting only JS literals, lists, objects, and the known declarative `C/S/D/R/K/M/T/B/cond` constructors. The output is original first-party static C++ UI metadata, **not copied DLL executable code**.

## Complete typed inventory

| Type | Records |
| --- | ---: |
| Boolean | 202 |
| Numeric slider | 111 |
| Dropdown | 32 |
| Color | 13 |
| Multiselect | 8 |
| Text | 6 |
| Numeric range | 4 |
| Button | 2 |
| **Total** | **378** |

All 378 records are compiled into `src/recovered_settings.cpp` with explicit module name, group name, display label, control kind, choice labels/placeholder or color, and example-only numeric range/default fields. `include/slinkywinky/recovered_settings.hpp` is the stable typed model. The data covers the module names from the candidate's **67 native descriptors**. Tests verify that no unknown module name appears, lookups cannot escape the 5-item preview bound, all typed values are finite, and the full counts/type families agree with the independently parsed archive.

**Provenance warning:** The reverse-engineered `mods_block.js` itself does not claim that its **numeric defaults/ranges or all category choices** are derived from the native binary. The UI presents these as **illustrative, read-only previews**, not saved settings or validated game-state values. Source-backed settings and first-party editable controls are separate work.

### Native registration comparison

For four directly comparable basic types (bool, number, enum, range), there are **59 modules with identifiable native `R_*` registration data** and **46/59 have matching type counts**. Thirteen differ. Eight modules have no matched native registration file; six of those also have no reconstructed controls, while `Weapons` and `Inv Manager` do have JS controls but lack corresponding registration evidence. An earlier raw **52/67** numerical match count incorrectly counted the six empty/empty cases as positive evidence. The **15 modules with mismatches or non-empty JS records lacking a native registration file** are:

`Aim Assist`, `Velocity`, `Criticals`, `Mod Overlay`, `Notifications`, `Auto Tool`, `Hit Select`, `Weapons`, `Inv Walk`, `No Hit Delay`, `Timer Range`, `Piercing`, `Ghost Hand`, `Inv Manager`, `Clutch`.

This is **module-level type-count evidence only**, not an attestation that every label, grouping, enum option, or numeric bound is native-exact. Some modules have native callback paths or conditional widgets not fully covered by the JS reconstruction. None of these differences was silently normalized or presented as fully verified.

## Renderer and input boundaries

- All eight kinds are rendered using new, first-party GL1.1 shape primitives, preserving the existing WGL context, scissor and complete GL state restoration.
- A module card may expand to inspect its first **five** recovered controls. The remainder are retained in the typed catalog and their count is shown explicitly; scrolling through each module's full setting tree remains future work.
- Expanded area height comes from the actual bounded control count, shared between renderer and mouse-hit geometry. Previews are clipped before becoming actionable.
- Source-only modules can expand their **informational card**, but the enable/toggle and `IncreaseSetting`/`DecreaseSetting` actions remain gated by the existing first-party native profile model. Inert setting widgets and profile icons receive **no input actions**, no OS key capture, no game packets, and no filesystem changes.
- Existing seven native numeric profile settings continue to be validated independently by the renderer; no third-party gameplay state is loaded.

## Actual first-party visual receipts

The screenshots are pixel readbacks from the disposable **900×630 Windows OpenGL** test context, not Photoshop mockups or official binary screenshots. The test also verifies in-frame, in-context rendering, unchanged GL state, and no clickable target within the expanded read-only settings area.

- [Aim Assist — dropdown, sliders, booleans](screenshots/r116-aim-controls.png)
- [Backtrack — range slider and numeric sliders](screenshots/r116-range-controls.png)
- [Friends — text field and inert action buttons](screenshots/r116-friends-controls.png)
- [Piercing — multiselect and dropdown](screenshots/r116-piercing-controls.png)
- [Item ESP — text, numeric, boolean and color controls](screenshots/r116-item-esp-controls.png)

## Verification and limitations

**Verified locally:** full Windows x64 Release build **PASS**, CTest **31/31 PASS**, Python unittest **55 tests (one skipped, zero failures)**. The extra `native_decrypted_settings_readonly_catalog` tests type/count/module boundaries, while the five actual isolated WGL screenshot receipts prove control-kind rendering, no hit target in the expanded area and GL state restoration. A separately supervised original vanilla Minecraft 1.8.9 Java 8 JVM (PID **33760**) passed the existing R94 **scripted operator show/hide/stop** acceptance, then exited normally; this is not physical mouse/keyboard or real game-feature acceptance. GitHub CI status must be checked after the R116 push.

**Not a complete functional reconstruction:** Visual kinds and descriptive controls are recovered; active module logic, reliable original defaults, animated popup/dialog behavior, nested setting scrolling, exact Inter font metrics, live profile file operations, original-game keyboard/mouse exclusivity, anti-cheat interaction, and packet timing are not certified. The project stays a native injectable-client architecture, **not Forge**. PR #1 should remain draft and unmerged pending actual physical input and gameplay proof.

**R117 priorities:** Use the confirmed native setting registration strings/option tables to mark individual controls as native-attested, implement bounded scroll/pagination through all settings in an expanded card, and test original mouse/keyboard ownership before enabling any additional functional settings.
