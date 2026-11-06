# R97 — shared GL11 panel geometry and safe mouse hit testing

**Status:** implemented on the `reconstruction/r1-core-fidelity` draft branch for the **standalone first-party Windows GL11 preview** and pure C++ tests. **No in-game mouse interception, Minecraft input hook, gameplay activation, combat or live ClickGUI is enabled.** PR #1 remains draft and unmerged.

## Why this change was needed

Before R97, the GL11 renderer positioned its three layouts with viewport- and scale-dependent coordinates, but the separate standalone Win32 preview still guessed clickable regions with fixed, full-window pixel coordinates. For example, R93's `CompactOverlay` draws in a centered 480×420 physical panel at **70% scale**; an unscaled click might land on a different row or trigger a preview setting.

R97 removes that mismatch by sharing one **render-neutral geometry calculation** between the native GL renderer and the pure menu hit tester:

- `include/slinkywinky/menu_hit_test.hpp` / `src/menu_hit_test.cpp` define `MenuPanelMode`, `MenuPanelGeometry`, `menu_panel_geometry` and `hit_test_menu`. This code uses no Win32 key APIs, WGL, Java, JNI, Minecraft runtime or networking.
- `StandalonePreview` retains its full-client viewport. `BoundedOverlay` has a centered panel up to 676 or 844 pixels wide (depending on the first-party appearance setting) and up to 630 high, leaving at least 12 pixels on every physical side. `CompactOverlay` uses up to 480×420 physical pixels, centered, with model coordinates scaled to 70%. Smaller viewports shrink the panel while preserving its 12-pixel border.
- `src/windows_menu_gl11.cpp` now calls `menu_panel_geometry` instead of independently calculating its dimensions. The OpenGL scissor, translation, scale and panel-size normalization all come from this one calculation. The existing GL context admission, GL state restoration, and R94/R95 visibility-only JNI paths are unchanged.

## Bounded, fail-closed hit semantics

`hit_test_menu` accepts **top-left-oriented client pixel coordinates** (the same convention as `WM_LBUTTONDOWN`) and the immutable, native first-party render snapshot. Its return types are `Tab`, `RowBody`, `Expand` and `Toggle`. It never mutates a module or sends an input event.

- A point outside the actual visible panel is denied **before coordinate transformation**. It is impossible to wrap a negative signed coordinate around to the right edge.
- The same compact 70% scale that renders the panel maps physical click coordinates back into model space. Only the displayed 7 tab cells at logical x=18…w−18/y=78…120 and visible module rows at x=18…w−18/y=153+i×49…+44 are considered. The 5-pixel gaps between rows are inert.
- The painted seventh `Unload` tab is **not a valid click target**, preserving previous fail-closed UI policy. Profiles is informational and has no module rows.
- Toggle hit regions match the drawn control at x=w−76…w−31 and y=row+12…+34. Expand matches the drawn x=w−123…w−105 and y=row+13…+31. Other areas within a valid row can only change **visual focus**. Unsupported modules always yield the inert `RowBody` hit, even when clicking where a toggle/expand widget would appear.
- Any row partly outside the current panel clip is **not** clickable. This matters at the minimum 320×240 viewport: a partially visible fourth row is deliberately denied rather than acting on an ambiguous target.
- Source frame metadata must have a valid epoch, sequence, tab, placeholder state and at most 8 entries. Every visible row must have an exact catalog identity/title and must appear in strictly increasing index order. Duplicates, reordered/forged rows and unknown tab/viewport modes are denied without changing native menu state.

## Standalone preview wiring

`examples/windows_gl11_menu_preview.cpp` now uses the same hit tester as OpenGL for its own `WM_LBUTTONDOWN`. It acts only through R96's first-party `MenuNavigation` and `MenuProfileBridge`, and only when the actual menu frame dimensions match the current Win32 client dimensions. Clicking the row body changes focus; clicking an implemented module's painted toggle or expand area changes **only local profile/preview state**. Nothing is wired to Minecraft.

Press `O` in the standalone preview to enter/leave an overlay demo and `K` to choose bounded versus compact layout; the window caption identifies the layout. Keyboard navigation from R96 remains separate. The preview's mouse-wheel handler now also requires the pointer to be inside the current physical panel; outside-wheel input cannot scroll the menu.

## Evidence and compatibility

- The new cross-platform CTest `native_menu_hit_testing` covers standalone/bounded/compact layouts, exact centered offsets and scale at 676×630 and 1024×768, painted tab/row/toggle/expand targets, rejected Unload tab, panel-edge coordinates, row gaps, off-page rows, 320×240 partial clipping, scroll-page index mapping, Profiles placeholder, unsupported-module inert focus, and forged/duplicate/reordered frame negative controls.
- The existing real Win32 `isolated_wgl_menu_state_restore` now performs an **actual compact GL11 pixel readback** using the same logical client-space toggle coordinates returned by R97, and checks Expand/Toggle hit identity, clipped borders and GL state restoration. No Minecraft process is needed to perform this test.
- The original C++ navigation and WGL ABI, the immutable `MenuRenderFrame` layout, and prior R91–R96 standalone and JNI entry points remain compatible.
- The full local Windows x64 Release CTest suite on the R97 source passed **27/27** (including native Java 8/JNI/Attach tests). After the final strict duplicated/reordered row negative controls, the affected native navigation, hit and real WGL tests passed **3/3**. GitHub CI on the latest branch head must be checked independently.

## Outstanding gates

R97 does **not** establish interactive in-game ClickGUI or physical mouse control. The original game still only has the R94 bounded show/hide and stop pathway. R95's direct synthetic/physical Insert foreground admission remains **unverified**, so it is not bypassed. Before any mouse capture in Minecraft, separately prove target-window focus/identity, press/release semantics and click noninterference with Minecraft gameplay (including multiplayer), original renderer thread ownership, and safe stop/restoration.

Keep PR #1 **draft/unmerged**, with all combat/gameplay modules disabled.
