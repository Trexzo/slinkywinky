# R215 — Minecraft's vanilla movement-input reader accepted through native No Slow

Date: 2026-11-14.
Repository: Trexzo/slinkywinky.
Branch: integration/openslinky-fd9c592-reference (draft PR #2).
Parallel PR #1 owns the native injector, GUI and ArrayList; **no PR #1 files were edited**.

## Scope

R214 demonstrated that a source-verified Java 8 game-class hook
immediately after vanilla's two 0.2× slowdown stores can route a
real in-world player update into the existing JNI/native No Slow
policy and owner-scoped movement-field writer. R214's stimulus
replaced the player's regular \`MovementInput\` class with a
special deterministic subclass, which limited how directly its
input-side acceptance represented an ordinary Minecraft client.

R215 tests the **same actual Forge 1.8.9 player-update method,
JNI/native module selection, and game-field action ownership**
while keeping the normal Minecraft
\`net.minecraft.util.MovementInputFromOptions\` object installed.

A synthetic test-only forward key binding, **not a physical keyboard
press**, is inserted in the named dummy-account local singleplayer
profile immediately before the normal vanilla movement-input refresh.
The real vanilla input reader generates forward=1.0; Minecraft's own
item-use branch reduces it to 0.2; the existing R214
post-vanilla-slowdown ASM hook calls the native No Slow decision
and restores it to 1.0 when selected.

## Implementation

Updated \`experimental/forge189/src/main/java/dev/slinkywinky/forge/ForgePreview.java\`:

- New separate opt-in property
  \`-Dslinkywinky.r215VanillaInputTest=true\`.
- Uses the **exact** new TEMP root
  \`slinkywinky-r215-independent-forge-*\`, dummy
  \`R215Offline\` session and its own new local save
  \`R215-local-only\`. Refuses missing explicit
  \`slinkywinky.r214NoSlowAgent\` test-only Javaagent.
- Verifies that the player's \`movementInput\` is an actual
  \`MovementInputFromOptions\`; records the *original object
  identity* and refuses player or input-object replacement.
  Unlike the R214 fixture, R215 does **not** overwrite
  the player's movement-input object.
- Uses \`KeyBinding.setKeyBindState(...,true)\` on the TEMP
  dummy player's forward key, including a pre-vanilla
  \`LivingUpdateEvent\` reassertion so the vanilla input
  reader sees the synthetic test key. The original
  registered Forge key binding is not reconfigured.
- Supplies a bow (item ID 261) and the local item-use state
  in the same pre-vanilla test phase; *vanilla itself* scales
  the input by 0.2.
- Calls the same R214/JNI/C++ No Slow policy and
  \`OwnedModuleActions\` at the original verified ASM hook.
  The existing Forge \`PlayerTickEvent.END\` path remains
  explicitly too late and cannot apply a No Slow write.
- Accepts three independent native toggling phases:
  enabled, disabled, re-enabled. The module manager state
  must agree with each decision.
- Unsets the **test-only** forward key binding and closes
  the native action/observer after acceptance; never restarts
  the test automatically.

Updated \`experimental/forge189/src/main/java/dev/slinkywinky/forge/R214NoSlowAgent.java\`
only to admit **both** the exact R214 and R215 named
TEMP gameDir patterns. It remains inert absent the
explicit test Javaagent property.

Updated \`tools/run_r207_independent_forge_loader.ps1\`
with \`-R215VanillaInputTest\`, the independent JAR hash,
TEMP root check, dummy account, and the explicit agent
JVM arguments. Earlier R207/R208/R209/R213/R214 flags
and their pinned hash values are retained.

Added \`tools/verify_r215_vanilla_input_receipts.ps1\`.
It checks the exact JAR digest, actual independent Forge
process identity, Java 8, dummy user, TEMP gameDir,
standalone Javaagent args, local world, ordered and
unique in-game receipt markers, the actual vanilla
movement-input class diagnostics, the numeric native
callback/field-write data, and one-time cleanup. It
then optionally closes/stops **only** the same
identity-verified isolated test JVM.

## Real game acceptance

Windows 11, Temurin Java 8,
Forge 1.8.9-11.15.1.2318-1.8.9;
Mojang vanilla 1.8.9 and Forge libraries read-only.

Reobfuscated first-party test JAR SHA256:

\`6620805e5f26438560e72a2f96280ca95f3d351323edde385ede37ed9b5e3915\`

Isolated test root:
\`%TEMP%\slinkywinky-r215-independent-forge-LIVE1\`

Real log markers in order:
1. \`R214_LOCAL_AGENT_ARMED\` and
   \`R214_POST_SLOW_PRE_CONSUME_ASM_PATCH_PASS\`.
2. \`R215_VANILLA_INPUT_ISOLATION_GATE_PASS\`.
3. \`JNI_STARTUP_PROBE_PASS\`;
   Forge successfully loaded 4 mods.
4. \`R215_LOCAL_WORLD_REQUESTED\`;
   \`R215Offline joined the game\`.
5. \`R215_REAL_MOVEMENT_INPUT_ATTACHED
   synthetic_binding=forward\`.
6. \`R214_REAL_ENTITYPLAYER_ONLIVINGUPDATE_HOOK_REACHED\`.
7. \`R214_NO_SLOW_ENABLED_INPUT_0P2_TO_1P0_PASS samples=10\`.
8. \`R214_NO_SLOW_DISABLED_INPUT_REMAINS_0P2_PASS samples=10\`.
9. \`R214_NO_SLOW_REENABLED_INPUT_0P2_TO_1P0_PASS samples=10\`.
10. \`R215_VANILLA_INPUT_THREE_PHASES_PASS
    movementClass=MovementInputFromOptions
    keySource=synthetic_local_binding callbacks=30 writes=20\`.
11. \`R214_IN_WORLD_NO_SLOW_PRECONSUME_PASS
    callbacks=30 verifiedFieldWrites=20
    phases=enabled_disabled_reenabled
    beforeForward=0.2 afterForward=1.0
    itemId=261 elapsedMs=1529\`.
12. \`R215_VANILLA_INPUT_CLEANUP_PASS\` (exactly once).

The actual client-tick diagnostic independently printed
\`movementClass=MovementInputFromOptions\`, confirming
that the original vanilla class was in place, not the
R214 substitute.

Independent source verifier result:

\`R215_VANILLA_MOVEMENT_INPUT_NATIVE_ACTION_PASS
 callbacks=30 writes=20 elapsed=1529
 state=VERIFIED_TEST_PID_STOPPED_AFTER_WINDOW_CLOSE\`

The accepted JVM's stderr file contained 0 bytes at
acceptance. Only the identity-checked isolated
Java8 PID 8216 was terminated after the game window
closed. No user SpawnPK, normal Minecraft, other Java
process, or game profile was modified.

## Regressions and negative controls

After the final implementation:
- Full first-party C++ suite **91/91 PASS in Debug
  and Release**.
- Real Java8 → JNI → C++ three-module roundtrip PASS
  Debug and Release.
- Java8 owned-action gate and late-No-Slow timing
  regression PASS Debug and Release.
- Pinned compiled-native-manager **7/7 PASS in both
  Debug and Release**.
- Full ForgeGradle 2.1 offline Java8
  \`build\`, including \`reobfJar\`: SUCCESS.
- ASM source test against the actual ForgeBin
  player class: **exactly one post-slowdown insertion
  PASS**, with game actions disabled in that test.
- Final JAR SHA256 and \`Premain-Class\`
  manifest verified.
- Launcher refuses existing test output, the real
  \`.minecraft\` gameDir, incorrect TEMP prefix and
  an older R214 reobfuscated JAR: **4/4 PASS**.

Test source revision before documentation:
\`ec3673e67fe57bc674b57bb128ad09eca899bb0c\`.

## What remains before full client parity

R215 proves that a real Forge 1.8.9 client can
use **its normal movement-input reader**, be scaled
by vanilla's actual item-use branch, and receive
an owner-scoped native No Slow correction at the
pre-consumption hook, with live module toggling.

R215 **does not** prove:
- physical W/A/S/D or user-held mouse input;
- movement distance/speed over terrain or
  natural blocking and collision behavior;
- sword and consumable item multipliers;
- multiplayer/anticheat compatibility;
- correct native JVMTI hook ownership and
  full lifecycle from the eventual PR #1 injector;
- original proprietary commercial Slinky
  event-21/parity or full ghost-client functionality;
- a finished ClickGUI, ArrayList or remaining
  candidate module implementations.

R215 Javaagent is an **isolated timing acceptance
vehicle**, not an injectable release client.
PR #1 still owns injection, GUI and HUD wiring.
PR #2 owns module decisions, typed game actions,
their lifecycle and test evidence.

Offline reconstruction coverage remains
**66 catalogued module identities;
171 candidate callback subscriptions;
16 offline policy models; 22 bindings implemented;
149 candidate subscriptions unimplemented**.

PR #2 remains draft/unmerged. Final GitHub Actions
CI must be checked on the final commit.
