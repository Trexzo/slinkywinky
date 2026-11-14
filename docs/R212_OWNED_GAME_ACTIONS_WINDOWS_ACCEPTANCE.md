# R212 — Owned local gameplay-action boundary for No Slow and No Jump Delay

Date: 2026-11-14. Repository: Trexzo/slinkywinky.
Module branch: integration/openslinky-fd9c592-reference, draft PR #2.
The GUI/injector/ArrayList owner remains PR #1; no PR #1 files edited.

## What changed

The native R211 module manager was already capable of independently
enabling No Jump Delay, No Slow and Sprint, and returning
source-grounded, typed data-only decisions through real Java8/JNI.
There was no validated **game-action owner** capable of applying,
scoping, releasing or revoking those decisions.

R212 adds a *real Minecraft field-action interface* for the
two simpler policies, while making the **normal** Forge preview
remain read-only.

### 1. Java8 owner and reversible write barrier

New: java/dev/slinkywinky/OwnedModuleActions.java

The first-party \`OwnedModuleActions\` class knows nothing about
Forge or Minecraft internals. It accepts a typed \`Input\` containing
native-policy output, the selected/acknowledged module activation,
an explicit \`allowActions\` permission and world generation.

The host supplies a \`Port\` with Minecraft world/player/movement
identity, read/write movement-input fields, and read/write jump
cooldown fields. The class **never** uses OS synthetic keys,
Java reflection, JNI memory writes, injected process access or
network packet operations itself.

Policy:
- Fail closed if no permission, player absent, invalid generation,
  nonfinite or excessive movement proposals, incorrect proposed
  jump cooldown, closed session or non-owner thread.
- For enabled No Slow, apply only an explicitly returned native
  \`replaceInput\` decision. Save the exact prewrite strafe/forward
  values and record the exact values actually written.
- On disable, subsequent eligible update, or close, restore a
  previously written movement field **only if** the game field
  still exactly matches the owner's last write. If physical input
  or Minecraft changed it, leave that new value alone.
- If a player, world, movement object or generation changes, discard
  stale ownership, and **never write into an old player object**.
  This is a conservative safety fence, not proof that the previous
  player object's movement was physically restored.
- No Jump Delay may set an admitted positive jump cooldown to
  zero; it is an immediate one-tick effect. Turning it off stops
  further overrides and lets normal vanilla cooldown progress.
  R212 does **not** attempt to restore an elapsed jump cooldown.
- Sprint still produces data-only decisions; synthetic sprint
  key ownership, physical-key arbitration and direct
  \`setSprinting\` writes remain unimplemented.

### 2. Real Forge 1.8.9 field port

New: experimental/forge189/src/main/java/dev/slinkywinky/forge/
ForgeModuleActionPort.java

The compiled Forge adapter implements the \`Port\` interface
against real \`EntityPlayerSP.movementInput.moveForward\`,
\`moveStrafe\`, and \`EntityLivingBase.jumpTicks\`.

The exact development/SRG 1.8.9 field mapping was **independently
verified from the local pinned ForgeGradle reobfuscation .srg**:

\`EntityLivingBase/jumpTicks\` → \`EntityLivingBase/field_70773_bE\`

Only these two known names are accepted. Reflection checks the
field is an int, and refuses unknown mappings, denied reflective
access and stale world/player/input identities. No field
enumeration, index guessing, unsafe memory write or private
binary information is used.

### 3. Opt-in Forge action trial wiring

Updated: experimental/forge189/src/main/java/dev/slinkywinky/forge/
ForgePreview.java

With default JVM properties the preview remains **read-only**.
The R212 action path is enabled only by the separate explicit
\`-Dslinkywinky.r212ActionTest=true\` property.

It refuses start-up unless all of these hold:
- read-only Sprint/JNI observation is also enabled;
- neither of the older R208/R209 auto-world tests is active;
- Minecraft gameDir is in Windows TEMP with a new named
  \`slinkywinky-r212-independent-forge-\` prefix;
- Minecraft session has the exact dummy offline username
  \`R212Offline\`.

After **manual F9 preview activation** inside a local Minecraft
singleplayer world (not multiplayer), the R212 test lane:
- instantiates the Java8 owner barrier;
- enables No Slow and No Jump Delay through the real R211
  owner-thread JNI module-control API;
- obtains typed No Slow / jump-cooldown decision proposals
  through \`SprintObservationProbe.observeModules(...)\`;
- obtains native **acknowledged** module activation state,
  not a GUI demo flag;
- passes those decisions and existing world identity through
  \`OwnedModuleActions\` and the real Forge field port;
- on F9 disable, native failure or player/world absence,
  revokes local action ownership and conditionally restores
  still-owned movement values.

The test path **does not** create a world automatically, inject
into a remote game, connect to a multiplayer server or press
synthetic keys. The session is test-only and must be explicitly
started by a local operator.

**Timing warning:** currently invoked at Forge client
\`PlayerTickEvent.END\`; this is **not** proof that No Slow
takes effect before vanilla consumes movement input or matches
the original Slinky's native event-21 timing. The action adapter
is real and compiled, but player-visible gameplay effect is
a separate R213 acceptance gate.

### 4. Java8 regression and CI test integration

New mock-backed Java8 regression:
\`tests/java/dev/slinkywinky/OwnedModuleActionsSmoke.java\`

It exercises the **actual** Java field-write owner logic using
a controlled \`Port\` instead of a game process. It validates:
permission-off zero writes; admitted movement and jump writes;
disable/restore; preservation of game/physical input changed
after our write; player replacement and movement identity
changes; absent world; malformed float/cooldown proposals;
foreign-thread denial; close preventing further writes.

The existing source-owned test script
\`tests/run_r204_sprint_observation_jni.cmake\` now includes and
runs this mock-backed regression during the Java8/JNI CTest.
It requires:
\`R212_OWNED_MINECRAFT_ACTIONS_PASS\`.

## Actual Windows verification

Code revision tested:
\`2225f23fecd100adff4c17e07ab15993a215fe48\`

Device: authorized connected Windows 11 PC, JDK8, Visual Studio/MSVC,
isolated PR #2 Git checkout.

- First-party source-only native suite:
  **91/91 PASS Debug** and **91/91 PASS Release**.
- Existing Java8 JNI Sprint + three-module native control smoke:
  **PASS Debug** and **PASS Release**.
- New \`OwnedModuleActionsSmoke\` Java8 test via the same CTest:
  **PASS Debug** and **PASS Release**.
- Pinned legacy ForgeGradle 2.1 / Gradle 4.10.3 offline
  \`--rerun-tasks build\`: **BUILD SUCCESSFUL / exit 0**,
  including Java8 compilation, packaging and **reobfJar**.
- The packaged Forge adapter was inspected using JDK8
  \`javap -c -p\`: the executable includes both the explicit
  \`jumpTicks\` MCP and \`field_70773_bE\` SRG names and its
  field-write methods.
- Most recent reobfuscated local test artifact SHA256:
  \`05c48771c1648370c2c8fed648e6d42facec20b605a80a566b689c35f47d507a\`
  under \`experimental/forge189/build/libs\` in the user's isolated
  local checkout. It is **not** a production-release artifact.
- No Minecraft game process was launched in R212; we deliberately
  prioritized action ownership code and focused regressions rather
  than repeating generic startup testing.

## CI and branch boundaries

R211's final branch CI has two known Windows failures:
\`isolated_wgl_menu_state_restore\`, with the GUI-owned
\`paintedArraylistAccent\` assertion expecting F3 5D 12.
I inspected both CI logs and posted the exact failure to
the PR #1 owner. This is not evidence of an R211 JNI compile
failure and must be resolved in the GUI-owning branch rather
than changing PR #1 pixels from this module chat.

R212 final-head GitHub Actions CI must be checked independently.

## What is NOT complete

- The user-facing ClickGUI and ArrayList from PR #1 do **not**
  yet drive these native module actions end-to-end.
- No Slow and No Jump Delay still require **actual live
  in-world gameplay-effect acceptance**, action timing comparison
  to candidate event-17/21 reference and repeated enable/disable,
  world switch and focus-loss tests.
- No unverified game action is activated by the normal Forge
  preview. The R212 test lane is a guarded experimental host,
  not the final injectable client.
- The Sprint native game-action adapter remains decision-only.
- These are first-party reconstructions from candidate
  reference information, not authenticated original commercial
  Slinky parity.
- No additional original native callback was implemented:
  **16 offline module policies, 22 callback bindings and 149
  unimplemented candidate subscriptions** out of 171 remain.

PR #2 stays draft and unmerged; PR #1 implementation untouched.
