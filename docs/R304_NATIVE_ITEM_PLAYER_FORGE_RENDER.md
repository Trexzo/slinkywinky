# R304 — native-backed diagnostic Item ESP and Player ESP rendering

R304 extends the R303 experimental Forge 1.8.9 3D world-last renderer,
without pretending the original Slinky ClickGUI or event dispatch is
already faithfully reproduced. The R300 immutable Forge world sampler
feeds real Minecraft loaded entity/roster snapshots into the **existing**
R298 `ItemEspController` and R299 `PlayerEspController` through R301 JNI.

## New native render data

`EspNativeReadOnlyPreview.nativeEntityPlan` returns a strictly bounded
versioned packet of **actual C++ source-model row results**, not a separate
Java reconstruction. The JNI owner must be the creating thread. The
world/player token, generation and tick must match the most recent
accepted R301 observation; stale/out-of-order/foreign-world render requests
are rejected. Previous/current positions are interpolated with a
finite, 0–1 partial-tick fraction, then translated relative to the
Forge render camera. The functions are read-only.

- **Item ESP**: source encounter-order rows, item IDs, and source
  count-merging results are carried to the renderer. Rows with
  `quantity == 0` are suppressed because the original source merged
  them into another same-ID row. Marker size indicates source count
  diagnostically; it does **not** implement the original count font.
- **Player ESP**: the actual source 100-row cap and farthest-first
  ordering are retained; normalized source health, absorption ratio
  and hurt time are forwarded. A diagnostic 3D line box and health
  bar are drawn, not the original Slinky 2D projection, color
  selection, health display or exact box geometry.

All draw rows and numeric fields are bounded and validated in native
and Java. `ForgeEntityEspDebugRenderer` uses a temporary GL11
attribute/matrix scope restored in `finally`. No gameplay entities,
movement, keys, world data or packets are mutated by the ESP path.

## Separate acceptance, not production installation

`tools/run_r207_independent_forge_loader.ps1` gains
`-R304EntityRenderTest` and `-R304ExpectedJarSha256`, exclusive with
R303/R302/legacy tests. Only a fresh direct TEMP gameDir named
`slinkywinky-r304-independent-forge-<unique>`, the dummy offline
identity `R304Offline`, verified Forge libraries/Java 8, and a
matching first-party native DLL are eligible. The resulting disposable
singleplayer world is named `R304-local-only`, and the observer
auto-starts once. The R304 specific Forge JAR hash is pinned after a
successful exact-source Forge build; old SHA-pinned modes are unchanged.

`R304_ENTITY_RENDER_CALLBACK_PASS` documents 40 real render callbacks.
`R304_ITEM_NATIVE_RENDER_SUBMITTED` and
`R304_PLAYER_NATIVE_RENDER_SUBMITTED` require eligible live item/player
rows, so a new flat singleplayer world can legitimately emit neither.
Do not call a CI build, count-only log or synthetic row test proof of
visible in-game rendering. The owner desktop was offline during authoring.

## JNI regression scope

The Java 8 native CTest exercises exact item ID/quantity, interpolated
camera coordinates, source health/absorption/hurt values, same-ID
quantity merge with zeroed contributor, player farthest-first sort,
invalid partialTicks, stale tick, stale world and foreign-thread
denial. ForgeGradle compiles and packages the actual render class.

## Outstanding full-client reconstruction work

Actual live 1.8.9 world screenshots, FPS/latency measurement, original
event 0/18/6/7 semantics, item count fonts, player 2D projection,
original OpenGL colors/primitives, the complete ClickGUI/HUD, and
injection into unmodified vanilla 1.8.9 remain unfinished. PR #1
and PR #2 stay draft/unmerged. The R304 scope is diagnostic integration.
