# R120 — Correct native ClickGUI option labels from decrypted data

Source: the user's original `decrypted files.zip` (SHA-256 `326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4`). The extracted `.rdata` SHA-256 is `61eaa6bad74fd5cf0d374198506ec4f87fa042b5cf673e37ae01e87eb336706a`.

## Findings

An independent read-only review of `src/gen/mod_*.cpp`, `src/reg_38330.cpp`, `src/reg_476e0.cpp`, `slinky2.decrypted/data.bin` and `slinky2.decrypted/rdata.bin` recovered **424 registration calls across 61 modules** and **61 module GUI callbacks**. The callback sources have 424 widget callsites plus 128 section-label callsites; one static GUI widget reference could not be matched to a registration by its configuration RVA. These are static source callsites—not guaranteed simultaneously visible controls or verified running gameplay.

The binary reference's configuration structures contain actual *GUI display strings* and, for native enumerations, the counted option pointers. This is firmer evidence than the earlier estimated `re/mods_block.js` model used in R116.

| Module | Setting | Previously shown | Native source-backed GUI |
| --- | --- | --- | --- |
| Auto Clicker | Click pattern | Randomize / Jitter / Butterfly | **Jitter / Butterfly** |
| Piercing | Sort targets by | Distance / Hurt time / Health | **Hurt time / Health** |
| Anti Bot | Dropdown label | Always stationary | **Invalid UUID** (options unchanged: Off / Stationary only / Always) |

These three corrections are applied to `src/recovered_settings.cpp` and locked by `tests/test_recovered_settings.cpp`. No settings are activated and no gameplay calls are added.

## Remaining gap

The old 378-record JavaScript-derived control catalog is not equivalent to the 424 native registration/callback sites. In particular, some callback branches are conditional and some enum/name associations need more source work. Matching a native control's name or type is not proof of its defaults, allowed numeric range, animation or runtime behavior. The original archive is a candidate decrypted source, not an authenticated signed vendor release.

**R121 priority:** reconstruct the full ordered GUI-setting metadata from the native `W_*` callbacks and verified data-pointer display strings, retaining conditional visibility, before replacing the presentation-only 378-record catalog. The game keyboard integration remains gated by R119's safety audit; PR #1 stays draft/unmerged.
