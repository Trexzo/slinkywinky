# R146 — Native registration vs HTML fixture and shared Aim speed settings

**Branch:** `reconstruction/r1-core-fidelity` · draft PR #1, unmerged.

## The comparison that changes how fidelity is measured

The user's candidate `decrypted files.zip` contains **three different levels of evidence**, not three interchangeable copies of the same GUI:

1. `re/modules_real.json`: the recovered **67 registered module names and categories**. All 67 names/categories match the repository's `src/r115_recovered_catalog.inc` exactly.
2. `src/gen/mod_*.cpp`: C++-like generated *registration stubs* with `R_bool`, `R_num`, `R_enum` and `R_rng` calls, and separate callback display routines. These are better evidence for distinct registered settings than the decorative HTML, but they are still user-supplied generated reconstructions, not authenticated compilable commercial source.
3. `re/mods_block.js` and `re/clickgui.tpl.html`: a reconstructed **HTML visualization** and higher-level display fixture with illustrative values, conditional visibility and simplified widget kinds. The original HTML expressly disclaims that all setting defaults/ranges and category locations were taken from native code.

The recovered HTML widget constructors count **378** (202 booleans, 111 sliders, 32 dropdowns, four ranges, eight multiselects, 13 colors, six text fields, two buttons); the repository's 385-entry display fixture has **206 booleans, 115 sliders, 32 dropdowns, four ranges, seven multiselects, 13 colors, six text fields and two buttons**. Exactly two modules explain the type/count differences:

| Module | HTML reconstruction | Repository display fixture | Native registration stub |
| --- | --- | --- | --- |
| Aim Assist | 12 bool, 4 numeric, 3 enum, 2 range (21) | 11 bool, 8 numeric, 3 enum, 2 range (24) | 11 bool, 6 numeric, 3 enum, 2 range (**22 registered settings**) |
| Piercing | 2 bool, 1 multiselect, 1 enum (4) | 7 bool, 1 enum (**8**) | 7 bool, 1 enum (**8 registered settings**) |

**Do not overwrite the native registration-shaped Piercing display with the HTML's four-control shorthand.** The candidate generated `src/gen/mod_piercing.cpp` explicitly registers one enum and seven Boolean controls.

## Bug and R146 resolution

The Aim Assist fixture has two *display callsites* for each `Horizontal speed` and `Vertical speed` numeric control: one visible in **Silent** mode, one in **Regular/Linear** modes. The registration stub `src/gen/mod_aim_assist.cpp` registers **one underlying numeric setting per speed axis**.

Before R146, `MenuState::set_recovered_slider_preview()` maintained a separate source-index override for each display callsite. Editing Horizontal speed in Silent and changing to Regular/Linear therefore displayed the old value. That was incorrect even in the GUI preview.

R146:
- Adds `menu_aim_speed_alias_peer()`: identifies exactly one opposite-branch callsite for each of the two speed axes, matching module, group, label, slider kind, opposite visibility and identical numeric bounds/step/default. Any missing or ambiguous peer fails closed.
- A source-verified visible slider edit is snapped/clamped first, then **both display overrides receive the same value atomically**. No off-page control can be edited directly.
- The underlying `ClientSettings`, game input, injector and original client configuration remain untouched.
- Registers `native_recovered_aim_speed_shared_settings`: checks two validated pairs, mode-dependent edits in Silent/Regular and round-trip retention, independent axes, other module isolation, read-only renderer frame mirrors, unchanged module enable flags and renderer teardown.

## Remaining acceptance gaps

This is a GUI consistency correction, not proof of gameplay Aim Assist reconstruction. The numeric min/max/default/step values in the fixture remain unverified approximations. The registered native callback and runtime semantics still require controlled testing. A screenshot/geometry/animation diff is also required before claiming an exact original ClickGUI.

**Validation:** inspect GitHub Actions at the exact current branch head. Do not use older workflow results as proof this batch has passed.
