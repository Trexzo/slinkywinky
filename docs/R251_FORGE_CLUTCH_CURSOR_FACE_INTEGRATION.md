# R251 — bounded previous-plan cursor-face observations in real Forge 1.8.9

PR #2 development only. PR #1 remains unchanged.

An explicitly separate opt-in `-Dslinkywinky.r251ClutchCursorFaceReadOnlyProbe=true`
requires both existing R237 and R245 isolation properties. The previous
TEMP-only, `R237Offline` dummy profile, local single-player, F9,
read-only Sprint observation and no-action-mode gates still apply.

On each existing `PlayerTickEvent.END` after a native candidate from
the previous tick, the `ClutchFaceSampler` reads the actual
`Minecraft.objectMouseOver` *once*. It binds exact world, player,
generation and monotonically increasing tick identity and checks the
previous native selected block plus its `faceUp` interpretation.
No cursor data is invented, reused after world swaps, or treated as input
to an actual click.

After 20 candidate snapshots, counts are independently classified:

- `R251_CLUTCH_CURSOR_FACE_INCONCLUSIVE` if no prior face matched.
- `R251_CLUTCH_CURSOR_FACE_MATCH_OBSERVED_NOT_PLACEMENT` only when
  at least one prior-plan target AND top/side hit matched. This is merely
  observed cursor alignment; the cursor may lag PlayerTick.END.

Malicious/inconsistent counter states are rejected by pure Java 8
negative controls in the R237 workflow. The separate Forge 1.8.9
workflow checks the genuine Minecraft field/method API. No JNI
gameplay write, input synthesis, rotation, packet, block placement,
hotbar mutation or mouse hook is added.

**Still missing:** live local-world acceptance; tying a true event-0
callback, exact click timer and placement-success receipt to the
owner-bound native runtime; authentic non-Forge injectable client parity.
Do not merge on compile-only evidence.
