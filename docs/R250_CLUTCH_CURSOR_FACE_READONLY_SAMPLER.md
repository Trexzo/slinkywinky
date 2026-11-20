# R250 — original Minecraft 1.8.9 cursor face observation (read-only)

Date: 2026-11-20. PR #2 only, PR #1 unchanged.

## Reference, scope and provenance

The private recovered `Slinky-Verified-SourceBuild.zip` has an independently
reconstructed `ClutchModule.State.placementTick`: its event-0
decision checks `hit.typeOfHit == BLOCK`, exact block X/Y/Z and
`centered != (hit.sideHit.ordinal()!=1)` before calling `press()`.
`ActionAccess.adjacentPosition` uses original 1.8.9
`EnumFacing.getFrontOffset{X,Y,Z}` mapping.

R250 introduces `ClutchFaceSampler` with an exact previous candidate
coordinate and top/side expectation, and one owner-thread cursor hit per
strictly monotonic tick. It compares all six faces and computes a bounded
adjacent coordinate only after block-and-face verification. World or player
changes, foreign threads, invalid ordinals, replayed ticks, overflow,
and contradictory observations fail closed. Null hit is observed absence.

The Forge `ForgeClutchWorldPort` implements an actual read-only
`Minecraft.objectMouseOver` block/side sampler. It performs no
Minecraft API mutation or packet, mouse, hotbar, or rotation actions.

## Evidence and limits

Pure Java8 negative controls run in the R237 workflow. The experimental
Forge 1.8.9 JAR workflow verifies legacy Minecraft API compatibility.
This is a **data adapter only** until a genuinely isolated in-world
trial produces a real hit receipt. Tick-END cursor observations can lag
the rendering event, so they are NOT claimed to be original event-0
or successful placement. There is no activation of gameplay behavior.

The R248 offline C++ gate already uses the same face indexing. R250 does
not connect cursor observations to a native click or claim module coverage.
Next work must verify real prior-tick proposal ownership, add a bounded
receipt path into the optional Forge diagnostic and prove absence/presence
of rays in a single world before producing an acceptance marker.
