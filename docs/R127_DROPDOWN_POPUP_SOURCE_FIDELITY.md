# R127 — Source-shaped popup selection in the native ClickGUI preview

**Repository:** Trexzo/slinkywinky; draft PR #1 (unmerged)  
**Runtime boundary:** separate first-party Windows GL1.1 menu preview/overlay demonstrations only. This is **not** an in-game Minecraft input hook, live combat configuration, or original commercial client.

## Source and provenance

The user's archived `decrypted files.zip` contains `re/clickgui.tpl.html`, a **reconstructed** GUI template with notes referencing native code around dropdown rendering. That reference specifies right-half dropdowns, 2px #404040 strokes, 10px rounded corners, open rows of **28px per option**, #F35D12 selected choices, and neutral option text. The archive is not an independently authenticated vendor release; CSS animation/geometry translations are not proof of exact original frame-for-frame behavior.

Separately, the recovered native registration source identifies:
- Aim Assist's `Aim / Mode` callsite with four preview choices;
- Aim Assist's conditional `Target / Mode` callsite;
- Piercing's `Always hit through / Sort targets by` callsite, options `Hurt time | Health`.

R120's source audit counted **61** GUI callbacks and **424** W_* callsites. The older typed 385-widget GUI catalog is a useful reference fixture but not a complete native setting parity proof. The R127 dropdowns only use callsites already mapped and consciously avoid writing the unverified settings of other modules.

## Implementation

- `MenuState` owns a three-kind, local-only popup (Aim, Target or Piercing sort) and admits selection only from the currently expanded, visible, five-control settings page. No gameplay profiles or combat state are touched.
- `MenuRenderFrame` captures the popup by value. `MenuMouseGesture` cancels an in-flight click if the popup is opened, closed, or changed mid-press.
- Shared `menu_dropdown_popup_geometry.hpp` calculates the same 28-pixel option rectangles for the GL11 renderer and mouse hit tester, including conditional Aim settings, group headings, bounded/compact panel transforms and an upward-placement fallback.
- An open popup captures all hit-testing. Clicking outside dismisses it without propagating the same click to nav tabs or other modules. Mouse-up on the same option after foreground/owner admission commits one locally selected value.
- The overlay is drawn above all cards with rounded border, 28px options, separators, neutral text and selected-option orange. Escape dismisses the popup before it can close the whole preview; wheel/focus loss also dismisses.
- Existing Piercing boolean switches remain directly editable in the sandbox. GUI module status and all Minecraft gameplay remain unchanged.

## Validation

The newly registered `native_menu_dropdown_popup_preview_safety` CTest covers all three presentation modes; menu state separation; visible Aim and conditional Target callsites; exact popup option hits; no click-through; wrong-choice refusal; stale held-click invalidation; Piercing choice selection without altering its seven other flags.

**Verification status:** Code and tests are committed, but Windows Release, complete CTest and original Minecraft acceptance on this precise head have **not** been run or confirmed. Desktop Commander was offline. No visual pixel-parity or actual in-game input ownership is claimed. Do not treat draft PR #1 as complete.

## Still to reconstruct

- Reference-calibrated hover, .12s popup animation, precise Inter font metrics (no font binaries are included), and all remaining widget families.
- Editable/profile-persistent settings for the full 67-module inventory, branch-dependent visibility, real profile cards/actions, bind assignment and modal confirmations.
- Complete safe keyboard **and** mouse ownership in the original vanilla 1.8.9 client, with exact restoration, before enabling live GUI actions.
- Independent gameplay logic verification for every module, after ClickGUI acceptance.
