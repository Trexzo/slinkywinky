# R198 — source-informed color-picker state, not yet the complete visual popup

## Evidence and provenance

The two independent reference implementations inspected for this change are:

- C++: [Skid-Technology/OpenSlinky, `gui_popups.cpp`](https://github.com/Skid-Technology/OpenSlinky/blob/main/src/engine/gui/gui_popups.cpp). Its recovered popup-kind 2 contains a color swatch, six-character hex input, three HSL control bars, and an eight-entry recent-color ring. On popup close, a selected color is added to history even when the callback's apply flag is false; duplicates are omitted.
- Java: [meowflags/OpenSlinky, `ClickGuiScreen.java`](https://github.com/meowflags/OpenSlinky/blob/main/SlinkyClient/src/main/java/dev/slinky/client/gui/ClickGuiScreen.java), especially `renderColor`, together with [`GuiMath.java`](https://github.com/meowflags/OpenSlinky/blob/main/SlinkyClient/src/main/java/dev/slinky/client/gui/GuiMath.java), which has a six-character hex filter. The Java GUI also renders three HSL bars and eight recent swatches.

This is **behavioral triangulation**, not proof that either reconstruction is an authenticated original Slinky release. Source files are consulted as reference; this contribution does not copy their proprietary binary data, GUI source, text tables, or renderer.

## New first-party code

`include/slinkywinky/menu_color_popup_preview.hpp` is an **isolated, portable, first-party** state model:

- Admits exactly six case-insensitive hex digits; blocks malformed values and source out-of-range 24-bit colors. The visible `#` is a distinct label, not part of the six-character edit buffer.
- Represents the current color as 24-bit RGB and exposes RGB→HSL and HSL→RGB conversion with explicit finite/bounds checks. 0–1 HSL values drive conversion; 8-bit rounding is a first-party approximation pending original numeric-equivalence tests.
- Tracks a maximum of eight unique recent colors newest-first, with duplicate rejection and replacement when the ring wraps. It records color history at popup close, independently of `apply`, matching the observed C++ close path. It does **not** invoke or fake a callback or write settings.
- Keeps explicit `open`, `current`, `original`, `set_hex6`, `set_hsl`, `select_recent`, and `close(apply)` operations; rejects an overlapping popup until the first has closed. This overlap policy is intentionally stricter than the recovered native popup's automatic closure and is a known remaining difference.

`tests/test_menu_color_popup_preview.cpp` exercises the parser's valid/invalid paths, HSL primary colors, invalid NaN/out-of-range values, RGB round-trips for seven colors, cancelled-close recent history, deduplication and eight-entry ring rollover. The standalone source was **compiled and executed successfully** using Linux g++ 14.2 with `-std=c++17 -Wall -Wextra -Wpedantic -Werror`.

The root CMake registers an independent `r198_native_color_popup_hex_hsl_recent_fixture` test. It links no Minecraft, WGL, JNI, injection or Java libraries.

## Acceptance status and blockers

- **Verified:** isolated portable C++17 model compiles and its standalone regression passes locally.
- **Not verified:** R197 Windows source/bind preview integration, R198 Windows/MSVC build, full Windows CTest and original vanilla 1.8.9 acceptance. The connected Windows PC was offline at this revision.
- **Not implemented yet:** user-visible GL11 popup, authentic six-color-segment hue gradient, HSL bar drag gestures, hex field keyboard edit integration, recent-swatch layout, dialog close/apply/cancel routing and source-validated visible target selection. The existing `MenuState::set_recovered_color_preview` remains independent; this R198 state model is **not connected** to any gameplay module.
- **Not established:** pixel-perfect parity, original native float-to-byte conversion, complete fidelity of color-history initial slots and timing, any physical keyboard/mouse exclusivity, and all 67 module behaviors.

Do not interpret a passing portable test as a complete color picker or as proof that a playable Slinky is reconstructed. Keep PR #1 draft and independent C++ source-integration PR #2 untouched.

## Correct next development step

After Windows access is available, build and execute R197/R198, then connect the isolated color popup to source-indexed `RecoveredColorSwatch` with the real first-party WGL rendering transaction, and verify screenshot/behavior differences against both reference implementations and the user-owned candidate decrypted artifact. Only then move to complete remaining GUI controls and the 66 originally evidenced native module registrations, keeping the documented extra 67th descriptor discrepancy visible.
