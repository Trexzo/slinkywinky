# R86 — native module menu backed by C++ profile settings

**Status:** source integration and offline preview workflow. This is **not** a live Minecraft module activation or a Forge mod. The project remains a native Windows Java 8/JNI/JVMTI injectable client in development.

## Why this step matters

The R62–R70 native menu exposed 65 published Slinky module **names**, but the demo toggles had no relationship to the first-party C++ combat/settings engine. Showing all 65 as functional was misleading. R86 introduces a typed configuration bridge that maps a supported subset to the already-present engine's `ClientSettings`, with explicit unimplemented states everywhere else.

## Implemented binding

`include/slinkywinky/menu_profile.hpp` and `src/menu_profile.cpp` add `MenuProfileBridge`, which is compiled into `slinkywinky_core` and deliberately uses no JVM, Minecraft client, mouse or packet API.

| Public catalog row | C++ settings field |
|---|---|
| Aim Assist (0) | `combat.aim_enabled` |
| Auto Block (1) | `combat.auto_block_enabled` |
| Auto Clicker (2) | `combat.auto_clicker_enabled` |
| Backtrack (4) | `backtrack_enabled` |
| Hit Select (6) | `combat.hit_select_enabled` |
| Reach (12) | `combat.reach_enabled` |
| Velocity (15) | `velocity.enabled` |

All **58 other entries remain unavailable**. The native menu cannot toggle them through `MenuProfileBridge::toggle`, and their rows show `NOT IMPLEMENTED`/equivalent. Supported toggles update native *configuration* only; this is **not yet evidence their gameplay effects work**, nor are the seven settings exposed to a live Minecraft hook.

The binding checks that a row is selected, **currently visible**, and supported before changing an option. It refreshes visible switch indicators on tab selection and scrolling. A new profile is accepted only after an entire `ClientSettings` document passes existing serialization/parse validation; corrupt or out-of-range imports leave the previous model unchanged.

## Preview controls

Both native Windows GDI and first-party GL1.1 preview applications now use the same controller rather than independently toggling demo bits. `P` writes and `L` reads the editable native profile at:

`%LOCALAPPDATA%\\SlinkyWinky\\module-profile-preview.conf`

The saved native profile is versioned, uses the existing validated `save_settings` and `load_settings` routines, and remains separate from menu colors/scaling. The preview never injects into the game, makes no Minecraft input requests and does not start an Attach.

The GL renderer distinguishes `NATIVE SETTING ONLY` from `NOT IMPLEMENTED`. No third-party fonts, branding assets or Slinky binary code are copied into this first-party display layer.

## Verification

`native_menu_profile_binding` is a new cross-platform CTest, exercising all 65 support classifications, wrong-category and offscreen rejection, seven C++ setting assignments, scrolling/tab synchronization, a save/reload roundtrip, corrupted-profile rollback, and out-of-range option refusal.

Existing full Windows native OpenGL pixel/state tests and Java 8 injectable integration must also remain green. No real-game visual, WGL or combat acceptance is claimed by these tests.

## Next milestone

Integrate this profile/settings controller with an **explicitly admitted in-process renderer/input owner** only after supervised original-1.8.9 rendering tests establish WGL context ownership, GL state restoration and safe teardown. Keep PR #1 draft and unmerged until then.
