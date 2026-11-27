# R356 — Lag Range original player coordinate source parity

The private candidate `c++_possibly_OpenSlinky-main.zip`,
`src/logic/combat/lag_range.cpp`, reads
`java_get_entity_other_player_mp_other_player_mpx(ctx->jvm,target->ent)`
at `lag_range_tick_decide_whether`. The resulting target position
feeds the later `lag_range_packet_hook_hold` movement-distance
comparison. R337 incorrectly supplied `Entity.serverPosX/Y/Z /32`,
which can differ and invert the hold decision.

R356 implements optional local, read-only
`-Dslinkywinky.r356ExactLagTargetReadOnly=true`, with the existing
R337/manual-F9/singleplayer gates. Strict mode accepts only a selected
`EntityOtherPlayerMP`, copies the candidate's three private doubles
using the R355 exact 1.8.9 mappings, bounds/rejects malformed values,
and forwards only those copied coordinates to real native
Lag Range event18. No other game object/reference crosses JNI. For
other living entities the strict source target bit is false; missing
candidate fields for an observed selected player fail closed.

The same immutable source coordinate triple now supplies R349
Backtrack reach evidence, avoiding three redundant reflective reads.
The separate R342/R343 vanilla `serverPos` provenance tracker stays
unchanged.

`r356_java8_native_lag_range_source_position` runs actual Java8/JNI
C++ e18->e3 with a discriminating fixture: target source private X=+10
but vanilla `serverPosX/32=-10`. An outgoing movement at X=+8
returns native hold=true using original private X and hold=false when
the prior vanilla fallback is substituted. Tests also cover all
three mappings, wrong types, nonfinite/bounds and wrong target type.

Still missing: original combat-target selection timing, actual native
network-thread packet hold/release, in-world original positive receipt,
full injectability, GUI/settings parity. This diagnostic remains
read-only and off by default. PR #2 stays draft; PR #1 unchanged.
