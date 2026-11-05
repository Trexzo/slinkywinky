# R62 — first-party native menu data model and standalone preview
Date: 2026-11-01 · Branch: `reconstruction/r1-core-fidelity` · PR #1 (draft)

## Sources and boundaries
Official public documentation consulted:
- https://docs.slinky.gg/basics/get-started/
- https://docs.slinky.gg/modules/visual/gui/
- https://docs.slinky.gg/modules/visual/mod-overlay/
- https://docs.slinky.gg/modules/combat/aim-assist/
- https://slinky.gg/

The source documents a top menu, collapsible settings, module toggle/bind interaction, profiles separate from GUI appearance, orange default accent, Compact/Wide presentation and text sizing. The documentation sidebar enumerates **65 module names**: Combat 17, Move 10, Visual 15, Block 7 and Utility 16. The onboarding text names four navigation categories, while the contemporary module documentation also has a separate Block category. The **Block preview tab is provisional** until an authentic menu recording resolves the actual navigation layout. "Profiles" is present but real profile management and original Unload behavior are NOT implemented in this milestone.

## Implemented
- `include/slinkywinky/menu.hpp`, `src/menu.cpp`: independent C++17 catalog of all 65 public documented module names and category labels. Per-tab scroll, bounded eight-row view, per-module expanded UI-only bits and UI-only enabled-preview bits. Invalid indexes, off-category toggles and Unload selections are rejected.
- `src/menu_prefs.cpp`: GUI appearance serialization separate from gameplay/combat `ClientSettings`. Strict version-1 document; compact, wide, accent, scale and allow-input are parsed transactionally. Reject duplicate/unknown keys, malformed or out-of-range values; save through a temporary file and atomic replacement where supported. No module enabled state or account data is saved here.
- `examples/windows_menu_preview.cpp`: original Windows GDI dark menu preview with orange accent, top category tabs, keyboard category selection, mouse-wheel scroll, compact/wide presentation and demonstration-only switches/expand buttons. Preview GUI preferences saved under `%LOCALAPPDATA%\\SlinkyWinky\\menu-preview.conf`; separate from combat profiles. No copied Slinky assets, fonts, DLLs or screenshots.
- `tests/test_menu.cpp`: complete catalog uniqueness and category-count checks, bounded navigation, state isolation, malformed values, appearance roundtrips and file persistence.
- `CMakeLists.txt`: core portable menu model and CTest `native_menu_catalog`; optional Windows-only executable `slinkywinky_menu_preview.exe`.
- `.github/workflows/native-attach.yml`: preview executable included in signed-by-hash artifact inventory (SHA-256 verification; not a software signature).

## Controls
Open `slinkywinky_menu_preview.exe` directly in a Windows desktop. Keys `1..6` change category, `C` changes compact, `W` changes wide, `G` switches between two first-party color palettes, `Esc` closes. Use the wheel to scroll, the `+` region to expand a preview row, and the illustrated switch to flip a UI demo bit.

**No UI toggle controls the actual Minecraft client.** Expansion does not yet render authenticated recovered settings, profiles are placeholders, Unload is disabled and no input binds, JNI, GL state, window injection, packet commands, keybind capture or gameplay hooks are connected. A working stand-alone GDI preview **is not** an injected Minecraft GUI or verified original visual fidelity. The chosen exact layout, row spacing and non-orange alternate palette are first-party approximations, not assertions about the real Slinky renderer.

## Next acceptance work
1. Require core Release/Debug tests and Windows injector CI to pass on the exact GitHub branch commit. Standalone preview may build successfully without ever opening a real GDI window in hosted CI.
2. Test actual appearance and input on an authorized Windows desktop, adjust from official visual references / user-supplied decrypted evidence.
3. Add a safe in-process renderer adapter, resource teardown, guarded render callback, input routing and clean shutdown to the independently-built injectable client. Do **not** enable combat actions based on GUI toggles until explicitly designed and tested.
4. Run R60/R61 original vanilla 1.8.9 single-player world acceptance once the Windows desktop is reachable. The R48 main-menu-only pass cannot substitute for this.
5. Keep draft PR #1 unmerged until the real in-game GUI and runtime lifecycle checks pass.

Do not redistribute decrypted binaries or proprietary artwork. No Forge dependency is introduced for the production injector.
