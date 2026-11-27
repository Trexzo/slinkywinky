# R360 — Recovered original combat clock and bounded attack-phase observation

The private reference archive `c++_possibly_OpenSlinky-main.zip`,
`src/data/rdata.s` + `jni_ids.cpp`, identifies the original 1.8.9
Entity ticks field as `pk.W:I`, MCP `ticksExisted`, SRG
`field_70173_aa`. In the same original variant, EntityLivingBase's
last-attack timestamp is `pr.bl:I`.

R360 updates the original Mojang 1.8.9 SHA-pinned static ClassFile audit:
it now checks `pk.W:I` as well as the previously verified
`pr.bl:I` and `bex.cde:D`. A wrong/missing tick field fails CI;
the original JAR stays temporary on the GitHub runner.

New `Vanilla189CombatClock.read(player)` is **independent of target
selection** and returns an immutable `{playerTicks,lastAttackerTime}`
snapshot from the exact obfuscated ancestor classes. Invalid,
future, negative or mismapped fields fail closed.

The separate `SourceAttackTickFence` accepts a candidate for e15 at
most once per strictly advancing original player-tick and
new `lastAttackerTime == playerTicks` equality. It never asserts
an attack on first attachment, repeated tick, target loss, reentry,
world generation switch, rollback or teardown. Java8 regression tests
exercise the real-name fixture class ancestry and these guards.

The original C++ `lag_range_attack_hook` observes phase1 by sampling
`ticksExisted == lastAttackerTime`. R360's tick-derived candidate is
deliberately **not** claimed as exact original phase timing or proof
that an attack landed. The recovered event15 hook is a distinct original
callback; this is only an explicitly opt-in read-only inference.

Production wiring into the standalone original-client JNI hook will
be separately verified. No packet retention, input writes or gameplay
automation are enabled. PR #2 stays draft and unmerged.
