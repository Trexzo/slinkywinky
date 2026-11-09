# R181 — strict foreground source-hit admission (manual original-client gate)

**Branch:** `reconstruction/r1-core-fidelity`, draft PR #1. OpenSlinky C++ integration PR #2 is untouched.

## Reason for the new gate

R179 demonstrated that the first-party C++ Auto Block / Main / Range
slider had a real native GL11 pixel rectangle in **60 of 64** render frames
of a freshly launched, original Mojang Minecraft 1.8.9 client. But the
original process was in the background for every passive sample:
`foreground=0 numeric=0 exact_hits=0`. That is source geometry evidence,
**not** evidence of foreground cursor hover.

R180 fixed a related stale-frame defect: navigation cannot expose an
unpainted source hit rectangle before the next successful WGL draw.

R181 now adds a deliberately stricter **optional** acceptance gate, without
hooking the game's input queue or moving the operator's pointer.

## New opt-in

In the existing manually supervised isolated-vanilla acceptance command, add:

`--render-retained-acceptance --require-foreground-source-hits`

The full command still requires its existing `--run`, `--workspace`,
`--jdk8` and exact optional `--loader` arguments. All prior original
Minecraft 1.8.9 JAR/class fingerprint, isolated process, unique PID,
JNI/WGL owner, original bytecode restoration and no-click admission remain
mandatory. An interactive terminal is required for this explicit opt-in;
a noninteractive CI process and other acceptance modes cannot request it.

The supervised runner prints a **three-second passive countdown**
before starting the finite renderer trace, giving the operator a chance to
focus the isolated game window. The operator must position the cursor over
the visible **Auto Block > Main > Range** slider while the 64-frame
passive test runs. The test does not synthesize cursor movement, inject
clicks or switch window focus; the countdown does not guarantee an
actual foreground observation.

The strict verifier rechecks all three exact original-JVM receipts:

- R176: 64 frames / 63 passive samples, consistent foreground/numeric/bind counts.
- R178: the source slider is eligible in precisely 60 source frames, with
  the same original foreground count.
- R179: the exact native source rectangle has matching per-frame foreground
  hits, and `native_click_delivery=disabled`.

It requires at least **two foreground/numeric/exact-source hit samples**,
with internally consistent bounds. A single stray hit, background-only
run, false marker, duplicate marker, or inconsistent counters fails closed.
When successful the supervised runner prints
`R181_ORIGINAL_VANILLA_FOREGROUND_SOURCE_HITS_PASS` with numeric counts.
The pre-existing unattended R179 diagnostic behavior remains unchanged.

## What this does and does not prove

A successful R181 manual acceptance demonstrates **repeated read-only
Win32 cursor observations while the source-matched native slider was
visible and the isolated original Minecraft window was foreground**.
The samples are not guaranteed to be consecutive; the test does not
verify whether cursor motion originated from physical hardware. It does
not prove a deliberate focus-loss/reacquisition sequence, actual tooltip
pixels in the original game, exclusive LWJGL input ownership, suppression
of vanilla clicks, a playable injector, or commercial Slinky 1:1 parity.

The Windows Desktop Commander device was **offline** when the R181 code
was authored. Consequently a live foreground acceptance result has NOT
been obtained by this change. Public CI verifies only synthetic parser
negative controls and the unchanged build/test suite.

## Synthetic verification

`tests/test_r181_original_foreground_source_hits.py` exercises the strict
source-marker verifier with repeated source hits and 25+ negative cases,
including the historical R179 zero-foreground condition. The new CTest
`r181_original_client_foreground_source_hit_negatives` is registered only
when the existing Windows Java 8 Attach/CI configuration is enabled.

No proprietary commercial source, decrypted binary or asset has been added.
Keep PR #1 draft; do not enable original Minecraft live click dispatch.
