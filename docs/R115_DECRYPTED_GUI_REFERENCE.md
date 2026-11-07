# R115 — SlinkyWinky GUI reconstructed from the user-supplied decrypted candidate

**GitHub target:** `Trexzo/slinkywinky`, `reconstruction/r1-core-fidelity`, draft PR #1.

## Provenance: source-first, not a screenshots-only approximation

The reference was the user's **original** `decrypted files.zip`, SHA-256
`326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4`
(130,199,032 bytes). It was opened **read-only** as a ZIP. Its bundled DLL, injected code,
unknown JavaScript and reconstruction scripts were **not executed**.

Source locations inside that archive:

- `decrypted files/re/clickgui.tpl.html`: annotated, recovered native ClickGUI layout constants and basic HTML illustration. Its title/notes identify it as a reconstruction from native `.text` code, not original DOM source.
- `decrypted files/re/modules_real.json`: native module descriptor names, descriptions, categories and GUI callback entry-point RVAs. This is the source of our **67** catalog records: 17 Combat, 10 Move, 15 Visual, 7 Block, 18 Utility; 61 descriptor callbacks, 6 engine-only.
- `decrypted files/re/mods_block.js`: reverse-engineered control groups/labels/widgets for 67 modules. **Important provenance note inside the supplied file:** setting defaults/ranges, and category assignment in this JS fixture, were **not taken from code**. Eight categories differ from the native descriptor list (Timer, Fake Lag, Blink, Weapons, Keep Sprint, No Hit Delay, Item Use Fix and Auto Rod). The descriptor categories are the source of truth here.
- `decrypted files/src/gen/mod_*.cpp`: native GUI setting registration callsites and wrappers, usable for further typed-control recovery. Existence is not proof of full behavioral equivalence.

**Authentication limitation:** the supplied candidate binary is internally consistent with its decrypted .text/.rdata, but an independently signed/authenticated official Slinky release is unavailable. “Source-backed” means this archive, not proven identical to all official builds.

## Corrected visual design

The R113/R114 previews were based principally on public screenshots and incorrectly followed an older screenshot's grid/color typography. R115 replaces those assumptions with settings from the recovered reference:

| Item | Candidate reference / R115 implementation |
|---|---|
| Navigation | **7** entries: Combat, Move, Visual, Block, Utility, Profiles, Unload. Block **remains** separate |
| Default accent | **#F35D12** |
| Overlay | **#303030BF**, 75% gray; nav/card **#141414** |
| Navigation | Height **42**, radius **20**, selected pill inset **6**, selected fill **#404040** |
| Module grid | Logical max width **900**, wide max **1500**, outer padding **8**, gap **12**, columns target **430px**, up to **4** |
| Module cards | Radius **20**, inner padding **8**, text **#DDDDDD** / description **#999999** |
| Toggle | **40 × 20** track, 8px knob; read-only preview switches |
| Expand control | **20 × 20**, radius **5** |
| Font | Reference observed **Inter Medium/Bold** embedded in its DLL. The reconstructed first-party client uses local Segoe UI and bundles **no** vendor font files |

Unlike a flat row list, the new renderer and hit tester share the same column geometry, including gap refusal, clipping, variable-height expanded cards, and the original index assigned to each card. The isolated 900px WGL test demonstrates **two rendered columns** and refuses clicks in the 12px separator.

## Module catalog and settings

The previous public screenshot catalog had only 65 module descriptors. R115 loads `src/r115_recovered_catalog.inc` with **67** names/categories and `include/slinkywinky/menu_visual_copy.hpp` with **67** exact descriptor descriptions. This restores **Chest Stealer** and **Debug**, and corrects **Mod Overlay** capitalization.

The user's archive provides a **reference GUI control inventory**, reconstructed as 378 widget records in the fixture: 202 booleans, 111 sliders, 32 dropdowns, 13 colors, 8 multiselects, 6 text controls, 4 ranges and 2 buttons. These are **reference metadata**, not 378 implemented operational settings. The full structured extraction manifest is maintained separately so future GUI branches can implement their grouping and layout without guessing.

For the **seven first-party numeric settings already modeled**, the renderer now receives typed numeric bounds/values rather than parsing untrusted display strings. Supported first-party controls draw read-only slider geometry with clamped progress and reject invalid nonfinite/reversed/out-of-range snapshots before GL drawing. Verified source labels include “Target CPS”, “Range”, “Maximum delay”, and “Distance”. No vendor game executable, player, combat logic or unknown DLL is invoked.

The Profiles section displays the actual local Default profile plus an **inert** create-profile tile. Existing Save/Load/Folder/Delete icon glyphs are strictly cosmetic. No profiles are invented or enumerated. A dummy four-profile gallery from R114 remains only a test fixture.

## Actual OpenGL screenshot receipts

These screenshots were captured from the first-party native renderer in a **disposable 900 × 630 WGL context**, not made by an image generator and not captured from a running game.

- [Recovered 900px Combat grid](screenshots/r115-recovered-900-combat.png)
- [Recovered 900px expanded Auto Clicker native slider](screenshots/r115-recovered-900-settings.png)
- [Recovered 900px Profiles and create tile](screenshots/r115-recovered-900-profiles.png)

The WGL test proves original GL state restoration across two contexts and a page change, verifies the native hitbox of the second column, and refuses clicks in the column separator or inert Profiles tile. It also validates Source-related negative controls for tampered numeric settings.

## Verification, limitations and next milestones

The full local Windows x64 Release build, CTest/Python regressions, source-pinned vanilla 1.8.9 supervised script, and GitHub CI for this commit are separately verifiable. No physical keyboard click-through acceptance is implied by the scripted game test.

**Not yet feature-complete:** The 61 GUI callback descriptor entries are *not* 61 runnable modules; first-party native settings are limited, the remaining recovered widget groups are not fully rendered or interactive, default/range values in the JS reconstruction are not source-certified, exact Inter glyph metrics/animation are not copied, source GUI layout behavior on very wide/resized screens needs more acceptance, and real exclusive Minecraft keyboard/mouse ownership and safe profile filesystem actions remain unverified. Do not connect gameplay hooks or merge the draft PR based only on visual test success.

**Next useful step R116:** convert the recovered per-module widget inventory into a strictly typed, bounded first-party read-only setting tree, using actual `src/gen/mod_*.cpp` code to distinguish confirmed options from guessed fixture defaults, and test scrolling/expansion and all control kinds against the original WGL GUI. Only then separately wire admitted profile edits to the first-party engine under live acceptance.
