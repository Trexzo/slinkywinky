# R216 — Real in-world horizontal displacement: No Slow enabled vs disabled

Date: 2026-11-14. Repository: Trexzo/slinkywinky.
Module owner: integration/openslinky-fd9c592-reference (draft PR #2).
Parallel GUI/ArrayList/injector owner: reconstruction/r1-core-fidelity (draft PR #1).
**No PR #1 source files were edited.**

## Result

R215 demonstrated that No Slow affects native-owned
\`MovementInputFromOptions\` fields **before input consumption**,
but field values alone did not prove that the player moves
farther in the actual game world.

R216 measures **real Minecraft client player horizontal X/Z
position displacement** during three native No Slow states.
All observations use the same local creative-mode superflat
world, the same Minecraft 1.8.9 vanilla input reader, a
test-only synthetic forward binding and bow item-use
stimulus, and exactly the same verified ASM hook and
Java 8 -> JNI -> C++ native No Slow policy as R214/R215.

The measured per-phase actual player movement was:

| Native No Slow | Actual horizontal blocks, 33 measured ticks | Blocks/tick | Ratio to disabled |
|---|---:|---:|---:|
| Enabled | 7.112093519179881 | 0.21551798542969336 | 4.991164445306116 |
| Disabled | 1.424936725109983 | 0.04317990076090857 | 1.0 |
| Re-enabled | 7.123081271411472 | 0.21585094761852947 | 4.998875491023422 |

Re-enabled/initial-enabled = **1.001544939222461**
(i.e. about a **0.154%** difference).

The last accepted run recorded **137 actual player-update
native callback decisions** and **92 owned movement-field
writes**. Exactly **33 actual horizontal position-delta
samples** were measured in each state after excluding
12 transient player ticks per phase.

**The result confirms that the tested No Slow action
produced a clear, repeatable in-world horizontal movement
difference when selected.** It is not simply proof that a
float field was overwritten. It is also not an authenticated
physical-key, multiplayer or original commercial Slinky
acceptance, and the world was not used to evaluate
anticheat behavior.

## Implementation and test isolation

Updated
\`experimental/forge189/src/main/java/dev/slinkywinky/forge/ForgePreview.java\`.

A separate explicit test flag
\`-Dslinkywinky.r216DisplacementTest=true\` admits
only the new
\`%TEMP%\slinkywinky-r216-independent-forge-*\` game
directory, the dummy \`R216Offline\` identity,
the normal Forge 1.8.9 local singleplayer world
\`R216-local-only\` and the opted-in R214
post-slowdown Java 8 test agent. Normal Forge
preview remains read-only.

The client continues to use **the original**
\`MovementInputFromOptions\`, not a replacement.
The test sets \`KeyBinding.setKeyBindState(forward,true)\`
**locally inside the disposable profile** and stimulates
the bow's use state immediately before vanilla's
\`EntityPlayerSP.onLivingUpdate\`. The real Minecraft
player method refreshes the normal movement reader,
applies vanilla's 0.2× item-use slowdown, then runs
the exact source-grounded ASM call into the existing
Java 8/JNI/native No Slow policy.

For this test, the first party observer samples the
same real player \`posX\`/\`posZ\` coordinates on each
accepted post-slowdown callback. It computes the
hypotenuse of consecutive world-position deltas,
**not** the movement-input field values or a
projected/simulated path. The local player must be
grounded, in neither water nor lava, and not flying.
The sample rejects invalid coordinates, implausible
per-tick teleport/collision gaps and wrong session
or owner thread.

The first real run stopped safely when the just-spawned
player had not yet landed. This is **not** a No Slow
module failure. The later code explicitly allows up
to 80 initial falling/landing callbacks without
counting them as movement samples, then starts the
45-callback phase windows only on the real ground.
Once measurement starts, leaving the ground is a
fail-closed condition in this isolated test.

Each native activation phase has:
- 45 accepted grounded player-update callbacks;
- the first 12 discarded from the displacement
  measurement, to limit acceleration/momentum
  contamination after a state transition;
- 33 measured real horizontal position differences;
- a phase-specific position distance, average
  blocks/tick and current module-control field writes;
- a native \`setModuleEnabled("No Slow", ...)\`
  transition at the phase boundary, not a
  locally faked GUI toggle.

The final acceptance requires both enabled states
to exceed the disabled mean by at least 1.75×,
and the re-enabled/initial-enabled ratio to remain
in [0.55, 1.45]. It prints the actual measured
values; the independent verifier recomputes every
reported ratio from the three recorded phase samples.

After the complete in-world acceptance,
\`OwnedModuleActions\` and the JNI observer are
closed; the temporary synthetic binding is released.
No subsequent automatic test restart occurs.

### Exact source and JAR evidence

Updated the test-only agent
\`experimental/forge189/src/main/java/dev/slinkywinky/forge/R214NoSlowAgent.java\`
to admit the additional exact R216 TEMP directory,
but only if the separate Javaagent property is enabled.

Updated \`tools/run_r207_independent_forge_loader.ps1\`
with the new \`-R216DisplacementTest\` mode,
R216-specific dummy identity, TEMP path and pinned
JAR hash, while preserving older R207–R215
source-owned modes and hashes.

Exact first-party reobfuscated test JAR SHA256:

\`a4ef40b697019fdc209e31ee7b6e6453eb6326e952add4533033d05ad70ae08d\`

The full ForgeGradle 2.1 / Java 8
\`--offline --rerun-tasks build\` completed,
including \`reobfJar\`.

R216 accepted independent test directory:
\`%TEMP%\slinkywinky-r216-independent-forge-LIVE2\`

The earlier
\`slinkywinky-r216-independent-forge-LIVE1\`
fixture crashed at the first airborne measurement;
it was never represented as passing.

## Source-controlled independent acceptance verifier

New:
\`tools/verify_r216_displacement_receipts.ps1\`.

This verifier checks:
1. Exact named TEMP gameDir and actual local
   \`saves/R216-local-only/level.dat\`.
2. Pinned SHA256 of the one staged Forge reobfuscated
   mod and source-owned launcher preflight.
3. Correct standalone Java 8 Forge LaunchWrapper
   client, dummy username \`R216Offline\`,
   exact test-only Javaagent and JVM flags;
   refuses Gradle development launch paths.
4. Ordered, **unique** first-party hook, JNI,
   local-player join, grounded sample, three
   phase-distance and one-time cleanup markers.
5. Exactly **33 position deltas** in each of the
   three phases; checks distance/33 equals each
   logged mean in world blocks/tick.
6. Independently computes ratios from those
   means and cross-checks against the game
   acceptance line, including 135+ accepted
   native callbacks and 90+ actual input writes.
7. Refuses any reported native action failure
   or ambiguous ASM transform.
8. After positive acceptance, optionally closes
   only the identity-verified isolated Java
   test client. It does **not** stop unrelated
   Java/Minecraft/SpawnPK processes.

Actual verifier result:

\`R216_REAL_WORLD_MOVEMENT_DISPLACEMENT_NATIVE_ACTION_PASS
 callbacks=137 writes=92
 enabledRatio=4.99116444530612
 reenabledRatio=4.99887549102342
 state=VERIFIED_TEST_JVM_STOPPED_AFTER_WINDOW_CLOSE\`

The exact isolated game JVM was PID **6340**.
The verifier closed its window and stopped only
that verified leftover process; its termination
was confirmed. The test JVM's stderr file
contained **0 bytes at acceptance**.

## Windows verification

Source revision tested before documentation:
\`e2661a09e502942bffb2a0d3b146e2f40394468e\`.

- Full first-party reference C++ CTests:
  **91/91 PASS Debug**, **91/91 PASS Release**.
- Actual Java 8 -> JNI -> native three-module
  control and decision round trip:
  **PASS Debug and Release**.
- Java 8 \`OwnedModuleActions\` regression,
  including ownership, stale input and
  late-tick No Slow refusal:
  **PASS Debug and Release**.
- Pinned legacy compiled native-manager
  suite: **7/7 Debug PASS, 7/7 Release PASS**.
- ForgeGradle Java 8 full build and
  \`reobfJar\`: **PASS**.
- Independent source-controlled actual
  game displacement verifier: **PASS**.
- Four invalid-launch negative controls:
  prior test instance, real user
  \`.minecraft\` root, wrong TEMP prefix
  and an older R215 JAR: **4/4 PASS**.

Final R216 GitHub Actions CI remains a
separate result to check after the final commit.
The prior R215 workflow jobs were not
fully green (known GUI-owned ArrayList
regressions); the locally accepted module
tests are not a claim that the combined
GUI/injector branch is release ready.

## Honest fidelity and next steps

R216 substantially improves evidence for **No Slow
with bow use and forward movement**. It does not yet
verify physical WASD/mouse interaction,
original Slinky native event-21 equality, sword
and food item-use cases, vertical/flying motion,
terrain and collision, server packet results,
focus transitions, or operation from PR #1's
actual production DLL injector and user-facing
ClickGUI/ArrayList.

The new Javaagent is a TEMP-only test vehicle,
not a production injection subsystem.
The natural repeated-jumping behavior of
No Jump Delay remains separately unfinished.

Candidate reconstruction coverage has not
increased: 66 mapped module identities;
171 candidate native callback subscriptions;
16 offline policy implementations;
22 callback bindings; **149 subscriptions
still without implementations**.

PR #2 remains draft and unmerged. PR #1
has not been edited by this module track.
