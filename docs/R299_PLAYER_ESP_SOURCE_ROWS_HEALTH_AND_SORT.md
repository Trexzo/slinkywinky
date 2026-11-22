# R299 — Player ESP source event-18 row assembly, health and sort

Source: `Slinky-Verified-SourceBuild.zip` Java 8
`source/java/src/main/java/dev/slinky/module/impl/PlayerEspModule.java`.
SHA256 `603a98c58ad4933f519fee362dd1b1a65d8bf381f2b34610950f3d9326f12e2f`.
`RenderConstants.f(1378848)` recovers IEEE754 float 1.0
from `RenderConstants.json` SHA256
`3e6a7dba6433f6416ef73ae3193e04a09dd2e9275b0b4890f6432f743f71301e`.

The source listens to type 18 (world player snapshot), type 6 (3D
world mode only) and type 7 (2D screen mode only). This C++ model covers
read-only row selection on event 18. It does not submit any draw plan.

Important recovered details:
- `update` returns without clearing when `c.ready()` is false;
  only a ready snapshot clears/replaces rows; world unload clears rows;
- excludes local player, dead players and relationship bits 2 or 7,
  depending on hide_friendlies; includes others within range squared;
- collection is limited to 100 before sorting;
- color selection is target override, then in-game name color when
  `color_mode==0`, otherwise friendly/enemy color using bits 1 or 4;
- health uses nonnegative absorption, normalized `(health+extra)/
  (maxHealth+extra)` and separate `extra/health`, with zero-divisor
  guards and [0,1] clamping;
- distance is narrowed to float and sorted by original
  `Float.compare(b.distance, a.distance)` **descending/farthest-first**;
  original tie order is preserved, including Java NaN/negative-zero
  comparison behavior;
- read-only mode routing is modeled (world=choice 1, screen=choice 0).

Source comparison uses the original `RenderMath.distanceSquared`
operation order `z*z+(x*x+y*y)` and original Java float comparison
semantics, including NaN rows. This does not assert correct visual
ordering or user desirability; it preserves source behavior.

Files: `reference_compat/include/slinkywinky/player_esp_controller.hpp`
and `reference_compat/tests/test_player_esp_controller.cpp`.

Not done: actual player-list sampling through Forge/JNI, 2D screen
projection, 3D box rendering, text/health bar, color value resolution,
GL integration, native input ownership, GUI, and original commercial
binary behavioral parity. This remains an **offline** controller.
PR #1 remains untouched and draft.
