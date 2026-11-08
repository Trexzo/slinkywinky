# R178 — 60-frame recovered numeric hover opportunity in original vanilla 1.8.9

**Repository:** `Trexzo/slinkywinky`; branch `reconstruction/r1-core-fidelity`; unmerged draft PR #1. C++ OpenSlinky integration PR #2 remains independent.

## Why this change is required

R176 verified that the original Minecraft 1.8.9 process executed the production passive-hover path: 64 retained GL11 draw frames and 63 native pointer observation calls. All 63 observations were **background**, so no foreground or numeric hover was proven. R177 independently verified the real first-party C++/OpenGL numeric tooltip pixels in a disposable WGL window and fixed the JNI-only GitHub CI configuration.

The original R99 menu diagnostic scripted its four inert navigation actions on frames **1, 2, 3 and 4**. That changed tabs almost immediately after focusing and expanding the original-source **Auto Block → Main → Range** slider. The actual source slider was therefore available for only about one drawing frame before the diagnostic switched tabs. A credible manual/foreground real-game hover test needs a longer source-controlled opportunity without modifying Minecraft input.

## R178 implementation

The same source-verified R99 client render diagnostic now calls `RendererWglEvidence.retainedHoverAction(drawFrame)` and checks `expectedRetainedHoverState` for **every** original-game frame.

| Render frame | Inert local navigation |
|---|---|
| 1 | NextRow (source Auto Block card) |
| 2 | ExpandFocused (open source numeric settings) |
| 3–60 | No navigation; keep the recovered slider visible |
| 61 | ToggleFocusedPreview (original third action) |
| 62 | NextTab (original fourth action) |
| 63–64 | No navigation; verify final tab/focus and close |

The source-backed expanded slider is available **during drawings 3–62**, exactly **60 draw frames**. The original **four local-only actions** remain identical and in the same order. The final menu state is unchanged: tab 1, focused index 17, preview 1, four actions, zero rejected actions. No synthetic OS input or Minecraft mouse/keyboard interception is introduced.

In the original client, the new Java 8 schedule validates the returned **real C++ native menu snapshot on each of the 64 draw frames**. A wrong tab, focus, action count or preview value fails the diagnostic. The restored original class bytecode and JNI/WGL teardown checks remain mandatory.

## Exact original Minecraft acceptance performed

On November 8, 2026, a supervised, isolated, separate original vanilla Minecraft 1.8.9 Java 8 process was launched under an offline test identity with the exact pinned original client JAR and original-source class verification. Its R99 retained-menu diagnostic completed successfully, verified by independent loader-side source, GL context, class-restoration and release receipts. It printed:

`R178_ORIGINAL_VANILLA_189_SLIDER_WINDOW_SOURCE_PASS eligible_draws=60 foreground=0`

The original JVM also produced:

`R178_ORIGINAL_SOURCE_SLIDER_HOVER_WINDOW_PASS eligible_draws=60 frames=64 actions=4 foreground=0 native_click_delivery=disabled`

The paired original-JVM R176 receipt retained `frames=64 samples=63 foreground=0 numeric=0 bind=0`.

This proves actual **source-validated native menu state during 60 original-game draw frames**, **not** that the Windows cursor was over the slider or that an actual foreground tooltip appeared. No foreground focus was taken from other windows; the game process was terminated after its own acceptance.

## Regression tests and negative controls

- `R178RetainedHoverScheduleSmoke` is a Java 8 test-only fixture that checks all 64 frame/action/state combinations, the exact 60-frame source slider window and preservation of the original four local-only actions.
- The fixture mutates **every field of the native six-integer menu snapshot on every frame**. Each invalid snapshot must fail the runtime acceptance predicate.
- `tests/test_r178_original_slider_window.py` rejects missing, duplicate, prefixed, suffixed, malformed, incorrect frame/action or inconsistent foreground markers.
- The original-game launcher `windows_vanilla189_isolated_manual.py` strictly requires both the earlier R176 runtime hover counter receipt and R178's source-window receipt from the **same original game stdout**, including identical foreground counts.
- The R178 Java 8 test-only fixture is excluded from the production instrument JAR and original bytecode audit JAR.
- The new Java 8 CTest is registered only when the attach agent exists. The JNI-only build configuration does not require `slinkywinky_loader`.

## Still not proven

Foreground/original-client hovered numeric tooltip visualization is **not** established in this run: the observed real-game foreground count was zero. The next milestone is a separately admitted, user-approved or genuinely noninterfering **real original foreground window** test that verifies an exact source hover hit and pixel-level result, while preserving original input and restoring desktop focus.

No original Minecraft clicks are suppressed or forwarded; no gameplay module execution is enabled by this change. PR #1 remains draft.
