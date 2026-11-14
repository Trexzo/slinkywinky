# R202 — Keep Sprint three-phase offline reconstruction

Date: 2026-11-14. Separate branch:
`integration/openslinky-fd9c592-reference`, draft PR #2.
PR #1's active Minecraft loader/runtime and GUI remain untouched.

## Reference provenance

Two user-provided candidate C++ archives and the previously saved Java
OpenSlinky candidate were inspected as ZIP entries, **without executing
DLLs, JARs, launchers, injectors or game code**:

| Candidate | SHA-256 source file | Size |
|---|---|---|
| `c++_possibly_OpenSlinky-main.zip` / `src/logic/move/keep_sprint.cpp` | `696f781d38eba6772b24276b9c04205cb28a9f17a1686b4cc1ab15786008ee09` | 3,995 B |
| `slinky-main (1)(1).zip` / `src/logic/move/keep_sprint.cpp` | `696f781d38eba6772b24276b9c04205cb28a9f17a1686b4cc1ab15786008ee09` | 3,995 B |
| `OpenSlinky-main(2).zip` / `KeepSprintModule.java` | `c2d2993d8a14764d4c1b36adaed859f91e803645bd8ac183525f51c76197f30a` | 3,821 B |

The two native archive files are byte-identical. The Java source is not
a byte-level match, but independently corroborates the three-phase
state model. No candidate is authenticated as the original commercial
Slinky binary.

The C++ descriptor in `src/data/data.s` registers Keep Sprint in
**movement category 1** with three native callback positions:
- event **14**, priority **0**: before-attack sample
- event **15**, priority **0**: post-attack motion restore
- event **17**, priority **0**: next player update/sprint reapply

The Java translation listens to those same event indices, but declares
`ModuleCategory.COMBAT`. This is a **cross-reference discrepancy**,
not proof that the original native category was combat. The integration
uses the source-pinned native category 1.

## Independently authored first-party implementation

New header:
`reference_compat/include/slinkywinky/keep_sprint_controller.hpp`.

The typed, deterministic controller models:

1. On a phase-active attack: if in-game, player exists, sprinting and
   external chance roll succeeds, save X/Y/Z motion and arm; on a
   failed qualified roll disarm. Inactive phase preserves state.
2. On qualifying post-attack: require armed, active phase, player
   present, optional on-ground condition, matched hit tick, and
   not a target that forbids the allow-reduce option.
   Multiply saved **X and Z only** by `motion_multiplier`;
   preserve Y and propose the saved vector; mark sprint reapply pending.
3. On player update: consume pending state once; propose enabling
   sprint only if the player exists. The pending flag is cleared
   even when no player exists, following the candidate sources.
4. Explicit world reset and shutdown clear saved state, armed
   state, and pending reapply.

It intentionally does **not** reproduce the candidate RNG algorithm,
the actual Minecraft player/motion setters, or live attack hooks.
The RNG outcome is an explicit deterministic input. The controller
also fails closed on nonfinite motion/multiplier and negative
multiplier; that is a **defensive divergence** from the C++ source,
not claimed exact commercial parity.

The `ModuleExecutionBridge` now registers Keep Sprint's three
callbacks as one all-or-nothing metadata-validated binding. It also
admits the actual overlapping event payload combinations:
- event 14 with No Hit Delay and/or Item Use Fix;
- event 15 with Item Use Fix;
- event 17 with No Jump Delay.

Other mismatched typed payload combinations remain rejected.
No Minecraft state is mutated by this offline bridge.

## Tests and coverage

New `reference_compat/tests/test_keep_sprint_bridge.cpp` exercises
success/failure and inactive phase, grounded/tick/target checks,
X/Z-only scaling, repeated post multiplication, one-shot sprint
reapply, missing player, reset, nonfinite values, event co-delivery
and five invalid native category/priority/subscription shapes.

The catalog audit tests full metadata-backed registration and replay
through three event callbacks, including post-reset and disabled
behavior. The synthetic 18-module manifest and four additional
negative files enforce correct native source metadata.

New cross-platform CTests:
- `r202_keep_sprint_three_phase_offline`
- `r202_fifteen_decision_coverage`
- `r202_reject_keep_sprint_wrong_category`
- `r202_reject_keep_sprint_wrong_attack_priority`
- `r202_reject_keep_sprint_missing_post`
- `r202_reject_keep_sprint_wrong_update_priority`

When final CI passes, the first-party **offline** coverage inventory
is expected to be: 66 registered module identities, 171 candidate
subscriptions, **15 module policies**, **20 bound callbacks** and
**151 unimplemented native callback subscriptions**.

These numbers do not represent game-integrated module completion or
prove equivalence with the commercial original. The user PC's remote
agent was offline, so exact native compiled-manager acceptance and
Minecraft 1.8.9 acceptance have **not** been rerun on this branch.

## Outstanding fidelity gates

- Validate the original binary's callback semantics and category with
  independently authenticated evidence.
- Test original JVM/Win32 game-thread event ownership and lifecycle.
- Independently reconstruct probability/RNG seeds and timing.
- Verify actual Minecraft attack event order, motion setter and
  sprint reapplication behavior in an authorized isolated client.
- Verify settings and GUI fidelity (chance, motion multiplier,
  on-ground and allow-reduce).
- Test unload, world changes, failure recovery and cross-module
  interactions before any use outside controlled tests.

Keep PR #2 draft/unmerged; PR #1 remains independently developed.
