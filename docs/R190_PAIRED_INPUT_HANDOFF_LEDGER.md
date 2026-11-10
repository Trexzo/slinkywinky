# R190 — paired keyboard/mouse handoff ledger (strictly offline)

**Target:** `Trexzo/slinkywinky`, `reconstruction/r1-core-fidelity`, draft/unmerged PR #1. PR #2 (separate OpenSlinky C++ source-reference work) is not edited.

## Purpose and boundaries

R108 already contains an isolated first-party keyboard routing *model*, and R159/R184–R189 added a source-pinned retained mouse gesture/hover *model* with increasingly strict WGL and Win32 focus gates. They are not wired to original Minecraft event dispatch. R171's original 1.8.9 callsite audit found 146 direct input callsites; restoring only one method is insufficient proof of exclusive ownership.

R190 adds a **portable, header-only, standalone test-fixture ledger** which models a common lifecycle for keyboard and five mouse buttons. It is not an LWJGL2 hook, Windows hook, native injection surface, shared dispatcher, actual source pin, event suppression, or permission to send a click. Its two permanent methods `may_deliver_live_minecraft_event()` and `may_suppress_vanilla_event()` both return **false**.

## Ledger rules

The ledger is thread-owner-bound and rejects all foreign-thread mutations. It rejects zero, repeated or decreasing epochs. A transition into `Candidate` requires a separate fixture's exact-source-window, foreground, menu-visible and **all-controls-released** checks. Candidate events are quarantined. A further `confirm_fixture` step requires an exact matching epoch and independent **synthetic** assertions of original-dispatch suspension, separate GUI dispatch and all-controls-released. These booleans are deliberately *not* real Minecraft ownership evidence: the result is only the `MenuFixture` state.

The ledger's *synthetic test-only* event route distinguishes `GameUnchanged` (idle model with no menu), `MenuFixtureOnly` (accepted isolated menu simulation), and `Quarantined` (all ambiguous/unsafe states). Keyboard key codes 1–255 and five mouse buttons 0–4 share tracked held-state. Any unknown code/device, lost focus, changed window identity, missing exclusive fixture evidence or hidden menu while active permanently **revokes** the fixture session.

Closing a menu enters `Draining`. Releases remain quarantined and reduce held-state; new Down events or unknown releases while draining revoke the session. Even when every tracked held key/button is released, draining does **not** automatically return ownership to gameplay. `restore` requires independent host attestations that the menu is closed, **all controls are released** and **the original input dispatcher has been restored**. Revoked sessions also require this receipt. Restoration never calls an original game handler; it merely resets fixture state. A new session requires a higher epoch.

## Fixture acceptance

`tests/test_menu_input_handoff_ledger.cpp` covers paired key + 5-button mouse chords, foreign-thread denial, candidate/no-owner quarantine, two-stage fixture confirmation, repeated/stale epoch rejection, draining all releases, independent restore predicates, focus loss while held, incomplete dispatcher attestations, unknown keyboard/mouse codes, unknown device enums, source dispatcher failure, new Down/invalid release while draining and positive recovery to a new epoch. It explicitly checks that **no test state can authorize live Minecraft delivery or original event suppression**.

The portable C++17 executable is registered as `r190_paired_input_handoff_recovery_fixture` in CMake/CTest. It is isolated from all current original Minecraft JNI/Java and live input pipelines. The complete Windows x64 Release build and **93/93 CTest tests passed**, including the new fixture, native WGL/JNI and agent-attach tests. A separate original vanilla Minecraft 1.8.9 read-only run exited successfully with **six** restoration/admission stages, **64** retained frames, **60** source-visible slider draws, and **63** passive observations. The original-JVM exact OS-focus receipt was `foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0`, and native click delivery was disabled. The original-client foreground-hover manual gate remains unverified.

## Unfinished / required external proof

**Do not interpret a passing R190 test as an actual input handoff.** Before live ClickGUI interaction, the project still needs an exact-source Minecraft 1.8.9 mouse+keyboard dispatcher integration, full observed original consumer-graph coverage and a reversible ability to prevent original input handlers from receiving the same event. Held-input/focus-loss/GUI-close and original bytecode-restoration tests must run against the actual game with operator supervision. The existing original slider foreground focus-cycle gate has only ever returned **background-only** measurements in the noninteractive runs. No full Slinky 1:1 GUI or module parity is established by R190.

Keep live click delivery disabled and PR #1 draft.
