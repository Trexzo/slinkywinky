# R362c — Original vanilla clickMouse return -> actual native Lag Range e15

The original Mojang 1.8.9 client method candidate from the private
recovered `ptr_to_str_al` table is `ave.aw()V` (`Minecraft.clickMouse`).
The exact original-class `ave` SHA-256 admission already exists from
R358. R361 adds the independent original-JAR method verifier.
R362b's production ASM5 `Vanilla189ClickExitWeaver` injects only one
read-only callback before every **normal `RETURN`** of original
`ave.aw()V`; it does not intercept throws, cancel a real click or
replace any Minecraft method body.

The existing separate opt-in CLI
`--source-tick-readonly[=600]` now enables this source callback as
part of its finite original-only mode. Its source tick and click
method must both exist exactly once in the fingerprinted original
`ave` class. Synthetic and unverified clients are rejected.
Normal Forge, private-source test fixtures and legacy prelaunch
JVM-property R360 diagnostics never receive this new click observer.

On game tick, the original-source `ObservedState` takes an independent
primitive-only `pk.W:I` and `pr.bl:I` combat-clock snapshot.
`SourceClickAttackFence.prime` binds it to the real world/player
generation. On an `ave.aw()V` normal exit, the injected read-only
callback re-reads only those two fields, refusing a world/player
replacement, different JVM thread, old timestamp or duplicate tick.
Only a **newly matched** `playerTicks == lastAttackerTime` candidate
sends the existing native Lag Range `event15`, which is subsequently
consumed by the next `event18` source tick. This replaces
R360's weaker *tick-only* event15 inference in the CLI source mode.
Both models can never dispatch native e15 in the same source session.

Failures are logged without propagating into Minecraft's click call,
and abort the bounded native diagnostic at the next game tick.
Both source and click JNI state are retired on the client owner thread
before the original class is restored. Only aggregate counts are
logged; no event target, private coordinates or game inputs are written.

**Limitations:** A vanilla clickMouse NORMAL RETURN is not proven
identical to the recovered original Slinky PhaseHook phase 1.
The click may attack air or blocks, so a fresh equality timestamp
is additionally required. Real on-wire packet event3 capture/hold
is NOT enabled. End-to-end original-game behavior remains unverified
without an authorized manual runtime acceptance test.
PR #2 remains draft/unmerged; PR #1 unchanged.
