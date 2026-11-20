# R240 — Clutch JNI sampler-scope and geometry provenance

Date: 2026-11-20. Changes confined to SlinkyWinky draft PR #2.

## Problem found in R239

R239 bound the C++ `ClutchObservationOwner` to constant JNI
`world_token=1,player_token=1`. This made the native source gate
unable to distinguish two different Java `ClutchWorldSampler` owners
with monotonically increasing ticks. Also, `ClutchNativePreview.observe`
constructed its geometry from the caller's `Request`, rather than the
previously captured `Snapshot`. Two requests could share tick and
generation but disagree on eye/AABB coordinates or range.

## Correction

- `ClutchWorldSampler` allocates two positive, value-only, process-local
  tokens per new sampler session, with exhaustion refusal. No Java object
  pointers or world/player identities cross JNI. Sampler world/player
  reference checks remain authoritative on the Java side.
- `Snapshot` captures the same range, height, AABB, eye and prior-request
  presence as the block observation, with private copies and a package-
  private equality gate. JNI geometry is assembled from the captured
  snapshot rather than a substitute request.
- The C++ source gate already demands the same world/player tokens,
  unchanged generation and increasing tick. It now receives genuinely
  session-specific tokens instead of constants. Cross-session use
  permanently quarantines that native owner.
- Real Java 8 JNI smoke adds same-tick altered-eye and altered-range
  negative controls, independent sampler on a newer tick, and persistent
  native quarantine. All decisions remain read-only.

## Evidence boundary

This validates cross-boundary **consistency**, not cryptographic proof
of a genuine Minecraft client observation. A deliberately compromised
Java producer remains out of scope. No source raycast, randomized previous
block retention, rotation, click or placement is implemented. Forge
PlayerTickEvent.END is still not original Slinky event timing. The
reference DLL is not bundled and original commercial parity is unverified.

Validation target: `r239_java8_native_clutch_source_planner` in
`.github/workflows/r239-clutch-jni-source.yml`. Verify the exact
new commit's CI receipts independently. Both PRs stay draft/unmerged.
