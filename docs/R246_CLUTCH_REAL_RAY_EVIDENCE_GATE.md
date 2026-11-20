# R246 — meaningful real-world Clutch prior ray evidence

Date: 2026-11-20. SlinkyWinky PR #2 (draft, unmerged); PR #1 unchanged.

## Corrected acceptance contract

R245 printed a generic R237 `PASS` after 20 source-ordered candidate
scans even if the opt-in prior ray path never made a call to
`WorldClient.rayTraceBlocks`. This is insufficient to validate R245.

R246 separates three outcomes:

- `R237_REAL_WORLD_CLUTCH_167_READONLY_PASS`: original R237 mode only,
  20 successful read-only snapshots.
- `R246_REAL_WORLD_CLUTCH_PRIOR_RAY_INCONCLUSIVE`: R245 mode completed
  20 read-only snapshots, but no authenticated prior ray was requested.
- `R246_REAL_WORLD_CLUTCH_PRIOR_RAY_READONLY_PASS`: R245 mode
  completed exactly 20 same-world snapshots and executed one or more
  source-authorized prior rays, through the actual Minecraft world adapter.

The classifier rejects inconsistent counts, including more ray calls
than prior requests, more retained priors than requests, unsupported
native plan or aim totals, or evidence from fewer than 20 samples.

Every new world/player observation owner now starts with fresh
candidate, native decision, and ray counters. A prior candidate
proposal is still **not** proof of successful placement. No blocks,
rotation, hotbar, packets or input are changed by this diagnostic.

## Verification

Pure Java 8 test:
```sh
javac --release 8 -d build/r246 \
  java/dev/slinkywinky/ClutchRayReceiptStatus.java \
  tests/java/dev/slinkywinky/ClutchRayReceiptStatusSmoke.java
java -cp build/r246 dev.slinkywinky.ClutchRayReceiptStatusSmoke
```

The dedicated R237 workflow runs that regression. The Forge 1.8.9
workflow compiles the changed real adapter and preview against the
legacy Java 8 game API.

A **real local singleplayer session** is still required to demonstrate
that `raycasts>0` and the R246 success marker actually occur.
Neither a passing Forge build nor synthetic Java/JNI fixtures can
substitute for this evidence. Even a real R246 pass does not establish
source-faithful Clutch placement, input hooks, or full client parity.
