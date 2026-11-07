# R147 — Recovered slider/range hover tooltips

**Repository:** Trexzo/slinkywinky · `reconstruction/r1-core-fidelity` · draft PR #1, unmerged.

## What the candidate reference actually establishes

The user's `decrypted files.zip/re/clickgui.tpl.html` is an **unverified reconstructed HTML reference**, not authenticated original native commercial Slinky source.

For slider (`it.k==='s'`) and range (`it.k==='r'`) widgets, the candidate reference attaches `mouseenter` to the slider track, positions a **20px, 5px-radius** tooltip above the value, formats numeric values with a custom `fmt()` function and repositions the value anchor as the selected values change. Its tooltip is floating and `pointer-events:none`, with `#404040` background and 14px text. Range tooltip position uses the midpoint of both values.

The explicit label formats and units are defined in `re/mods_block.js`, not in the original generator's native metadata. R147 transcribes 115 distinct module/label-format pairs (e.g. `12.5 CPS`, `3.0 - 4.5 blocks`, `80%`, `200 ms`) as **first-party data**. Formatting never invokes a source-controlled `printf` format string: only whitelisted `%0.0f` through `%.3f` and escaped `%%` tokens are parsed and rendered using a fixed static numeric format.

The 385-entry GUI fixture contains **119** Slider or Range display callsites. Of these:
- **117** match the **115** format rules, including both duplicated Silent/Regular/Linear Aim speed callsites sharing each native-backed value through R146.
- **Two** Aim Assist Multi-point sliders are present in the C++ preview fixture, but absent from the simplified candidate HTML numeric formatter. R147 intentionally leaves these **without invented numeric tooltip text** pending stronger registration/display evidence.

## Implementation

- `menu_numeric_format_for_widget` requires a real Slider/Range catalog entry with a matching module and label.
- `MenuState::set_numeric_hover` admits only a source-matched slider/range on the current tab, in an expanded module, on the current five-widget page, and with a known source format. It refuses wrong modules, unknown indices, closed panels, or active dropdown/modal states.
- `MenuRenderFrame::numeric_hover` holds only the indexed hover identity. Mouse gesture equivalence detects changes and invalidates an in-flight click; the original GUI fixture, gameplay state, JNI, disk and Minecraft input remain untouched.
- The Windows GL11 standalone preview finds the hovered numeric track using **the same `menu_numeric_track_geometry()`** used by click hit testing and slider drag release. The hitbox excludes labels, off-page and clipped controls, and nested popups.
- Hover tooltips use the current immutable `frame.recovered_slider_overrides` or `recovered_range_overrides` (or source fixture value), not a separately synthesized number. They position over the actual value knob (or range midpoint), clamp within the logical panel, and render after cards so they float visibly.
- Hover is cleared on mouse leave/window focus loss, tab changes, wheel scroll, settings pagination, expanded-card changes, Aim/Target mode switches and popup/modal transitions.

## Regression

CTest `native_recovered_numeric_tooltips_hover` verifies all **115** source formats and **119** current numeric display callsites, the exact **117/2** known/unknown split, formatter escaping and malformed format rejection, formatted range and decimal labels, viewport and tooltip clipping, visible hit target and local value changes in standalone/bounded/compact layouts, stale mouse-gesture rejection and tab ownership isolation.

## Still not identical to real Slinky

The reconstructed HTML preview updates the floating tooltip **continuously during a captured pointer drag**. Our first-party GUI demo updates the numeric value **only upon a guarded mouse release**, and therefore the tooltip updates after that commit. This intentionally preserves the existing source-indexed input admission policy but is **not full pointer-drag parity**. Exact font glyph widths (original HTML uses Inter), tooltip anti-aliasing, animation easing and full visual screenshot parity also remain open. Likewise, slider fixture min/max/step/defaults are not authenticated native runtime settings, and none of these changes implements actual gameplay features.

**Verification:** inspect GitHub Actions on the exact PR head; a successful source commit or older CI run is not proof of R147 testing.
