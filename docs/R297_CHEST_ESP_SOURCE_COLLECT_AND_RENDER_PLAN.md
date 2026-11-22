# R297 — Chest ESP recovered Java event-0 collector and event-6 draw-plan model

Original source used: `Slinky-Verified-SourceBuild.zip`,
`source/java/src/main/java/dev/slinky/module/impl/ChestEspModule.java`.
Original Java source SHA256:
`aee1d14f1cfa1d2f16b1f6e80276a95cc719052fd5edf4e54c078fb17f47de43`.
Original immutable drawing operands taken from the same bundle's
`assets/slinkysemantic/RenderConstants.json` (SHA256
`3e6a7dba6433f6416ef73ae3193e04a09dd2e9275b0b4890f6432f743f71301e`).

Recovered source behavior:
- listens to semantic events 0 (refresh) and 6 (render), not player-event 18;
- clears rows on every event-0 tick, including when no world/player is ready;
- filters each loaded tile entity by its **world block ID**, not tile type:
  54 chest, 130 ender chest, 146 trapped chest, 138 beacon;
- applies the corresponding `render_*` flag and inclusive float-squared
  range comparison against the world **block corner**;
- retains loaded-tile encounter order, stopping at the first 100 matches;
- clears collected rows on world unload;
- calculates a per-row event-6 draw-plan translation from camera-relative
  block coordinates plus (0.5, 0.5, 0.5), draw scale 0.5,
  optional filled primitive 168960 at alpha 0.4, and outline 169008;
- source has line-width and per-kind color selection, but no world GL
  effect is invoked by this offline model.

Recovered operands: f(1383008)=0.5 (`0000003f`),
f(1383012)=0.4000000059604645 (`cdcccc3e`),
d(1383120)=0.5 (`000000000000e03f`),
d(1383128)=0.5 (`000000000000e03f`).

Implementation: `reference_compat/include/slinkywinky/chest_esp_controller.hpp`.
Dedicated regression: `reference_compat/tests/test_chest_esp_controller.cpp`.
The tests cover all four block IDs, irrelevant IDs, flags, inclusive
range boundary, loaded order, 100-row cap, no-world and unload cleanup,
exact float alpha bits, recovered draw-plan primitives and translations,
plus source-defined negative/NaN range comparison behavior.

This is one additional **offline C++ controller**, not a player-visible
ESP feature. The test does not authenticate native source event priorities,
Forge world snapshots, GL rendering, colors, GUI toggles or live Minecraft.
Do not count it as complete playable parity. PR #1 is unchanged.
