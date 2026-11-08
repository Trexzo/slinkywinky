# R180 — painted-frame fence for passive input geometry

Development branch: reconstruction/r1-core-fidelity (draft PR #1). PR #2 untouched.

## Confirmed discrepancy

The previous retained WGL preview could derive source-backed Auto Block
Range slider hit geometry from the current MenuState after an operator
navigation action but before the slider had actually been painted.
An undrawn slider could therefore be reported as a source hit target.

## R180 behavior

WglRetainedMenuPreview stores an immutable MenuRenderFrame snapshot only
after successful native GL11 painting. Its source_hover_target method and
all actual hit tests use the last-painted frame. Navigation and successful
isolated model pointer releases invalidate that snapshot until another
successful draw; close and failed drawing invalidate it as well.

The original JNI WGL render callback needs to observe a frame even while
a prior navigation invalidated the snapshot. In this case observe_hover
admits an **empty passive sample** (preserving 63-frame accounting), clears
visual hover state, and records no numeric, bind, or source hover hit.
It never re-derives geometry from the unpainted model.
Neither an empty sample nor a source rectangle grants click ownership.

## Verification

The disposable Java 8/JNI/WGL tests now assert that expanding Auto Block
Range before painting does not expose a source_hover_target, and a passive
observation of the prepared model location is counted only as an empty
sample with zero actual source hits. After real native GL11 redraw, the
source rectangle becomes available; original owner, focus, viewport,
profile and hit tests continue to apply. The R175 test also verifies
the pre-paint empty-sample contract.

Run the full Windows Release CTest suite and an original vanilla 1.8.9
read-only render acceptance for final runtime confidence.

This is a local presentation/hit-geometry lifecycle fix, not proof of
exclusive original Minecraft mouse ownership, genuine foreground physical
mouse acceptance, gameplay modules, or 1:1 original commercial Slinky.
No upstream OpenSlinky code is redistributed.
