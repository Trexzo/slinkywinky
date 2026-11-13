# R196 — wire recovered multiselect and dual-handle range into native retained ClickGUI

**Target:** [Trexzo/slinkywinky PR #1](https://github.com/Trexzo/slinkywinky/pull/1), `reconstruction/r1-core-fidelity`, still draft/unmerged. Independent OpenSlinky C++ PR #2 untouched.

## Functional gap found

The native WGL ClickGUI had source-backed hit geometry and first-party preview implementations for `RecoveredMultiControl` (R134) and `RecoveredRangePreview` (R133), but `WglRetainedMenuPreview::pointer()` did **not dispatch either hit kind**. They silently reached its default refusal. Earlier successful tests exercised those controls as **separate MenuState and MenuMouseGesture models**, not the retained WGL instance that is actually rendered. R196 closes this concrete gap.

## Implementation

`src/windows_retained_menu_preview.cpp` routes source-validated `RecoveredMultiControl` hits through `navigation_.focus_visible_row()` and `menu_.open_recovered_multi_preview()`. The already existing `DropdownOption` dispatch uses the R134 model to toggle selected bits, and the multiselect popup remains open to support repeated selection.

For `RecoveredRangePreview`, the same original GL11-painted `MenuRenderFrame` is used to calculate exact source widget track geometry. Physical pixel X is converted back to panel-local logical coordinates with the current panel scale, using `menu_numeric_value_at_x` rather than introducing another range conversion. Crucially, `hit->sub_index` is copied from the **captured mouse-Down** `MenuMouseGesture` hit: dragging over the other knob cannot steal ownership. The source-indexed `menu_.set_recovered_range_preview()` performs existing clamping and quantization and changes **only the isolated preview override**, never native game profile/behavior.

All pointer input still travels through R190–R193's offline synthetic paired input ledger and first-party MenuMouseGesture. This work does not introduce original-LWJGL event capture, event suppression or injected clicks.

## Native WGL test evidence

Added two genuine retained-WGL interaction regressions to `tests/test_windows_retained_pointer.cpp`:

1. **Auto Tool / Block multiselect:** navigate and expand the source-indexed module in the actual retained menu, locate its `MultiSelect` widget from the rendered frame and hit-test the exact GL11 control rectangle. Synthetic `Down/Up` opens the popup; after repaint, clicking a source-verified option sets bit 1, the menu stays open, and clicking again toggles the same bit off. Both before and after, the typed native profile and module enabled flag remain unchanged.
2. **Aim Assist / Combat range:** traverse the actual expanded settings pages until the `Range` track is painted. A synthetic held left-button drag crosses the other endpoint for both lower and upper handle cases (separate fresh native preview instances). The original pressed endpoint remains selected, and the R133 source model clamps it at the opposing value on release. Before Up, the painted frame's settings remain unchanged; after Up the old painted hit rectangles are invalid and a new GL11 draw shows the exact preview override. No gameplay/client profile mutation occurs.

R196 also introduces a **read-only, borrowed, exact-WGL-owner fixture frame receipt** `fixture_painted_frame(context,dc,width,height)`, which returns the actual last rendered immutable `MenuRenderFrame` only while the correct thread/context/DC/viewport is bound. The pointer expires on the next redraw, navigation or close. This enables testing actual rendered control placement instead of inferring it from a standalone fake MenuState. It adds no fields and preserves the existing WGL menu class ABI/layout.

The R119/R160 source guard explicitly forbids `fixture_painted_frame` and R190–R193 synthetic input methods from being called in `jni_bridge.cpp` or the Java 8 original Minecraft agent.

## Remaining limitations

The user-supplied decrypted candidate source is a reference for recovered labels, widgets, track geometry and behavior, **not** an authenticated complete original Slinky source dump. R196 proves correct operation of two control families in the first-party retained GUI **under synthetic test input**, not pixel-perfect parity with original Slinky or genuine physical Minecraft click ownership.

The original jar input audit R171/R194/R195 still finds original input consumers outside one main method, and foreground-original-client physical input exclusivity remains unproven. R196 does not implement remaining GUI controls, profiles persistence, real keybinds or the 67 gameplay modules; these remain separate future work. Keep live input disabled and PR #1 draft.

## Verification

**Windows x64 Release build PASS; full native/Java 8/JNI/agent-attach CTest 95/95 PASS**, including both R196 native WGL popup/range receipts and the R119/R160 no-live-input source guard. The standalone WGL test reported `R196_WGL_REAL_MULTI_POPUP_SELECTED_DESELECTED_PASS` and `R196_WGL_RANGE_PRESSED_HANDLE_CAPTURE_AND_CLAMP_PASS`. The isolated original vanilla Minecraft 1.8.9 read-only JVM acceptance separately exited **0**, with **six** admission/restoration stages, **64** retained GL11 frames, **60** source-visible slider draws and **63** passive cursor observations. Its exact OS focus counters remained `foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0`, source exact-hover hits **0** and `native_click_delivery=disabled`. The original game was only observed in the background, so there is **no** physical input interaction proof. No GitHub Actions success is asserted.
