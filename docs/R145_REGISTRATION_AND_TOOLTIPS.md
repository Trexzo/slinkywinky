# R145 — candidate registration audit and Profiles tooltip fidelity

**Repository:** Trexzo/slinkywinky · `reconstruction/r1-core-fidelity` · draft PR #1 (not merged).

## Evidence boundary

User-supplied candidate archive: `decrypted files.zip` (SHA-256
`326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4`).

The candidate `decrypted files/re/modules_real.json` contains exactly **67 unique** registration descriptors and category IDs. It was checked against the current first-party `src/r115_recovered_catalog.inc` by name/category, with **zero missing modules, zero unrecognized names and zero category mismatches**:

| Category | Archive and source |
| --- | ---: |
| Combat | 17 |
| Move | 10 |
| Visual | 15 |
| Block | 7 |
| Utility | 18 |
| **Total** | **67** |

Six entries have zero candidate callback pointers: **Instant Stop, Item Use Fix, No Hurt Cam, No Jump Delay, Null Move, Ping Fix**. A zero pointer in this candidate manifest does not establish the absence of actual runtime behavior. It is not proof that the native binary was decrypted accurately or that any given gameplay module is reconstructed and functional.

Repeat the comparison without unpacking or running archive content:

```sh
python tools/audit_recovered_registration.py \
  --archive "/path/to/decrypted files.zip"
```

The audit fails closed on duplicate names, invalid registration categories, malformed records, mismatched labels/category IDs and unexpected counts. It only reads ZIP metadata and JSON; it does not execute candidate code or inspect sensitive personal directories.

## Visual gap: source tooltips on Profiles icons

The candidate `re/clickgui.tpl.html` defines a 20px-high `#404040` tooltip with radius 5, horizontal padding 5, 14px Inter text, positioned **28px above the hovered icon center**.

R145 adds the six exact strings from its reconstructed `profileCard()` callbacks:

| Target | Tooltip |
| --- | --- |
| Any profile — Load | Click to load |
| Default — Save (disabled) | Default profile cannot be overwritten |
| Any profile — Folder | Click to open folder |
| Default — Delete (disabled) | Default profile cannot be deleted |
| User profile — Save | Click to save |
| User profile — Delete | Click to delete |

The Default Save/Delete glyphs are now drawn at 50% alpha and can show explanatory hover tooltips, but **remain unclickable**. Hover is stored as source-indexed transient `MenuProfileIconHover`; it is admitted only for a visible, modal-free Profiles card. A tab change, modal, delete, pointer leaving the window, or losing foreground focus clears it. The model and GL11 renderer share the same bounded icon geometry.

`native_recovered_profile_tooltip_hover_isolation` checks all six tooltip messages, disabled action exclusion, three panel modes, bounded tooltip coordinates, modal ownership, stale mouse frame invalidation and no gameplay changes.

## Current fidelity limitations

The candidate HTML is a reconstruction/transcription, **not authenticated original commercial Slinky source code**. GL11 uses its own system/bitmap fallback font renderer, so exact Inter Medium/Bold glyph widths, 0.1s hover/transition easing and pixel-perfect screenshot parity are still **unverified**. Native game/injector input integration and functional acceptance of 67 game modules are separate unfinished work.

**CI status:** Check the exact latest PR head. Source-level commits do not imply that Windows/Linux CTest, sanitizer or runtime screenshot comparisons succeeded.
