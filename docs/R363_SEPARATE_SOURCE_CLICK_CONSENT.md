# R363 — Explicit and independently authorized click-exit source diagnostic

R362 added a read-only ASM clickMouse normal-return callback to
the original standalone source diagnostic. R363 separates its opt-in
from the older R359/R360 tick-only mode so operators cannot
accidentally enable a new attack-input observer with an old command.

Original finite loader options:
- `--source-tick-readonly[=600]` keeps R360's conservative independent
  player-tick timestamp inference and native event18 sampler.
- **New:** `--source-click-readonly[=600]` uses the SHA-pinned
  `ave.aw()V` original Minecraft clickMouse normal-return observer
  and `SourceClickAttackFence`, which replaces R360's weaker
  tick-only event15 inference in that session.

Both require `SLINKYWINKY_EXPERIMENTAL_HOOK=1` and
`SLINKYWINKY_SOURCE_NATIVE_READONLY=1` in the operator's attach
environment. The click mode additionally requires **independent**
`SLINKYWINKY_SOURCE_CLICK_READONLY=1`; 600 ticks also
requires `SLINKYWINKY_EXTENDED_DIAGNOSTIC=1`.
Exact CLI spelling is enforced; all unauthorized or synthetic
Attach-test JVM requests are denied, even with every consent set.

Agent options `minecraft-source-readonly` and
`minecraft-click-source-readonly` remain distinct. The new
`ave.aw()V` class transformer only applies to the latter; the
pinned SHA-256 `ave` gate and one-shot instrumentation ownership
are unchanged. Neither diagnostic alters on-wire packet flow,
input state or entity position. Both are bounded to 20 or 600 ticks
and explicitly restore their original instrumented class bytes.

R363's `tests/attach_smoke.py` executes negative controls with
missing click consent and against a fake JVM despite full consent.
No original game bytes are committed.

Original Slinky `PhaseHook` timing, successful combat hit proof,
actual packet ownership and full GUI/mod parity remain unverified.
PR #2 draft, PR #1 unchanged.
