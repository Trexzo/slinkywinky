# R295 — recovered Chams Java filtering and GL begin/end

Source: user-provided recovered `ChamsModule.java`, Java 8
`dev.slinky.module.impl.ChamsModule`, from the verified bundle.

Original `eligible` formula:

    isEnabled() &&
    (RelationshipService.flags(entity) &
       (flag("hide_friendlies") ? 5 : 0) + 2) == 0

Java `+` binds more tightly than bitwise `&`. Consequently
`hide_friendlies=false` tests relationship mask 2 (Anti Bot);
`true` tests mask 7 (friend bits 1/4 plus Anti Bot 2).
The recovered Java method `begin` enables OpenGL capability
32823 (`GL_POLYGON_OFFSET_FILL`) then calls glPolygonOffset
with floats from RenderConstants.f(1392420) and f(1392424).
`render` calls original.run inside try/finally and calls
`disable(32823)` only if begin selected the entity.

R295 models these GL calls through a fully synthetic Graphics
port; no actual OpenGL, game render hook or player flags change.
Source-like end behavior on original-render exceptions is tested.
Begin occurs before the source try-finally; this is captured
rather than silently rewriting exception semantics.

R296 has now recovered the **exact** immutable float values from
`RenderConstants.json`; real vanilla 1.8.9 entity rendering, GL state
restoration, graphics initialization and module GUI remain
separate unverified integration gates. This adds one **offline**
module policy to the source-mapping inventory, not a playable
Chams feature or completed Slinky reconstruction.
