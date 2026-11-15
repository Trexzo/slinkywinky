# R214 — Real Forge 1.8.9 pre-consumption No Slow native-module acceptance

Date: 2026-11-14. Repository: Trexzo/slinkywinky.
Owner: PR #2, integration/openslinky-fd9c592-reference.
Separate GUI/injector owner: PR #1, reconstruction/r1-core-fidelity.
**PR #1 source files were not edited.**

## Outcome and limits

R213 proved that Forge's **PlayerTickEvent.END** is too late
to apply the No Slow input correction. R214 now provides an
experimental but **real** post-vanilla-slowdown/pre-consumption
callback, routes it into the *existing* Java 8 -> JNI -> C++
native No Slow policy, and applies the returned input correction
through the already owned, opt-in action port.

In an independent installed Forge 1.8.9 singleplayer process,
with the exact Java8/JAR fingerprints and one isolated TEMP
game directory, three actual module activation phases passed:

| Native No Slow selection | Player movement field after vanilla 0.2× | Actual result |
|---|---|---|
| Enabled | 0.2 forward | 1.0 forward, 10/10 accepted native writes |
| Disabled | 0.2 forward | remained 0.2, 10/10 admitted, zero writes |
| Re-enabled | 0.2 forward | 1.0 forward, 10/10 accepted native writes |

The test observed **30 admitted actual player-update callbacks**,
including **20 confirmed input-field writes**. The 30-sample
sequence took **1400 ms** in the accepted run.

This **does not** prove physical-key acceptance, differences
in distance traveled, competitive/multiplayer behavior,
the original commercial native event-21 timing, or a final
injectable client. The input and item-use stimuli were
explicitly **synthetic, local, isolated test fixtures**.
The code used a real Minecraft player/world and native
module state but did not interact with any external gameplay
server or real account.

## Bytecode source evidence and exact insertion location

We inspected the installed, pinned Forge 1.8.9 stable_22
deobfuscated \`EntityPlayerSP\` class with Java8 \`javap\`,
and verified production SRG identifiers in the actual local
ForgeGradle reobfuscation mapping:

- \`EntityPlayerSP.onLivingUpdate\` /
  \`func_70636_d\`
- \`EntityPlayerSP.movementInput\` /
  \`field_71158_b\`
- \`MovementInput.moveStrafe\` /
  \`field_78902_a\`
- \`MovementInput.moveForward\` /
  \`field_78900_b\`

Within the method, vanilla calls
\`MovementInput.updatePlayerMoveState\`, then for an
item-using non-riding player multiplies strafe and forward
by \`0.2f\`. Our transformer matches both **float FMUL 0.2f
PUTFIELD** writes and inserts the hook **immediately after
the forward PUTFIELD**, before the subsequent player-movement
logic. It counts the refresh and both stores and refuses
ambiguous classes. The transformer does nothing outside
the explicitly named class.

This hook is tested against the real ForgeBin class by the
source-controlled Java8 ASM smoke:
\`tests/java/dev/slinkywinky/forge/R214TransformerSmoke.java\`.

That smoke passes with **exactly one hook**, immediately
after the second vanilla 0.2× write. It also rejects corrupt
class bytes. We did **not** infer that Forge's earlier
\`LivingUpdateEvent\` itself was an input-consumption hook;
it is earlier than the slowdown and used only to set the
synthetic item-use stimulus in our test.

## Code additions

### Precise, opt-in test Javaagent

- \`experimental/forge189/src/main/java/dev/slinkywinky/forge/R214SlowdownTransformer.java\`
- \`experimental/forge189/src/main/java/dev/slinkywinky/forge/R214NoSlowAgent.java\`
- \`experimental/forge189/src/main/java/dev/slinkywinky/forge/R214NoSlowHook.java\`
- \`experimental/forge189/build.gradle\` (Premain-Class only)

The isolated game uses an ordinary Java 8 \`-javaagent\`
to insert this **source-specific** ASM callback on the
game classload, with the exact local package symbols.
It must be **explicitly** enabled; the normal Forge preview
does not use the agent.

The agent checks that its working directory is inside the
expected named Windows TEMP instance and is opted in by
\`-Dslinkywinky.r214NoSlowAgent=true\`.
The Java/Forge host additionally checks the real loaded
Minecraft game directory, dummy R214Offline session, local
integrated-server world, and separate
\`slinkywinky.r214LocalNoSlowAcceptance=true\` property.

The Forge preview JAR itself is not an installable production
injectable DLL; this test agent demonstrates the correct
callback location independently of PR #1's native injection.

### Native module and action wiring

- Existing \`SprintObservationProbe.observeModules(...)\` JNI
  invokes the native No Slow policy using item ID 261 (bow).
- The verified native \`moduleState("No Slow")\` is used
  to decide whether an action is active, **not** any
  local GUI preview flag.
- Existing \`OwnedModuleActions\` receives
  \`postSlowPreConsumeVerified=true\` **only** in the
  exact verified ASM post-slowdown call. The late Forge
  \`PlayerTickEvent.END\` continues to supply false and
  is blocked from writing No Slow.
- Real \`ForgeModuleActionPort\` conditionally writes
  \`movementInput.moveForward\` and \`moveStrafe\`.
  Its owner cleans up still-owned fields without
  overwriting subsequently changed game/physical input.

The native No Jump Delay candidate action remains an
independent callback and its R213 cooldown-field acceptance
remains valid. R214 did not change the native policy counts
or add new candidate native subscriptions.

### Synthetic local-world acceptance fixture

\`ForgePreview\` in the R214 test-only mode:
1. Creates \`R214-local-only\` under a fresh TEMP gameDir.
2. Keeps only its temporary integrated singleplayer server
   ticking when the window is unfocused.
3. Supplies a test-only \`MovementInput\` subclass whose
   \`updatePlayerMoveState\` returns forward=1 and strafe=0;
   Minecraft itself applies the 0.2× vanilla slowdown.
4. Places a bow in the temporary creative-mode player's
   inventory and uses the Forge \`LivingUpdateEvent\` as
   a **pre-vanilla test stimulus**, setting the bow use state
   right before real \`EntityPlayerSP.onLivingUpdate\`.
5. Calls the actual C++ No Slow decision through JNI
   at the precisely inserted post-slowdown callback.
6. Runs three **10-call** source-grounded native selection
   phases: enabled, disabled, re-enabled. Each uses
   \`setModuleEnabled("No Slow", ...)\`, **not a Java mock**.
7. After the final accepted phase, closes its action
   owner and JNI preview and restores the original
   Minecraft \`MovementInput\` object.
8. Does not auto-restart the test on later client ticks.
   This was fixed after an earlier run logged duplicate
   cleanup receipts.

Early local debugging had two unresolved assumptions:
- Java's synthetic key press was lost by unfocused game
  input handling; the final test uses the deterministic
  \`MovementInput\` subclass, not an OS key.
- Calling \`setItemInUse\` at client tick END alone did not
  reliably maintain item use. The final TEMP-only
  \`LivingUpdateEvent\` fixture sets item use immediately
  before vanilla onLivingUpdate.

These fixes are test-scoped and do not modify the regular
game profile or original client.

## Final independent live acceptance

Host: authorized connected Windows machine,
Temurin Java 8, installed Forge 1.8.9.
JAR SHA256 accepted and pinned:

\`aa7bb470c3c5996ed08fa16d8962abe4aed0e8ae3ca07c5909351904452d78d8\`

Fresh isolated directory:
\`%TEMP%\slinkywinky-r214-independent-forge-LIVE6\`

Source-controlled launcher:
\`tools/run_r207_independent_forge_loader.ps1
 -R214LocalNoSlowAcceptance\`

The pinned loader discovered 48 library JARs and unpacked
14 existing Windows natives into the TEMP game folder.
No real Minecraft saves, mods, settings or account tokens
were copied into that folder.

Actual game log, in sequence:
- \`R214_LOCAL_AGENT_ARMED\`
- \`R214_POST_SLOW_PRE_CONSUME_ASM_PATCH_PASS\`
- \`R214_ACTION_ISOLATION_GATE_PASS\`
- \`JNI_STARTUP_PROBE_PASS\`
- \`Forge Mod Loader has successfully loaded 4 mods\`
- \`R214_LOCAL_WORLD_REQUESTED\`
- \`R214Offline joined the game\`
- \`R214_LOCAL_INPUT_FIXTURE_ARMED\`
- \`R214_REAL_ENTITYPLAYER_ONLIVINGUPDATE_HOOK_REACHED\`
- \`R214_NO_SLOW_ENABLED_INPUT_0P2_TO_1P0_PASS samples=10\`
- \`R214_NO_SLOW_DISABLED_INPUT_REMAINS_0P2_PASS samples=10\`
- \`R214_NO_SLOW_REENABLED_INPUT_0P2_TO_1P0_PASS samples=10\`
- \`R214_IN_WORLD_NO_SLOW_PRECONSUME_PASS
  callbacks=30 verifiedFieldWrites=20
  phases=enabled_disabled_reenabled
  beforeForward=0.2 afterForward=1.0 itemId=261
  elapsedMs=1400\`
- \`R214_INPUT_FIXTURE_AND_ACTION_CLEANUP_PASS\`

Each phase and cleanup marker appeared **exactly once**.
No \`R214_NO_SLOW_ACTION_FAIL\` was logged, and the
test JVM's stderr file was 0 bytes at acceptance.

Independent source-controlled verifier:
\`tools/verify_r214_no_slow_action_receipts.ps1\`.

It checks exact SHA256, TEMP gameDir and local save,
dummy username, standalone Forge/JDK8/javaagent process
identity and JVM flags, order and unique presence of
the real game markers, numeric input and callback
constraints, and clean test session shutdown.

Verifier result:
\`R214_REAL_PRECONSUMPTION_NO_SLOW_NATIVE_ACTION_PASS
 callbacks=30 writes=20 elapsed=1400
 state=VERIFIED_TEST_PID_STOPPED_AFTER_WINDOW_CLOSE\`

The accepted game window was closed and **only** the same
identity-verified leftover test JVM PID 5764 was stopped.
No other Java/Minecraft/SpawnPK process was terminated.

## Regressions and safety controls

After the final tested source, Windows:
- first-party C++ reference suite **91/91 Debug** PASS,
  **91/91 Release** PASS;
- actual Java 8 → JNI → C++ three-module control and
  native decision round trip PASS Debug/Release;
- Java 8 \`OwnedModuleActions\` regression including
  late-phase rejection PASS Debug/Release;
- pinned native-manager regression **7/7 Debug** PASS,
  **7/7 Release** PASS;
- actual full ForgeGradle 2.1 offline Java8
  \`build\`, \`reobfJar\`: SUCCESS;
- true ForgeBin Java8 ASM transformer smoke PASS:
  \`R214_REAL_FORGEBIN_POST_SLOW_PRE_CONSUME_TRANSFORM_PASS\`;
- final JAR manifest includes the exact
  test-only \`Premain-Class\`;
- four launcher negative controls PASS:
  existing instance, user's real \`.minecraft\`,
  incorrect TEMP prefix and older/unpinned JAR.

Final code acceptance revision prior to documentation:
\`0c765c6ae27f27848e9886ce393c83b831482916\`.

R213's final GitHub Actions had two green audits and
three Windows failures from GUI-owned ArrayList accent
pixel expectations, not the module JNI implementation.
R214 final-head GitHub Actions must be checked separately.
PR #1's GUI, ArrayList and native injector code was not
modified by Chat 2.

## Production integration contract for Chat 1

Chat 1 owns the native injector, JVMTI/JNI lifecycle,
GUI input ownership and ArrayList. This R214 Javaagent is
strictly an **isolated timing acceptance vehicle**, not
the final production injector.

To implement real No Slow on PR #1, instrument or otherwise
admit the matching \`EntityPlayerSP.onLivingUpdate\` phase
**immediately after** the two vanilla 0.2× \`MovementInput\`
field writes and **before** subsequent movement consumption.
At that exact point, deliver one coherent module input
snapshot through the existing owner-thread JNI module
control system. Use the native acknowledged No Slow selection,
typed item ID and multipliers, and bounded action owner.
Never call JNI game action operations from WGL
SwapBuffers, a GUI render thread or Forge
\`PlayerTickEvent.END\`.

Future acceptance must still prove physical player keys,
sword/consumable multipliers, world focus loss,
multiplayer limitations and actual motion/position
differences under normal player input. Original commercial
event-21 timing and proprietary binary parity have
**not** been verified. A production release also requires
the actual GUI/ArrayList and injected JVM lifecycle.

Unchanged full candidate catalogue:
66 mapped module identities; 171 potential native callback
subscriptions; 16 first-party offline policies; 22
implemented callback bindings; **149 unimplemented**.

PR #2 remains draft and unmerged.
