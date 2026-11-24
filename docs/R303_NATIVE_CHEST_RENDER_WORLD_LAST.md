# R303 — first native-backed Chest ESP world-render diagnostic

This milestone reaches a real Forge Minecraft 1.8.9 `RenderWorldLastEvent`
handler using the previously recovered **R297 C++ ChestEspController**.
It does **not** assert that the original client dispatched equivalent events.

### Real wiring
1. R300 takes an immutable client-thread snapshot of loaded tile
   entities and their **world block IDs**.
2. R301 marshals the same provenance-bound snapshot over JNI, updating
   the real R297 Chest, R298 Item and R299 Player native controllers.
3. New R303 `nativeChestPlan` asks the **existing C++** Chest controller
   for `plan(camera, settings)` only on the exact latest owner generation,
   world/player token and tick, with a creating-thread check.
4. JNI returns up to 100 rows of real R297 camera-relative **block
   centers**, block IDs and recovered scale=0.5/filled-alpha=0.4;
   Java rejects invalid dimensions, IDs, nonfinite coordinates,
   unexpected scales or wrong counts.
5. The Forge debug renderer submits outline edges in the world-last
   callback with temporary GL state: fixed-function GL attribute and
   matrix push/pop, no texture/light/depth writes beyond the saved
   state, and no changes to entities, packets, keys or gameplay controls.

Chest kind palette colors are **temporary diagnostic RGBs**. They are
not claimed to equal the original ClickGUI-selected colors, original
filled primitives, visibility behavior, or final client visual style.

### How to launch the isolated R303 experiment

The extended R207 launcher has a new `-R303ChestRenderTest` switch,
`-R303ExpectedJarSha256` argument, and only allows a fresh direct
TEMP child named `slinkywinky-r303-independent-forge-<unique>`,
the dummy offline identity `R303Offline`, and matching original
Forge 1.8.9/JDK8/native DLL material. This switch is exclusive with
R302/R289/R224/legacy modes. It independently creates a local flat
world `R303-local-only` and **auto-starts the observer only once**.
F9 can manually disable or re-enable the test; a disabled observer is
not silently restarted.

In the game, place a chest **manually** within the diagnostic
32-block range. The native rows should produce a diagnostic wireframe
around a matching chest. Real row-positive submissions log
`R303_CHEST_LIVE_DRAW_SUBMITTED`, and render-callback reachability logs
`R303_CHEST_RENDER_CALLBACK_PASS` after 40 render frames. An empty
flat world can correctly report zero matched rows. A log marker
is not proof that pixels are visible; validate with original-game
screenshots and frame-time measurements on the authorized Windows PC.

A reproducible exact CI artifact hash will be pinned into the launcher
once the new R303 Forge JAR builds. Prior R302 hash pins remain unchanged.

### Remaining blockers to full Slinky fidelity
- Original native event-0 vs event-6 timing, exact GL primitives,
  original colors/opacity/shape, chests behind occluders, responsive
  settings and draw-thread correctness still require actual in-world
  and recovered decrypted-reference comparison.
- Item ESP source count-merge text, item colors/shape; Player ESP 3D
  boxes and 2D projection/health HUD are not rendered here.
- Production **vanilla Minecraft 1.8.9 injection** and original
  ClickGUI/HUD parity are independent and unfinished. Forge remains an
  experimental harness, not the final injector.
- R303 is *not* verified visually until a genuine owner-machine
  world, chest, active module, and captured frame can be reviewed.

Do not merge either draft PR based only on GitHub CI.
