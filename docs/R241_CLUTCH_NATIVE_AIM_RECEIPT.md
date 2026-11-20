# R241 — Clutch source aim returned through real Java 8 JNI

Date: 2026-11-20. Draft SlinkyWinky PR #2, based on R240.

## Gap closed

R239 returned only the chosen block and face. Yet the original-source
`clutch_plan_original` already calculated a source-randomized 3D aim
point and signed yaw/pitch differences. Discarding them made it
impossible for Java to verify the exact native plan's intended aim.

## Implementation

- The native read-only JNI Clutch receipt now returns 15 values rather
  than 9, retaining the original nine plus aim X/Y/Z, signed yaw/pitch
  deltas and the preserved-prior flag.
- Java 8 `ClutchNativePreview.Decision` validates finite bounded aim,
  expected selected-block geometry, top-face height, and that signed
  angle magnitudes reproduce the reported angular cost within .02.
- R239's no-prior admission stays explicit: a preserved-prior proposal
  is rejected until the source-ordered raycast phase is implemented.
  Missing blocks must return zero aim and zero rotation.
- The independent JNI smoke checks a real source-native selected aim,
  exact fresh-owner deterministic replay, air/no-aim and R240 owner
  rejection. Forge's isolated read-only PlayerTick preview counts
  accepted aim receipts, **without applying any rotation or gameplay
  changes**.

## Remaining work

R241 does not implement previous-block retention, authentic event-18
hooking, real game-controlled rotation, click/placement, snap-back,
hotbar ownership or original full-client reconstruction. The fixed
`{1,2,3,4}` JNI seed is for deterministic regression only.
Verify the exact commit's Java8/JNI CTest and Forge compilation before
claiming success. The private decrypted DLL was not available for new
differential tests. Keep both PRs draft/unmerged.
