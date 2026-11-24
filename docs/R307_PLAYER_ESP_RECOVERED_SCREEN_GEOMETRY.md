# R307 — recovered Player ESP screen-geometry and health-bar parity

R306 rendered *diagnostic* Player ESP 2D boxes using an invented
`±0.30 × 1.80` box, clamped all visible coordinates to the viewport,
and drew a single health segment. R307 replaces these approximations in
a new **opt-in source geometry mode**, while preserving R306 behavior.

## Evidence and provenance

The archived, inspected `OpenSlinky-main(2).zip` Java 8
`SlinkyClient/src/main/java/dev/slinky/module/render/PlayerEspModule.java`
(SHA256 `8b99ecdb976b33b6195301d149df455dcfbbbdefc8b62a0415fe33eea0179e1b`)
and `RenderMath.java` use a recovered byte-array
`RenderConstants.json` (SHA256
`4b9f955844a5ef071dd0e886ec39f37e7eee1dddf7124f1eb5194a5152adae34`).

The earlier R299 independent source recovery also identified
`source/java/src/main/java/dev/slinky/module/impl/PlayerEspModule.java`
(SHA256 `603a98c58ad4933f519fee362dd1b1a65d8bf381f2b34610950f3d9326f12e2f`)
with the same source event 18/6/7 mode separation and C++ row rules.

Earlier forensic tests showed that the Java constants span matches the
candidate decrypted Slinky binary after pointer relocation
normalization. That establishes shared *binary lineage*, not an
officially authenticated original release or runtime equivalence.

## Source recovery improvements

1. **Eight corners:** The original `RenderConstants.d` span at
   1378976..1379168 yields the exact order (x, y, z):
   `(-.4,0,-.4),(-.4,0,.4),(.4,0,-.4),(.4,0,.4),`
   `(-.4,2,-.4),(-.4,2,.4),(.4,2,-.4),(.4,2,.4)`.
   The source code hard-codes the first corner z = -0.4.
2. **Projection:** The old recovered `RenderMath.project` uses
   Java `strictfp` float arithmetic, model and projection
   multiplication in a specific nested-add order, reciprocal
   `1/w*.5f`, and **strict depth 0 < depth < 1**. R307's
   `projectSource` reproduces this order from the actual Forge
   modelview/projection matrices. Nonfinite results are additionally
   rejected as a safety fence; the pre-existing R306 projection is
   preserved separately.
3. **Visibility:** Instead of rectangle intersection and viewport
   clamping, the recovered `RenderMath.visibleBounds` admits a
   rectangle if **either its top-left or bottom-right corner** lies
   within viewport bounds. Partially visible coordinates are left
   unclamped. This odd rule is preserved deliberately.
4. **Health:** Source `healthBar` uses x =
   `minX - lineWidth - 1 - 4`; first a black segment
   from minY to a `lerp(maxY,minY,health)` point, then an optional
   yellow `(1,0.67,0)` absorption segment, then an RGB
   `(1-health,health,0)` segment ending at maxY.
   R307's pure Java `sourceHealthSegments` exposes these
   calculations to exact regression assertions, and its separate Forge
   renderer draws all segments.
5. **Hurt:** The source derives a blend strength
   `min(10,hurtTime-partialTicks)*.1` when its `hurt_effect`
   setting is on. R307 reconstructs the arithmetic, but still applies
   it to a **diagnostic** base cyan, not the unrecovered player name,
   target, or relationship color.

The new Forge screen path is selected only by explicit
`-R307SourceScreenTest` in the isolated R207 launcher, using a distinct
direct-TEMP-only `slinkywinky-r307-independent-forge-<unique>`
folder and `R307Offline` identity. All earlier SHA-pinned
R302–R306 modes are preserved. A separate Java 8 JNI test asserts
source geometry dimensions, source visibility's unusual two-corner
behavior, health segments/absorption split, hurt interpolation, and
stale identity rejection using actual R299 native Player ESP rows.

## Additional R306 launcher bug fixed

The R306 PowerShell branch previously had
`if($R306PlayerScreenTest -or ...)` inside an enclosing
`if($R306PlayerScreenTest){...}`, making *every* R306 invocation
fail its own exclusivity check. R307 removes the self-condition and
adds a Windows PowerShell 5.1 negative source gate. This is independent
of in-game ESP quality.

## Boundaries not to overstate

The source's `show_2d_box`, `filled`, `show_health_bar`,
`hurt_effect`, per-player color and actual configurable line width
are **not transported** from original user settings in this diagnostic
run. The R307 test draws the box and health bar unconditionally at
diagnostic line width 1.25f. This is geometry/operand parity, **not
source-faithful configuration or pixel-parity rendering**.

The actual owner PC remains offline, and no running Minecraft 1.8.9
video, player fixture, screenshot or frame-time benchmark exists yet.
No production vanilla injection, original event priority, original
GUI/HUD or full 66-module completeness is claimed.
Both PRs stay draft/unmerged.
