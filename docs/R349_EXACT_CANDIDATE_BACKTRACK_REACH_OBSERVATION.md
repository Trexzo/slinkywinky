# R349 — Original Backtrack reach evidence, read-only

Reference examined privately: `c++_possibly_OpenSlinky-main.zip`,
`src/engine/modlib/reach_distance_checks.cpp`,
`src/engine/jni/jni_entity_accessors.cpp`,
`src/engine/jni/jni_entity_rotation.cpp` and `src/data/rdata.s`.

Candidate reach equation: **distance(eye, EntityOtherPlayerMP.otherPlayerMPXYZ)
minus distance(eye, target.currentXYZ) > 0.01**. The private
`rdata.s` scalar is the double `0.01`. The candidate separately reads
`EntityLivingBase.lastAttackerTime` and requires an attack within 20 ticks.

## R349 implementation

- Pure Java 8 `SourceBacktrackReachEvidence` independently implements this
  strict geometry and a conservative recent-attack snapshot. Invalid,
  unbounded, unknown and future inputs fail closed.
- The Forge 1.8.9 source-world adapter adds the separate property
  `-Dslinkywinky.r349BacktrackExactReachReadOnly=true`, alongside its
  R337 read-only source flag and manual F9 opt-in.
- It resolves the **candidate-named original fields** via read-only
  reflection only on a real `EntityOtherPlayerMP`; if an installed mapping
  lacks those fields, it rejects admission, rather than substituting
  `serverPosX/32` or invented attack timings. Nonplayer targets and
  normal/default runs preserve the previous reach=false behavior.
- JNI only receives the copied tick int/bool evidence. No packet, world,
  entity or keyboard references cross the source controller boundary.

## Evidence and limits

New `r349_java8_source_backtrack_reach_geometry` regression tests threshold,
negative/future/expired attack times, nonfinite/bounded coordinates and
translated positions; local Java 8-compatible tests passed. CI must still
verify the complete ForgeGradle build and packaged class. No live positive
EntityOtherPlayerMP receipt, original attack-event timing, eye-height
variant parity or physical game input is claimed. This is a conservative
source-faithful start gate, not a fully functioning Backtrack module.

No packet holding, replay, game writes, or original proprietary code
included. PR #1 unchanged; PR #2 stays draft and unmerged.
