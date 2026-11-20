# R254 — three more recovered first-party module decision models

Date 2026-11-20. Chat 1 draft PR #1, Chat 2 PR #2 untouched.

## Actual original source consulted

Independent C++17 implementations in PR1 correspond to the
**original public OpenSlinky** commit
`fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4`:

- `src/logic/utility/no_hit_delay.cpp` and `.h`: attack-click
  event 14, utility category 4, priority 0. Guard JVM readiness
  and positive left-click wait. Mode 0 proposes clearing delay;
  selective mode 1 instead proposes a swing on miss when not
  using an item.
- `src/logic/move/safe_walk.cpp` and `.h`: player tick event 0,
  move category 1, priority 0. Pitch >=65, no-forward, and
  hold-block requirements condition edge guard; grounded
  X/Z motion multiplier leaves Y unchanged. No game
  collision guard or Minecraft motion field is written.
- `src/logic/block/fast_place.cpp` and `.h`: player tick event
  0, block category 3, priority -1000. Source-order key
  press timer, initial physical use press, activation delay,
  block/ladder/collision/random branch. The model caps maximum
  CPS at 100 and bounded proposed press slots at six per tick,
  explicitly correcting dangerous unbounded source behavior.

The PR1 source implementation is separate from PR2 (no copied
PR2 header, no PR2 files modified). New header
`include/slinkywinky/recovered_source_module_policies.hpp`
defines isolated typed, **value-only** policies and an
`RecoveredSourcePolicyOwner` with thread ownership, native
registration metadata, generation/world admission, selected vs
active state, snapshot publication and reset/unload isolation.

## Evidence

- Always-on PR1-only test
  `r254_recovered_utility_movement_block_decision_owner`:
  catalog names/indices and native registration, No Hit Delay
  and Safe Walk branch cases, Fast Place high-CPS bounded
  proposals, three native ArrayList rows, world exit and
  re-entry reset, wrong event and foreign-thread refusal,
  retirement.
- The opt-in source-pinned PR2 workflow compiles actual
  `NoHitDelayController`, `SafeWalkController` and
  `FastPlaceController` from exact PR2 commit
  `7f457376058ad42e0a0e0323ed874d7e5b2ae03a`. New `r254_real_pr2_three_module_policy_parity`
  runs **1,536 No Hit Delay cases, 7,680 Safe Walk combinations,
  4,320 stateful Fast Place source-order ticks**, plus
  invalid-settings negatives. It checks behavior rather than
  merely compiling the headers.
- The three original Slinky sources and PR2 controller sources
  are both source references. The PR1 implementation is an
  offline simulation with intentional bounds, not a recovered
  binary/JVM hook or reverse-engineered source-tree equivalent.

**Important:** R254 adds three *offline decision policies*
in PR1, not three additional controllable live Minecraft
modules. The original R245 GUI-to-native request path remains
strictly limited to No Jump Delay, No Slow, Sprint; R254
decisions do not cross a Java/JNI gameplay callback.
Authoritative policy ArrayList rows come only from the
separate source-owner state, never the GUI preview. This
work increases recovery coverage, but not the 3-module
live control count. The user goal of 66 modules and
171 callbacks remains unfinished.

## CI results

Add exact commit evidence after workflow completion.
