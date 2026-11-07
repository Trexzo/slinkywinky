# R148 — live numeric drag preview and original 12px knob radius

**Branch:** `reconstruction/r1-core-fidelity` · draft PR #1, unmerged.

## Candidate recovered GUI evidence

The user's `decrypted files.zip/re/clickgui.tpl.html` is a reconstructed HTML reference, **not authenticated commercial native source code**. Its `widget(it)` branch for `it.k==='s'||it.k==='r'`:

- Calls `setPointerCapture` on pointerdown, chooses one of two range handles once by nearest distance, and calls `mv(e)` on every pointermove while dragging.
- Snaps to `it.st`, clamps values within min/max, and prevents a range handle crossing its opposite endpoint.
- Updates the filled track and knob position, calls `tipAt()` and renders the current formatted value as a floating tooltip.
- Declares the actual knob radius as **6px**, corresponding to a **12×12px** circle; the earlier GL11 approximation used 10×12.

The candidate HTML also calls `applyLive(mod)`, whose implementation and real native registration effects are not authenticated. R148 must not copy the side effect into an injected gameplay process.

## R148 implementation

- New `MenuNumericDragPreview` in the first-party `MenuState` stores the **source module/widget ID**, the **press-selected range handle**, and a transient slider value or pair of range endpoints.
- `begin_numeric_drag_preview` validates actual source widget kind, tab, expanded module, current five-control page, min/max/step and current values. It refuses wrong modules, hidden pages, another active drag, a popup/modal or a malformed range. This does not write `recovered_slider_overrides_`, `recovered_range_overrides_`, `ClientSettings` or Minecraft memory.
- `move_numeric_drag_preview` snaps to source step size, clamps each range endpoint at the opposite value, and changes **only** its transient display values. It rejects NaN, infinities and out-of-range inputs.
- `MenuRenderFrame` snapshots the temporary drag record; `draw_menu_gl11` overlays that record onto an ephemeral widget copy for the visible knob/bar. `menu_numeric_tooltip_geometry` formats the same transient value, including a range midpoint, **without modifying the persistent R132/R133 preview values**.
- `MenuMouseGesture::equivalent_frame` now tolerates a *single special case*: the original captured target is a numeric Slider or Range and the updated immutable frame contains a transient drag matching **that exact module, source widget and captured handle**. All unrelated UI changes still invalidate the gesture, including a hover switch, tab/mode/page change, popup, color/text/bind/profile edit, appearance, and a normal committed numeric override.
- In the isolated Windows WGL preview, mouse-down begins the temporary drag after acquiring explicit window pointer capture. The current logical pointer x position updates the temporary value on each admitted mousemove; repaint shows the new knob and formatted tooltip. **Only the original guarded same-target mouse-up** calls `MenuState::set_recovered_slider_preview` or `set_recovered_range_preview`. Every cancel/focus loss/window resize or invalid release rolls back the transient display value.
- GL11 slider and range knobs now use **12×12px, radius 6px**, with centers unchanged. Shared 18px input/hit geometry remains backward compatible.

## Regression and limitations

`native_recovered_numeric_drag_transaction` tests both single and range controls across standalone, bounded, and compact panel modes, including:
- Source-typed click admission, matching transient target, and per-move step snapping
- Current tooltip changing before mouse-up while the saved value remains untouched
- Preserving range lower-handle ownership even after crossing toward the upper knob
- Strict denial of malformed source indices, forged slider handles, NaN and out-of-range values
- Exactly one verified release commit, cancellation rollback, page-change invalidation, and unchanged module/profile flags.
- Numeric-only off-track pointer capture inside the owned panel, endpoint clamping, and a deliberate three-second drag.

**Source capture refinement:** R148 also extends the R98 mouse gate for **numeric controls only**: their originally owned track continues to receive sampled x positions while the pointer is anywhere **inside the exact same preview panel**, even beyond the thin 18px track. The value clamps to the source min/max at the horizontal endpoints. R98's strict same-hit rule and **1.5-second** timeout remain unchanged for all non-numeric buttons, toggles and navigation. Numeric drags have a separate bounded **30-second** maximum duration. Leaving the panel, changing the source frame, losing focus, or canceling Windows pointer capture still rolls back the transient drag. This is closer to the HTML's `setPointerCapture` behavior but deliberately does not capture beyond the trusted preview panel boundary. Font metrics, easing/animations and in-game Minecraft GUI integration are further unverified. This source-neutral implementation avoids making unsupported claims about the proprietary client's runtime. Verify the exact GitHub head in CI before reporting R148 as passing.
