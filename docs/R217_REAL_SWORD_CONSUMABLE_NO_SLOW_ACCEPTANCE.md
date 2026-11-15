# R217 — Real sword and consumable No Slow movement acceptance

Date: 2026-11-14. Repository: Trexzo/slinkywinky.
Module branch: integration/openslinky-fd9c592-reference, draft PR #2.
Parallel GUI/ArrayList/injector branch: PR #1, untouched by this work.

## Outcome

R216 established actual Minecraft 1.8.9 horizontal player displacement
with No Slow enabled/disabled while using a bow. R217 exercises the
same source-grounded, post-vanilla-slowdown ASM hook, the existing
Java 8 → JNI → native C++ No Slow policy, native module toggles and
its three independent settings. It uses the ORIGINAL Minecraft
MovementInputFromOptions object, not the R214 test substitute.

The six verified native-state/category movement windows were:

| Actual item | No Slow setting | Actual horizontal blocks (33 ticks) | Mean blocks/tick | Ratio vs item disabled |
|---|---:|---:|---:|---:|
| Diamond sword, ID 276 | 65% | 4.630021934121629 | 0.1403036949733827 | 3.24954895735424× |
| Diamond sword, ID 276 | Disabled | 1.4248198734298683 | 0.0431763598009051 | 1× |
| Diamond sword, ID 276 | 40% | 2.849272617240672 | 0.08634159446183855 | 1.9997423326092578× |
| Apple, ID 260 | 45% | 3.205489932154819 | 0.09713605855014604 | 2.2498564006128015× |
| Apple, ID 260 | Disabled | 1.42475312259117 | 0.04317433704821727 | 1× |
| Apple, ID 260 | 80% | 5.698478483636251 | 0.17268116617079549 | 3.999625193501975× |

The two vanilla disabled means have a cross-item ratio of
0.9999531513842956. Expected native ratios were 3.25,
2.0, 2.25 and 4.0, respectively.

The accepted test recorded 272 real game-thread native callback decisions,
182 owner-scoped game-field writes, 33 measured X/Z position deltas per
phase after 12 warmup ticks, one final cleanup and zero native action
failures. Actual movement, not merely input fields, was measured.

## Implementation

Updated experimental/forge189/src/main/java/dev/slinkywinky/forge/ForgePreview.java
with a separately gated R217 item-categories test. The new opt-in
JVM property is slinkywinky.r217ItemCategoriesTest=true. It requires:
- TEMP-only game directory named slinkywinky-r217-independent-forge-*;
- dummy account R217Offline;
- isolated local creative superflat world R217-local-only;
- explicit source-grounded Java8 test agent enabled;
- same owner-thread native module control and post-0.2× hook.

The pre-vanilla LivingUpdateEvent sets an explicitly synthetic
local forward-key binding and the test-only item-use stimulus.
Minecraft refreshes its OWN MovementInputFromOptions class and applies
the vanilla 0.2× slowdown. Only at the verified later ASM hook is the
native No Slow action applied.

The six 45-tick phase settings are:
1. Diamond sword, native enabled, sword multiplier 0.65.
2. Sword, native disabled; vanilla baseline.
3. Sword, native re-enabled and sword multiplier changed to 0.40.
4. Change item to apple, native enabled, consumable multiplier 0.45.
5. Apple, native disabled; vanilla baseline.
6. Apple, native re-enabled and consumable multiplier changed to 0.80.

Actual JVM item IDs are checked at the native decision point.
Transitions use SprintObservationProbe.setModuleEnabled and
setNoSlowSettings, not GUI demo flags. The Java action owner
applies only acknowledged native settings and cleans up after
the final phase. The earlier R216 three-phase path stays
available with its original flag and hash.

The test-only agent R214NoSlowAgent was extended to admit the
specific R217 TEMP directory, without granting access to a
normal Minecraft launch. The Javaagent is NOT the finished native
injectable client.

Updated tools/run_r207_independent_forge_loader.ps1 with
-R217ItemCategoriesTest. Every older hash-pinned launcher mode is
preserved. R217 reobfuscated Forge Java8 JAR SHA256:

8f03d30da031aeb21d72eb0dbc5cd337b09aadfdc673988101bb64a28b29bcff

Actual ForgeGradle 2.1, Java8, offline full build and reobfJar PASS.

## Independent verification and process cleanup

New tools/verify_r217_item_category_receipts.ps1:
- checks the exact TEMP gameDir, real local world save,
  staged JAR SHA256 and pinned loader preflight;
- checks the exact live Java8 Forge process, R217Offline name
  and opt-in Javaagent flags;
- requires ordered, unique startup, actual player update,
  all six item/category phase markers and one cleanup;
- recomputes six mean blocks-per-tick numbers from measured
  game world-position distances, plus five independent ratios;
- validates sword ID 276, apple ID 260, configured multipliers,
  33 measured ticks per phase, native callbacks/writes and
  absence of an action or transformation error;
- after a successful acceptance, can close and stop ONLY
  that exact verified test JVM.

The first verifier attempt found a Windows PowerShell array
arithmetic precedence issue; it was fixed in the verifier
source. The original test window was closed before its PID
could subsequently be verified; we did not treat this
attempt as a completed process-identity check.

We reran the full test in fresh TEMP instance
slinkywinky-r217-independent-forge-LIVE2, and invoked the
corrected verifier immediately after the live final marker.
The independent check PASS result:

R217_REAL_SWORD_CONSUMABLE_DISPLACEMENT_NATIVE_ACTION_PASS
callbacks=272 writes=182 sword65=3.24954895735424
sword40=1.99974233260926 apple45=2.2498564006128
apple80=3.99962519350197
state=VERIFIED_TEST_JVM_STOPPED_AFTER_WINDOW_CLOSE

Only the verified temporary Java process (PID 29896) was
stopped; the independent receipt remains in its TEMP folder.
No normal user Minecraft, account, other Java or SpawnPK
process was modified or stopped.

## Regression suite

After the changes, source revision
b41d61dcdcf94d29c7bf9615818e4a5595bd9528:

- C++ source-owned native CTests: 91/91 Debug and 91/91 Release PASS.
- Java8 → JNI → C++ three-module settings/toggle round trip:
  PASS in Debug and Release.
- Java8 action ownership and late-tick No Slow refusal: PASS
  in Debug and Release.
- Pinned compiled native-manager tests: 7/7 in Debug and Release.
- Java8 ForgeGradle full build and reobfJar: PASS.
- Invalid launcher controls 4/4 PASS: existing instance,
  actual user Minecraft directory, wrong TEMP prefix,
  and older R216 JAR.
- Independent six-phase real world displacement verifier: PASS.

Final GitHub Actions for the R217 head still require separate
confirmation; previous R216 workflows had outstanding failures
outside this isolated acceptance.

## Boundaries and next steps

This proves category-specific native No Slow multiplier settings
and changes to actual in-game horizontal movement, using
artificial item-use and a simulated forward-key binding in a
credential-free singleplayer test. It does NOT prove physical
human input, original proprietary Slinky parity, multiplayer
network movement, fully natural sword/food behavior, terrain,
all held-item variants or production DLL injection.

PR #1 must still connect the actual ClickGUI toggle to the
acknowledged native module state, ArrayList HUD and correct
game-thread callback. This module track did not modify PR #1.

Original reconstruction coverage is unchanged: 66 mapped
module identities, 171 candidate callback subscriptions,
16 first-party offline policy models, 22 offline callback
bindings, 149 candidate callbacks still not implemented.

PR #2 remains draft/unmerged.
