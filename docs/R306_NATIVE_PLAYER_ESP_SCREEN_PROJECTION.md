# R306 — real native Player ESP source event-7 2D projection experiment

## Scope

The original-recovered `PlayerEspModule.java` (SHA-256
`603a98c58ad4933f519fee362dd1b1a65d8bf381f2b34610950f3d9326f12e2f`)
subscribes to source event 18 for player-row collection, 6 for world
3D mode, and **7 for screen 2D mode**. R305 separated event admission,
but did not draw from event 7. R306 makes an actual experimental
native-backed screen-space drawing path, **not** an authenticated
replacement of the commercial Slinky GUI or render algorithm.

### Actual R299 native player source rows

`nativePlayerScreenRows` now requires the same live, thread-owned
world/player/generation/tick and, importantly, a completed original
semantic source **event 7**. Event 6 alone cannot read screen rows.
The native adapter calls the existing R299 `PlayerEspController`
for **source-selected farthest-first rows**, their previous/current
positions, normalized health, absorption ratio, hurt time, with the
same 100-row cap. Interpolation occurs before world-camera subtraction.
No Java-side player re-filtering, separate fake roster or world writes.

### Actual Forge camera projection

The experimental `RenderWorldLastEvent` captures current
`GL_MODELVIEW_MATRIX`, `GL_PROJECTION_MATRIX`, and `GL_VIEWPORT`
**before** the diagnostic world ESP rendering; this capture is bound
to the same native receipt and tick. `RenderGameOverlayEvent.Text`
admits native source event 7, asks the same native owner for
`ScreenPlan`, and calls independently tested
`PlayerEspScreenProjection.Frame.project()`. The pure Java 8
projection multiplies the real OpenGL column-major matrices,
perspective-divides all eight corner points of a diagnostic human-sized
bounding box, converts to Minecraft scaled GUI coordinates, rejects
behind-camera/near-plane-straddling boxes and off-screen rectangles,
and clamps visible boxes to screen edges.

`ForgePlayerEspScreenRenderer` draws diagnostic 2D line boxes and
normalized health bars within pushed/popped GL11 attribute/matrix
state. R306 suppresses the previous 3D player wireframes, leaving
Chest and Item diagnostics unchanged. Shape, color, line thickness,
bounding box dimensions and projection clipping are explicitly
**not claimed original Slinky pixels**. GPU readback overhead of
GL11 matrix capture is not measured, and real frame-time acceptance
remains to do.

### JNI/source regressions

The real native Java 8 CTest now proves:
- Event 6 alone rejects screen-plan requests, event 7 admits them;
- native rows match first-party C++ Player ESP sorting and health;
- exact known column-major matrix fixture projects a bounding
  rectangle to scaled GUI coordinates;
- off-screen geometry is suppressed rather than fabricating a box;
- stale/mismatched world metadata, nonfinite matrix and partialTicks
  are rejected.

R306 requires its own explicit R207 mode
`-R306PlayerScreenTest` with an exact-review SHA-pinned Forge JAR,
new direct-TEMP-only `slinkywinky-r306-independent-forge-<unique>`
game folder, dummy username `R306Offline` and a disposable
`R306-local-only` singleplayer world. Earlier R302–R305 hashes/modes
remain unchanged. This is not enabled in ordinary Forge/production
Minecraft or PR #1's injector.

Runtime logs distinguish **frame callbacks** from an actually
positive geometry row:
`R306_PLAYER_SCREEN_CALLBACK_PASS` after 40 overlay callbacks,
and `R306_REAL_NATIVE_2D_PLAYER_SUBMITTED` only after a visible
native player row produces a 2D rectangle. A new empty singleplayer
flat world usually has **no remote players**, so the latter should not
be expected without a real controlled player fixture.

### Still outstanding

No owner Windows desktop was available for actual in-game rendering
or pixel screenshots; original settings/color, text, edge geometry,
clip-space near-plane behavior, true event priorities, ClickGUI/HUD,
R306 performance, full module coverage and production vanilla injector
fidelity remain **unverified**. Neither a Forge CI build nor a
positive regression count is proof of full original Slinky parity.
Keep PR #1 and PR #2 draft/unmerged.
