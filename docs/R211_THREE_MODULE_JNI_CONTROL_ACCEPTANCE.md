# R211 — Three-module native control and typed Java 8/JNI integration

Date: 2026-11-14. Repository: Trexzo/slinkywinky.
Branch: integration/openslinky-fd9c592-reference (draft PR #2).
Parallel PR #1 owns GUI, input, ArrayList, overlay and injection.
**No PR #1 implementation files were edited by this track.**

## Outcome

R210 introduced a real native owner-thread Sprint toggle/settings/state
boundary. R211 extends the **same single native session**, rather than
creating a second disconnected module manager, to these three canonical
modules:

| Canonical name | Candidate category / native slots | Runtime selection | Explicit settings |
|---|---|---|---|
| No Jump Delay | Movement / event 17 priority 100 | Supported | None identified in tested policy |
| No Slow | Movement / event 21 priority 200 | Supported | Bow, sword, consumable multipliers |
| Sprint | Movement / events 17 and 21, both priority 100 | Supported (backward compatible) | In GUI, using item, backwards, sideways |

These policy/event mappings predate R211 and come from the source-grounded
candidate reconstruction. R211 does **not** add new original native
callback implementations: it turns already implemented offline policies
into independently controllable, admitted, stateful event subscriptions.

All three modules have their own requested and effective in-world enabled
state. Requested selections and No Slow/Sprint settings survive world
exit/re-entry; actual callbacks are removed when the world disappears.
Disabling a module stops its decisions without disabling its peers.

## Public control surfaces for PR #1

Shared native C++ contract (unchanged version 1):
\`include/slinkywinky/module_control_contract.hpp\`

The first-party native \`SprintObservationPreview\` keeps its Sprint
legacy APIs and now adds:

- \`request_module({"No Jump Delay", enabled})\`
- \`request_module({"No Slow", enabled})\`
- \`request_module({"Sprint", enabled})\`
- \`configure_no_slow({bow, sword, consumable})\`
- \`configure_sprint(SprintSettings)\`
- \`control_state("canonical name")\`
- \`control_states()\`: owning, ordered copies for
  No Jump Delay, No Slow, Sprint.

Java 8 API (class name retained for binary/source compatibility with
the existing Sprint preview):

\`dev.slinkywinky.SprintObservationProbe\`

- \`setModuleEnabled(String canonicalName, boolean enabled)\`
- \`moduleState(String canonicalName)\`
- \`moduleStates()\` returning acknowledged named copies
- \`controlledModuleNames()\` returning a clone of the canonical names
- \`setNoSlowSettings(float bow, float sword, float consumable)\`
- \`setSettings(boolean inGui, boolean usingItem,
   boolean backwards, boolean sideways)\` (legacy Sprint API)
- \`observeModules(clocks, flags, coords, angles, movement,
   moduleContext, passiveNoSlowSettings)\`

\`observeModules\` uses ONE real Java8 -> JNI -> native
\`ModuleExecutionBridge\` dispatch with both source-grounded events:
event17 can hold Sprint and No Jump Delay; event21 can hold No Slow
(priority 200) and Sprint (priority 100). Both callbacks see typed
data for the same observed player. Inconsistent input is rejected
before any native world/session generation is changed.

The new \`moduleContext\` int array is:
\`[inGame, observedJumpTicks, usingItem, heldItemPresent, itemId]\`.
The new \`passiveNoSlowSettings\` float array is:
\`[bow, sword, consumable]\`.
Native configured No Slow settings override these passive values.
\`inGame\` must match observed player/world flags; \`usingItem\`
must match the existing Sprint observation flag.

R211's native result array is 15 floats: the **original 9 Sprint
decision floats unchanged** plus:
\`[hasJumpTicks, jumpTicks, noSlowEvaluated, replaceInput,
   proposedStrafe, proposedForward]\`.
The Java \`ModuleDecision\` validates this layout and gives the UI
a typed result; legacy \`observe()\` still returns the exact
prior 9-float Sprint-only result. The *extended* method is required
to obtain real typed No Slow / No Jump Delay decisions.

### ArrayList and ClickGUI ownership

PR #1 must run native module-control requests on the admitted owner
game thread, not a WGL frame/render hook or background thread.

An actual ClickGUI click only requests state. After the native
acknowledgement, copy the \`ModuleControlState\` /
Java \`ControlState\` into an immutable HUD snapshot. Its
\`requested_enabled\` flag is the user's choice, while
\`active_in_world\` reports effective, world-admitted subscriptions.

The source-backed \`MenuItem.preview_enabled\` field on PR #1 is
**not** evidence of a real enabled module and must not be used
as ArrayList state.

The tested UI-facing canonical order is
\`No Jump Delay\`, \`No Slow\`, \`Sprint\`. Java callers cannot
mutate this catalogue by editing the returned array.

## Windows source and JNI acceptance

Tested revision:
\`70465cf2c6a79125f4e3603a2295867628fbeb06\`

User-authorized connected Windows machine, Temurin Java 8 and
native MSVC Windows DLL build. All builds remained isolated from
the user's normal game installations.

| Suite | Debug | Release |
|---|---|---|
| Full first-party C++ reference CTests | 91/91 PASS | 91/91 PASS |
| New R211 3-module event/state integration CTest | PASS | PASS |
| Existing R210 Sprint module-control compatibility CTest | PASS | PASS |
| Actual Java 8 -> JNI -> C++ R211 3-module smoke | PASS | PASS |
| Pinned 66-entry compiled-native-manager regression CTests | 7/7 PASS | 7/7 PASS |

New test file:
\`reference_compat/tests/test_three_module_control.cpp\`
New CTest name:
\`r211_three_module_control_integration\`
Expanded Java smoke:
\`tests/java/dev/slinkywinky/SprintObservationProbeSmoke.java\`

Specifically verified:
- all three canonical names and independent requested/effective states;
- enabling No Jump Delay returns a typed jump-tick-zero proposal;
- enabling No Slow with bow multiplier 0.8 changes the already slowed
  0.1 / -0.2 input into a 0.4 / -0.8 proposal;
- changing bow multiplier to 0.5 changes the same JNI decision
  into 0.25 / -0.5;
- toggling No Slow off leaves No Jump Delay and Sprint independent;
- toggling Sprint off still allows the other two module callbacks;
- malformed item-use, contradictory jump/world fields, illegal
  module names and nonfinite multipliers rejected;
- unknown/non-owner session access rejected;
- absent world removes active subscriptions while retaining
  requested selections and explicit settings;
- re-entered world restores exactly requested modules;
- legacy 9-float Sprint JNI smoke and R210 tests still pass.

There was **no Minecraft launch during R211**, and no new
key presses, motion writes, packet changes, native injection
or anti-cheat bypass. These remain **data-only decisions**.

## What is not done

The native module state and Java/JNI adapter now support three
modules, but PR #1's ClickGUI and ArrayList have not yet been
wired to them. No Jump Delay and No Slow do not yet apply game
actions in Minecraft. Source-faithful settings constraints
and original event-17/21 timing still require authenticated
reference and game-thread verification.

This is not completion of three commercial gameplay modules;
it is a tested control and offline decision path for three
previously modeled modules.

Coverage count remains:
66 catalogued native module identities;
171 candidate callback subscriptions;
16 first-party offline policy models;
22 offline callback bindings;
149 unimplemented candidate subscriptions.

CI for the final R211 head must be checked separately.
PR #2 remains draft/unmerged, PR #1 untouched.
