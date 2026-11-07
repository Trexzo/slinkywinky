# R149 — ClickGUI tab/open motion and source-family typography

**Repository:** Trexzo/slinkywinky · `reconstruction/r1-core-fidelity` · draft PR #1, **not merged**.

## Provenance, not an authenticity claim

The user's candidate archive `decrypted files.zip` contains `decrypted files/re/clickgui.tpl.html`, a reconstructed HTML visualization. This is **not an authenticated original Slinky binary or signed native GUI source**.

The reference supplies the following concrete visual behaviors:

1. Selected nav pill: `st.anim += (st.tab-st.anim)*min(1,dt/0.05)`, snapping when difference is less than `0.001`.
2. Page movement: `translateY(-0.1 * viewportHeight * (tab-anim))`.
3. Page opacity: `max(0, 1 - 2*abs(tab-anim))`.
4. Open animation: opacity 0→1, vertical offset -50→0 in **250 ms**, `ease-out`.
5. Preferred typography: `font-family: Inter, system-ui, sans-serif` and **weight 500** with grayscale antialiasing.

The auxiliary R120 native-registration audit bundle explicitly contains no source fonts or screenshot assets. That bundle cannot establish which actual typeface is present inside a signed commercial release.

## Implementation

- New deterministic, cross-platform `MenuVisualMotionClock` in `include/slinkywinky/menu_visual_motion.hpp` implements the recovered tab lerp exactly, including snapping and an elapsed-delta bound. Page displacement and opacity are calculated from the same intermediate pill position; opening slide/opacity uses an original cubic ease-out approximation over **250 ms**.
- A renderer-neutral `MenuVisualMotion` is attached as an **optional field** in `MenuRenderFrame`. Existing callers (including the JNI/host paths and test frames) do not supply it, keeping their historic static rendering unchanged.
- The GL11 renderer uses the motion only for decorative tab highlighting, whole-panel opening transform/fade and page-card movement/fade. A scoped alpha multiplier restores its previous value on early returns; existing R67 fixed-function GL state restoration remains in effect. Confirmation dialogs stay above page motion.
- In the standalone Windows GUI preview, a **16 ms window-owned timer** supplies time progression. On each tick, a settled frame stops repainting. The timer is destroyed with its window and **never registers global hooks or touches Minecraft**. If timer setup fails, the preview retains the static renderer behavior.
- Because page cards are translated visually, pointer-down, idle-hover and mouse-wheel admission are **suppressed until the motion settles**. This guards against rendering a clickable control at a position different from the source-indexed hit-test geometry. Keyboard tab navigation remains supported. Neither module states nor native game settings are modified by animation.
- The existing GDI font renderer now checks whether the **exact Inter font family is locally installed**. If present, it rasterizes Inter with a requested Windows GDI weight of 500. Otherwise it uses Segoe UI at the same weight. **No third-party font file is distributed, registered or copied**, and no font from the decrypted candidate is bundled.

## Regression

`native_recovered_gui_tab_open_motion` checks:

- A zero-time first frame with opening offset -50px and zero opacity
- 50ms tab lerp, 250ms opening duration and frame-to-frame settling
- Correct 10%-height page offset and calculated opacity
- Validity guards for nonsensical tabs, non-finite elapsed time/visual values and tiny panels
- Reset, reverse direction and animation-model independence from actual selected game/menu tab

The existing Windows GL11/Java8 bridge smoke suites compile and exercise the renderer through its previous static API. These are code tests, **not pixel-accurate screenshot comparison to original Slinky**.

## Remaining limits

The original HTML uses CSS `ease-out` as a Web Animations easing function, which does not translate exactly to the first-party cubic ease-out approximation. The visual frame is also currently **suppressed from pointer input during page motion**, unlike a DOM that could in theory accept input while moving. The native renderer's glyph metrics, fallback rasterizer, style, baseline and kerning are not independently measured against actual original Slinky screenshots. Real embedded-font evidence would be needed to establish pixel-perfect typography; source UI content, not copyrighted fonts, is the current permitted reference.

**Verification:** Check GitHub Actions on the exact latest PR head. Successful CI proves the model and renderer build/test boundaries, not runtime gameplay module completion or original GUI screenshot parity.
