# R210 — Shared module-control handoff: PR #1 GUI ↔ PR #2 native modules

**Repo:** Trexzo/slinkywinky  
**Owner:** PR #2, \`integration/openslinky-fd9c592-reference\`  
**Status:** interface implemented and tested; not yet connected to PR #1 ClickGUI.

## Why this exists

PR #1's \`MenuState::toggle_preview\` and \`MenuItem::preview_enabled\`
are deliberately *visual preview flags*, not active native modules.
PR #2's Sprint observation bridge previously enabled Sprint
unconditionally whenever a world appeared, so even a future
Clickable GUI would not be able to control it. This was a real
integration blocker, independent of further Minecraft test launches.

**R210 closes that particular gap** without editing PR #1's existing
menu, HUD, injector, renderer or JNI files.

## Versioned contract (new public first-party header)

\`include/slinkywinky/module_control_contract.hpp\` contains:

- \`kModuleControlContractVersion = 1\`
- \`ModuleControlRequest { canonical_name, enable }\`
- \`ModuleControlState { canonical_name, requested_enabled,
  active_in_world, settings_override, world_generation }\`

Canonical module names are exactly those used by the native module
catalogue. The first implemented control name is **\`Sprint\`**.
Unknown names are rejected, not silently accepted.

### State semantics for the other chat

- **Requested**: the user's selected module state, retained after
  disconnect and before a player/world is available.
- **Active in world**: the native module is currently subscribed and
  the associated event policy can be dispatched. This is false
  before entering a world and after losing the world.
- **Settings override**: the module has received explicit current
  control settings. This is distinct from fixture/demo defaults.
- **World generation**: source-side session identity. Game events
  must be admitted by the owner thread, not from the OpenGL hook.

An ArrayList entry representing *active* modules must derive from
the native \`active_in_world\` value, not from
\`MenuItem::preview_enabled\`; a GUI-selected but temporarily
inactive module may be displayed separately as "queued/armed".
The existing PR #1 HUD rendering is not modified in this PR.

## Currently implemented first module: Sprint

PR #2 added owner-thread API to \`SprintObservationPreview\`:

- \`request_module(ModuleControlRequest{"Sprint", enabled})\`
- \`configure_sprint(SprintSettings)\`
- \`control_state()\`

The Java 8 wrapper \`SprintObservationProbe\` now offers:

- \`setEnabled(boolean)\`
- \`setSettings(inGui, usingItem, backwards, sideways)\`
- \`state()\` returning \`ControlState\`

These go through **real JNI exports** in
\`reference_compat/src/sprint_observation_jni.cpp\`, not a second
mock state. All requests are accepted only from the original Java
preview owner's thread. Closed handles, invalid parameter values,
unsupported module names (native side), and foreign-thread calls
fail closed.

The prior default remains \`requested_enabled=true\` for legacy
R204–R209 observation tests, but a newly connected UI can now
disable Sprint before first world entry and expect **no callback
decisions** until enabled.

Explicit settings win over older read-only Java observation flags.
They survive world generation changes; native callback state does
not. Disable clears Sprint's in-session forced tracker.

**No JNI API in R210 presses keyboard keys, writes player motion,
changes network packets or invokes a Minecraft setter.** It is a
real stateful module command/dispatch boundary, not yet a
working cheat/gameplay action.

## Minimum PR #1 wiring (owned by the GUI chat)

1. On actual user click, generate \`ModuleControlRequest\` for the
   canonical module name from the native module catalogue.
2. Enqueue the request onto the admitted Minecraft client/owner
   thread; never call the native callback directly from WGL
   SwapBuffers, mouse hook, HUD renderer or unrelated Java threads.
3. Invoke the PR #2 module-control API only on that owner thread.
4. Read back **acknowledged** \`ModuleControlState\` and publish an
   immutable copy through a bounded render/HUD snapshot channel.
5. Render the enabled/disabled state and ArrayList from the
   acknowledged module state, not from the unconnected GUI preview.
6. Deliver the four typed Sprint settings to the native adapter
   through the existing control API when changed.
7. On world exit: revoke game effects and active state, retaining
   requested selection. On disconnect/unload: revoke stale
   event subscriptions. On full client shutdown: close the JNI
   preview and reject all subsequent requests.

R210 does **not** change PR #1 and the GUI chat should not modify
PR #2's \`sprint_observation_preview.hpp\` or JNI callback
implementation without coordinating an interface change.

## Independent acceptance matrix

New native test:
\`reference_compat/tests/test_module_control_integration.cpp\`
(\`r210_native_module_control_integration\`).

Updated Java8 JNI integration smoke:
\`tests/java/dev/slinkywinky/SprintObservationProbeSmoke.java\`.

The test chain is:
**owner request → native queue/apply → admitted world state →
typed Sprint callbacks → acknowledged state snapshot → world
transition and restored control**.

Scenarios explicitly tested:
- UI-equivalent disable before a world prevents policy decisions;
- enable in-world activates the actual Sprint event callbacks;
- explicit settings disable and restore backward movement
  speed proposals;
- disable in-world stops new proposals;
- selection + settings persist through absent world / next session;
- foreign-thread calls fail; session close rejects further work;
- old R204-R209 default read-only behavior remains.

Windows native CTest passed in both Debug and Release. The
Java8/JNI updated smoke is exercised with the real first-party
Windows JNI DLL, not a mock DLL. CI and wider regression test
outcomes should be attached separately once final-head tests finish.

## Next functional milestone (not claimed finished)

The next joint acceptance should prove **ClickGUI toggle →
owner-thread Java/native control → acknowledged ArrayList state**
in an injected, explicitly authorized Minecraft client.
After that, implement **game action ownership** and cleanup for
each module, beginning with one well-understood module. This
requires verifying actual Minecraft API semantics and reference
differences before calling gameplay modules functional.

**Module completion count is unchanged at 16 offline policies,
22 bound decisions, 149 native callback subscriptions not yet
implemented.** Control-plane work is not another recovered
gameplay callback. Full authentic commercial Slinky parity is
not established.
