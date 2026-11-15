# R204 — Read-only Sprint observation JNI bridge and Forge 1.8.9 opt-in

Date: 2026-11-14. Repo: Trexzo/slinkywinky.
Branch: integration/openslinky-fd9c592-reference (draft PR #2).
PR #1 reconstruction/r1-core-fidelity was not edited or rebased.

## Goal and ownership

R203 reconstructed offline Sprint decisions and verified exact public native
candidate metadata: movement category 1, event 17/priority 100 and
event 21/priority 100. R204 adds a **read-only observation boundary**
that can receive the existing first-party runtime observation wire shape,
then a **real Java 8 -> JNI -> native C++** round trip and finally an
optional read-only Forge 1.8.9 source integration.

These changes use only independently authored first-party code. They do
not execute the public OpenSlinky gameplay callbacks, import original
commercial code, inject a native DLL into Minecraft, call Minecraft
movement setters, press keys, or modify packets.

## Native observation adapter

New:
  reference_compat/include/slinkywinky/sprint_observation_preview.hpp

The adapter accepts the existing first-party
slinkywinky::ObservationFrame from include/slinkywinky/observation.hpp,
not a newly invented incompatible frame type. It combines that frame
with explicit read-only Sprint GUI/player/movement observations.

It uses the production first-party ObservationLedger from
src/observation.cpp for strict owner-thread, nonzero generation,
monotonic clock, finite position/angles and world/player admission.
It then drives the same R203 ModuleExecutionBridge callback pair:

- event 17: calculate Sprint key/forced-sprinting decisions;
- event 21: calculate permitted movement speed proposals.

A world or player token transition requires a new generation.
The adapter retires the previous offline module state when the
generation changes, discards decisions on no-player frames, and
starts a newly configured state on the next admitted session.
Wrong-thread, invalid world/player relation, stale time and stale
session observations are rejected before any native state mutation.

The result contains *only* immutable proposals and admission state.
It never executes proposals against a game object.

## Genuine Java 8/JNI boundary

New:
  reference_compat/src/sprint_observation_jni.cpp
  java/dev/slinkywinky/SprintObservationProbe.java

The JNI export functions are compiled directly into the existing,
optional slinkywinky_jni.dll when SLINKYWINKY_BUILD_JNI=ON.
They accept fixed-size Java arrays containing clocks, flags,
coordinates, angles and movement samples. The native side validates
all dimensions, booleans, clocks and session handles.

Native preview instances are registered under opaque, incrementing
integer IDs in a mutex-protected map. Invalid or closed IDs are
rejected; JNI instance ownership is checked by the native preview
and the Java wrapper. The ID counter now guards signed overflow.
Java's close() retires the native object on its owner thread.

The nine returned float values are bounded proposals:
admission, world/session status, key state proposal, optional
player sprint-state proposal, optional speed proposal and
sprint-reapply proposal. No JNI member accesses a Minecraft class
or invokes a game action.

Cross-language test:
  tests/java/dev/slinkywinky/SprintObservationProbeSmoke.java
  tests/run_r204_sprint_observation_jni.cmake

This test compiles Java classes with JDK8 and executes the actual
native JNI roundtrip against a separately built C++ DLL. It verifies
GUI forced-state behavior, backward speed factor 1.3, session loss
and return, stale time rejection, malformed Java array dimensions,
native invalid boolean flags, wrong-thread refusal and recovery.

## Opt-in Forge 1.8.9 observer (source-only compile)

Updated:
  experimental/forge189/src/main/java/dev/slinkywinky/forge/ForgePreview.java

If started with the JVM property:

    -Dslinkywinky.sprintObservation=true

the existing read-only preview's F9 toggle creates a
SprintObservationProbe alongside its normal GameHost. Forge render
events supply observed world/player identity, GUI presence, sprint
state, item-use state and player movement-input fields. The opt-in
preview appends *proposed* Sprint key/speed values to its overlay.
It does **not** apply key bindings, movement, sprint-state changes
or gameplay actions. F9 disable and failed reads close native
session state.

Additional diagnostic-only options:
  -Dslinkywinky.sprint.allowGui=true
  -Dslinkywinky.sprint.allowUsingItem=true
  -Dslinkywinky.sprint.allowBackwards=true
  -Dslinkywinky.sprint.allowSideways=true

These are explicit test switches, **not** the authentic recovered
original GUI settings or evidence of their default values.

**Important fidelity limitation:** Forge RenderTick.END is not the
same native event timing as the original candidate callbacks (player
update 17, movement input 21). The preview currently passes
mc.thePlayer.capabilities.getWalkSpeed() as a *diagnostic proxy*
for the original event-21 speed field. The values demonstrate a
working source-level observation-to-decision pipeline but **must
not** be treated as calibrated original gameplay speed semantics.
The game-thread model and event ordering need independent acceptance.

## Local Windows acceptance

Tests performed on the explicitly connected authorized
DESKTOP-LTXXO4K machine; all artifacts are kept in Windows Temp.

**First-party native observer**

- r204_sprint_native_observation_preview: PASS Debug and Release.
- Complete reference_compat source-only CTests:
  **89/89 PASS Debug** and **89/89 PASS Release**.
- Cases include world/player generation transitions, clock rollback,
  missing world, wrong-thread refusal, nonfinite observations and
  state reset across a new session.

**JNI boundary (real Windows DLL + actual JDK8)**

- Compiled slinkywinky_jni.dll in Debug and Release, including
  sprint_observation_jni.cpp.
- Actual JDK8 javac and java.exe completed
  r204_java8_native_sprint_observation_roundtrip:
  **PASS Debug** and **PASS Release**.
- The original NativeEngine JNI bridge remains built as part
  of the same DLL; no private binary was loaded in another process.

**Independent pinned native OpenSlinky manager regression**

- The previously compiled and SHA-256-verified 66-entry module-manager
  COFF, hash
  632d706ec121d4809a744876f4006c0e4ece19d7af0ad4994def704a77e516ae,
  was retested with the isolated first-party manager harness:
  **7/7 PASS Debug, 7/7 PASS Release**.
- These compiled manager tests do not execute the new Sprint preview's
  Java code or original Sprint native callback; they are separate
  regression evidence.

**Forge 1.8.9 Java compilation**

The isolated Forge preview checkout currently has no gradlew.bat and
the authorized PC has no gradle command on PATH; therefore a full
ForgeGradle package/build and actual Minecraft launch were **not**
completed.

Instead, the updated ForgePreview.java and its Java support classes
were **successfully compiled** with an actual JDK8 javac against
the cached mapped Forge 1.8.9
11.15.1.2318-1.8.9 stable_22 forgeBin.jar, LWJGL 2.9.4 and Guava 17.
The direct javac run returned exit 0 with a deprecation note.
This validates Java source compatibility against the target API,
but not build packaging, JNI DLL availability inside Forge, live
RenderTick owner-thread behavior or multiplayer correctness.

## Outstanding work and strict coverage accounting

- Full ForgeGradle build/package and controlled Minecraft 1.8.9
  runtime probe (singleplayer/offline) must be verified separately.
- Connect true player-update event 17 and movement-input event 21
  rather than relying on RenderTick sampling and a speed proxy.
- Replace diagnostic JVM switches with validated recovered GUI
  settings and actual session-owned settings persistence.
- Ensure safe JNI native handle lifecycle during unload/reinject
  and no stuck physical/synthetic key state.
- Compare against an authenticated original Slinky release before
  claiming any one-to-one GUI or gameplay parity.
- No real client DLL injection, player input write, packet send or
  Java game setter was authorized or performed in R204.

Offline policy coverage is **unchanged** from R203:
66 candidate module identities, 171 candidate subscriptions,
16 first-party offline module policies, 22 bound offline callback
decisions and 149 unimplemented candidate subscriptions.
R204 added a Java/observation integration path, **not** a new
completed gameplay module.

Keep PR #2 draft/unmerged; PR #1 remains untouched.
The final-head GitHub Actions status must be checked separately.
