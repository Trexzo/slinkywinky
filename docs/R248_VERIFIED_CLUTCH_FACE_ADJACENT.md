# R248 — verified Clutch face-to-adjacent ownership (offline bridge)

Follow-on to R247 in draft PR #2. PR #1 unchanged; no live action path.

## Problem discovered

R229 Clutch read-only input accepted a caller-provided `adjacent`
coordinate. A ray hit's block and boolean top/side indication could
match while the purported adjacent block pointed somewhere else.
That is insufficient evidence to advance a placement target.

## Change

`ClutchClickInput.observed_face` defaults to -1 for the existing
legacy offline replay fixtures. When the caller supplies the actual
Minecraft `EnumFacing` ordinal 0..5, R248 uses the independent R247
timer/face evaluator, requires consistency with the top/side flag, and
recomputes the only adjacent coordinate permitted by that observed face.
Contradictions refuse the full event without mutating the native planner
or timer. Valid receipts remain value-only click *proposals*.

The existing unverified -1 path is retained for compatibility but must
not be treated as real-world placement evidence. A real game adapter
must provide a legitimate observed face rather than guessing one.

## Tests and limits

R248 extends `reference_compat/tests/test_clutch_bridge.cpp`
with contradictory top/side, forged adjacent, no-block, invalid face,
out-of-range CPS and correct UP/EAST coordinate examples. R247's
standalone timer/face regression remains intact.

This is **not** a module coverage increase, true game input/click,
inventory selection, block placement, physical gameplay verification
or original client injectable integration. Live JNI and original
Minecraft 1.8.9 world acceptance are still required.
