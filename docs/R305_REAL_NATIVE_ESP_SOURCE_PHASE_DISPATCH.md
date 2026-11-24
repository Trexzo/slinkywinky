# R305 — source ESP phase dispatch in real Java8 → JNI → native C++

## Problem: R304 used a single eager native observation

R301–R304 previously updated all three first-party native ESP
controllers at once during the same Forge `PlayerTickEvent.END`
callback. This was useful for verifying actual source row algorithms,
but it did not preserve the recovered source's distinct semantic event
subscriptions:

| Source semantic event | Recovered behavior | R305 first-party dispatch |
| --- | --- | --- |
| 0 | Chest ESP collect/clear | `ChestEspController::tick` |
| 18 | Item and Player ESP collect | `ItemEspController::sample`, `PlayerEspController::update` |
| 6 | World-space ESP rendering | Render-plan admission for world-last callback |
| 7 | Player ESP screen-space rendering | Source phase admission only; 2D not implemented |
| 26 | Item ESP clear-policy and rows | Clear Item policy/rows plus all ESP rows on teardown |

## Opt-in native phase session

`EspNativeReadOnlyPreview(true)` enables a new ABI-compatible
`nativeEnableSourcePhases` on the newly created JNI handle. The
legacy default constructor and `nativeObserve` remain unchanged,
preserving earlier R301–R304 DLL/JAR behavior.

In phase mode, `nativeObserve` validates and stages a bounded
world/player snapshot **without calling any ESP controller**. It returns
raw input dimensions with `-1` for all pending row counts. Actual
per-source algorithms execute **only** after:
1. event 0 — source Chest collection; retain the source 100 cap and
   return Chest rows; Item/Player remain pending;
2. event 18 — source Item and Player collection, count merging, health,
   relationship filtering and farthest-first ordering.

Only after these phases may the same session admit event 6, which
permits the R303/R304 native plan queries. Event 7 must follow
event 6 in the same world/player/generation/tick; 6 and 7 may
repeat during a single simulation tick as real render callbacks do.
Event 26 revokes rendered rows and clears the original Item policy.
A world generation transition also invalidates old rows.

All phase calls use the existing single JNI creating thread, the
exact most recent owner world/player identity, generation and tick,
and bounded counts. Duplicate or out-of-order events, stale ticks,
world switches, foreign threads and premature rendering fail closed.
R305 tests all of these against the actual C++ controllers via Java8
JNI, while keeping the prior R301-R304 smoke suite.

## Forge 1.8.9 experiment

R305 adds an **exclusive**, opt-in R207 isolated launcher mode
`-R305SourcePhaseTest` with `-R305ExpectedJarSha256`,
`R305Offline` dummy identity and direct TEMP-only
`slinkywinky-r305-independent-forge-<unique>` game folder. It
creates only the disposable `R305-local-only` flat world, and
starts the native observer once. Older SHA-pinned modes are untouched.

The admitted real Forge client `PlayerTickEvent.END` stages
one snapshot, then invokes source event 0 and event 18 in sequence.
`RenderWorldLastEvent` calls event 6 before the existing Chest and
Item/Player diagnostic draw paths. An eligible screen overlay callback
signals event 7; world loss calls event 26 before sampler retirement.
Each phase has an independent evidence marker.

**Important fidelity boundary:** The source controller dispatch is
now separated, but event 0 and event 18 are still invoked at adjacent
points in the same Forge PlayerTick.END callback, not at proven original
native callback priorities or actual original commercial event times.
Event 7 signals admission, not original Player 2D projection. Neither
new event phases nor GitHub tests prove visual parity, original 66
descriptor runtime, vanilla injector ownership, pixel capture or
real user-visible 1.8.9 gameplay. The owner desktop was offline.

The new R305 Forge 1.8.9 build passed on commit `5ce4081`
(GitHub Actions run 45473900295). Its `slinkywinky-forge189-preview-jar`
artifact was downloaded: the packaged JAR SHA-256 independently matches
the enclosed `forge-preview.sha256` manifest, and required renderer and
Java JNI classes are inside the JAR. The exact R305 JAR SHA-256 is:

`05e6f837f7ba2c953f1e2974bca364f63348e1bd32b8b084cb1c1664ac60aa0b`

The new R305-only launcher path pins that exact reviewed hash in code
and rejects a caller-supplied alternate digest. R302, R303 and R304
hash pins remain unchanged. No owner-machine runtime acceptance has
been performed. Keep PR #1 and PR #2 draft and unmerged.

## Target runtime evidence

- `R305_SOURCE_PHASE_ISOLATION_GATE_PASS`
- `R305_REAL_NATIVE_SOURCE_0_18_PASS` after 30 tick samples
- `R305_REAL_NATIVE_SOURCE_EVENT6_PASS` after 30 world render frames
- `R305_REAL_NATIVE_SOURCE_EVENT7_PASS` after 30 overlay callbacks
- `R305_SOURCE_EVENT26_UNLOAD_PASS` after an actual world exit/swap

Live positive ESP geometry depends on genuine chests, dropped items
and remote players; the default flat singleplayer world may show zero
matched item/player rows. Do not invent visual pass receipts.
