# R358 — Real original vanilla 1.8.9 client-thread -> native source JNI

**Runtime:** The existing bounded standalone Java8 Attach/Instrumentation
tick hook now has a separately opt-in, read-only path:
`-Dslinkywinky.r358OriginalSourceTickReadOnly=true`.
This mode requires the exact original obfuscated `ave` class and
SHA-256 `430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82`
before installing the instrumented client-tick callback. The default
probe, Forge experiment, GUI and input behavior remain unchanged.

On the admitted real Minecraft game thread, `ObservedState.sample()`
optionally captures the selected original `bex` target's
`c,d,e` double fields and the player's `pr.bl` attack tick using
`Vanilla189SourceEvidence`. All results are immutable primitives,
with exact original class/type admission, bounded finite values, cached
reflection, no JNI refs or additional gameplay writes, and explicit
release of reflection/world identity on teardown.

The existing standalone agent already calls `System.load` on the
authorized absolute `slinkywinky_jni.dll`. R358 additionally
packages the **existing** `SourceRuntimePreview` JNI facade and
opens its native source C++ session on that SAME client thread.
Every admitted original tick dispatches native Lag Range event18,
using observed source remote coordinates, fixed opaque owner tokens
and the actual client-sampler world/player generation. An absent
or non-bex target produces a native inactive tick, not stale
target ownership. Both source and original observation JNI owners
close on the owner thread before the existing classfile restoration.

**Important:** This is the first opt-in *actual standalone injected
vanilla game tick -> native reconstructed Lag Range decision*.
It is not the original Slinky event18 timing, attack-phase callback
or packet owner. Target selection is the Minecraft ray selection;
range/weapon settings are explicitly synthetic conservative
data-only defaults, the distance uses the existing observed ray
coordinates, and event3 packet hold/replay is not wired.
The source JNI observations are telemetry, not game actions.

The new R358 Java8 CTest exercises source-field capture with
synthetic exact-named class fixtures (not original proprietary bytes),
immutable copying, cache refresh/reset, absent/non-player targets,
NaN and world bounds. The native loader packaging, source class SHA
gate, actual original Minecraft world execution and JNI cleanup
still require exact-head CI and in-game acceptance.

Keep PR #2 draft, PR #1 unchanged and unmerged.
