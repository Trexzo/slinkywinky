# R210 — First real GUI-to-module control boundary: Windows acceptance

Date: 2026-11-14  
Repository: Trexzo/slinkywinky  
Owner branch: `integration/openslinky-fd9c592-reference` (draft PR #2)  
Parent GUI/runtime branch: `reconstruction/r1-core-fidelity` (draft PR #1; code untouched)

## What was added

The candidate Sprint module previously ran by default whenever the
read-only native observation preview entered a world. A ClickGUI
toggle could not change its native callback state. The menu's
`preview_enabled` value was just a visual demonstration, not a
gameplay control.

R210 introduces a versioned shared contract in
`include/slinkywinky/module_control_contract.hpp`:

- `ModuleControlRequest { canonical_name, enable }`
- `ModuleControlState { canonical_name, requested_enabled,
  active_in_world, settings_override, world_generation }`

The acknowledged state uses an owning `std::string` canonical name
so copies do not dangle when published to another thread's HUD.

The first implemented owner-thread API is in
`reference_compat/include/slinkywinky/sprint_observation_preview.hpp`:

- `request_module({"Sprint", enabled})`
- `configure_sprint(SprintSettings)`
- `control_state()`

Changes persist the requested selection and explicit settings across
absent-player/world generations. Disabling the module in an admitted
world immediately removes its native event callbacks; no Sprint
proposals are emitted until it is enabled again. Settings changes
become authoritative over earlier passive observation flags.
The callback's volatile state is reset on disabling and world loss.

The real Java 8 wrapper exposes:
- `SprintObservationProbe.setEnabled(boolean)`
- `SprintObservationProbe.setSettings(inGui, usingItem, backwards, sideways)`
- `SprintObservationProbe.state()`

Real JNI exports were added under
`reference_compat/src/sprint_observation_jni.cpp`. The native module
handle registry validates session IDs and module requests, and the
Java and native layers both enforce owner-thread access. These APIs
return acknowledged native module state, not a GUI preview flag.

There are **no new Minecraft key, player motion, packet, injection
or game-state write actions**. This is a stateful, live JNI
control/decision boundary; the full in-game effect adapter is
still unimplemented.

## Local Windows acceptance

Tests ran on the user's explicitly authorized Windows PC
DESKTOP-LTXXO4K, building from the isolated PR #2 checkout.
The relevant fully tested source revision was
`58ae4c6fa93f1c8a1cadf7c5523acf380c0d0fd3`.

- **90/90** first-party reference CTests passed in **Debug**.
- **90/90** first-party reference CTests passed in **Release**.
- The new `r210_native_module_control_integration` passed in
  both configurations, covering disabled callback suppression,
  effective-state acknowledgement, settings override,
  world exit/re-entry, invalid canonical names, shutdown and
  cross-thread rejection.
- The real Java 8 JNI DLL was compiled in **Debug and Release**
  in a dedicated build outside Windows TEMP
  (`%LOCALAPPDATA%\SlinkyWinky\R210-JNI-Integration-Build`).
- The updated Java 8 `SprintObservationProbeSmoke` passed
  in both configurations against this actual JNI DLL, reporting
  `R210_JAVA8_MODULE_CONTROL_INTEGRATION_PASS`.
- Separate pinned 66-entry native-manager regression CTests:
  **7/7 Debug** and **7/7 Release**, with the reference
  native object and private fixture remaining local.

The GUI/runtime PR #1 checkout and original user Minecraft,
mod directories and SpawnPK processes were not touched.
No live Minecraft process was launched for R210.

## Cross-chat coordination

The detailed PR #1 handoff is
`docs/R210_MODULE_CONTROL_INTEGRATION_CONTRACT.md`.
A GitHub conversation comment on PR #1 links the interface
without editing the parallel chat's files.

PR #1 should:
1. route actual in-game ClickGUI toggles from admitted
   foreground input to the owner's game-thread module command;
2. display acknowledged `requested_enabled` /
   `active_in_world` state, not local `MenuItem.preview_enabled`;
3. render ArrayList/HUD from copies of acknowledged native state;
4. call the four typed Sprint settings controls;
5. preserve selected state through world absence while clearing
   effective game subscriptions and releasing all owned actions.

PR #2 owns the underlying native module binding, settings,
callback behavior and later game-action cleanup. Both chats
should coordinate on the versioned contract before modifying it.

## Important uncompleted work

R210 does **not** prove a working graphical toggle inside
the injected client, an active ArrayList HUD, or a game-action
implementation of Sprint. It does not make the module equivalent
to the original commercial Slinky module. Its purpose is
closing the previously missing state/command/acknowledgement
interface to enable those integrations.

Coverage is deliberately unchanged:
66 mapped native module descriptors / 171 candidate callback
subscriptions / 16 first-party offline module policies /
22 callback bindings / 149 unimplemented subscriptions.

PR #2 remains draft and unmerged. Final-head GitHub Actions
CI must be checked separately.
