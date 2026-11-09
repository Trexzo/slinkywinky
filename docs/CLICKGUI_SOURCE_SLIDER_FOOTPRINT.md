# ClickGUI source slider — client-pixel footprint verification

Development branch: `clickgui/source-slider-footprint-20261109`, based on the active PR #1 ClickGUI reconstruction branch. The OpenSlinky C++ integration PR #2 is untouched.

## Failure mode addressed

The original R179 source-backed Auto Block / Main / Range slider target
required the center pixel to resolve to the unique recovered slider.
The target also returned a physical client-pixel rectangle, but **had not
independently established that its four corner pixels belonged to the
same hit-test identity**. After fractional panel scaling or another
geometry change, a center hit alone can be insufficient to certify
the claimed rectangle.

## Bounded change

`menu_original_auto_block_range_target` now requires the actual
`hit_test_menu` to resolve the center **and all four inclusive interior
corners** to the same recovered widget ID, index and slider kind before
publishing a target. The check uses the immutable render frame and
does not read or synthesize mouse input.

The existing Java 8 disposable native WGL/JNI fixture exercises all four
corner positions through the actual passive hover observer, in addition
to the original center, wrong-thread, wrong-viewport, foreground-loss,
unpainted-frame and close/teardown checks.

This is a conservative **five-point sampled footprint guarantee**, not
a mathematical proof of every interior pixel or a certificate that the
native GL11 slider is visible to a human operator. Existing original
Minecraft sessions with no foreground cursor hits still provide only
geometry evidence.

## Verification

Run the existing Windows Java 8/JNI fixture:
`R179ExactSourceTargetJniSmoke` via its registered CTest suite.
Check all GitHub Actions jobs for the final branch SHA. A passing
synthetic fixture does **not** replace a fresh original vanilla Minecraft
1.8.9 in-game foreground hover acceptance (R181).

No original/commercial binary, font, decrypted data, upstream C++
source or asset is included. No Minecraft input capture, gameplay
module enablement, Forge production dependency or click dispatch is
introduced. Keep the integration draft/unmerged until full acceptance.
