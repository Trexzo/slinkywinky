# R54 — original native HUD preview

R54 adds a first-party Windows/GDI window using the portable pure C++17 HUD model.
**It is not a Forge mod or a Minecraft in-game overlay.**

## Preview

- Windows executable: slinkywinky_hud_preview.exe, packaged beside the standalone native injector.
- Original restrained dark panel with tick count, session transitions, generation, world/player presence, camera angles, eye height and ray-selected entity distance.
- Uses only synthetic telemetry. Keyboard: 1 no attachment, 2 menu, 3 world, 4 ray target, C compact, A angles, Esc close. Window can be dragged.
- Uses Windows' installed Segoe UI via GDI. No commercial fonts/assets or external rendering libraries.
- No Minecraft process, JVM, Forge, injected hooks, input synthesis, game packet changes or networking.

## Verification limits

The HUD formatter is a pure native C++17 model, independently tested on invalid inputs, bounded row output, formatting and state transitions. Windows CI builds and checksums the standalone preview executable, but **does not open its window**. Visual inspection on a real desktop remains necessary.

Future native HUD rendering in Minecraft needs a separate thread/graphics admission and an explicit teardown plan. R54 does not modify Minecraft rendering or claim playable original commercial Slinky fidelity.