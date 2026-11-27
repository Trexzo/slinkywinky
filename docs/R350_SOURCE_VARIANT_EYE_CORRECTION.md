# R350 — Correct the source Backtrack eye-coordinate variant

## Evidence from private candidate archive

This milestone inspects the actual retained
`c++_possibly_OpenSlinky-main.zip`; no recovered bytes are committed.

1. `src/logic/combat/backtrack.cpp` first reads `java_get_entity_pos_x(ctx->jvm,ctx->player)`. Only if `java_variant(ctx->jvm)` is true does it add `vec3_add_eye_height_sneak`; this eye vector is passed into `entity_reach_check`.
2. `src/engine/jni/jni_entity_rotation.cpp` defines `java_variant(jvm)` as `ids->variant == 0`.
3. `src/engine/gui/jni_ids.cpp` branches around the BlockPos-dependent JNI initializer for `variant == 0`, identifying that as the old 1.7.10 path; `variant == 1` is the 1.8.9 path.
4. `src/engine/math/vec_aabb.cpp` implements `vec3_add_eye_height_sneak` via `unused_constant_neg_1_62_b`, whose `src/data/rdata.s` scalar is **-1.62**, not +1.62.
5. The source `entity_reach_check` compares the distance to `EntityOtherPlayerMP_otherPlayerMPXYZ` against the distance to `Entity_posXYZ`, with strict `> 0.01`.

## Corrected implementation

R349 mistakenly used `self.posY + self.getEyeHeight()` in the
**1.8.9** Forge source-world Backtrack reach comparison. In the actual
candidate 1.8.9 branch, the eye vector is the raw player position.
R350 corrects `ForgeSourceWorldPort.capture()` to explicitly use
`SourceBacktrackReachEvidence.candidateEyeY(1,self.posY)`.

The helper validates candidate variant and player Y; it also exposes
the distinct 1.7.10 source path for independent cross-checking,
without selecting that path for the 1.8.9 Forge adapter.

## Discriminating regression

Player base Y=0, target current=(2,1.62,0), target shown=(2.012,1.62,0):
- Correct source 1.8.9 eye=(0,0,0): distance advantage ~0.009336.
  Strict source `>0.01` returns **false**.
- R349 erroneous eye=(0,1.62,0): distance advantage 0.012.
  Incorrectly returned **true**.

The R349 Java 8 CTest now covers both outcomes, 1.7.10 -1.62
variant-specific scalar, invalid variants and invalid coordinates.
The existing CMake/ForgeGradle workflows execute the revised test and
rebuild the production Forge snapshot adapter.

**Scope:** opt-in/read-only diagnostics only. This fixes a verified
source-mismatch, not original native hook timing, exact 1:1 gameplay,
packet retention, injectable completion or an in-game Backtrack
acceptance receipt. Keep PR #2 draft and PR #1 untouched.
