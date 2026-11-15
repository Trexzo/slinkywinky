# R221 — Isolate opaque accent from translucent-row blending

## R220 Windows CI evidence

Both PR #2 GitHub Windows Debug and Release produced this exact
matrix in a first-party, disposable 676x630 WGL fixture (R220):

```
R220_BLEND=0 DITHER=0 ACCENT=F35D12 DARK=101014
R220_BLEND=1 DITHER=0 ACCENT=F05B10 DARK=0A0A0D
R220_BLEND=0 DITHER=1 ACCENT=F35D12 DARK=101014
R220_BLEND=1 DITHER=1 ACCENT=F05B10 DARK=0A0A0D
```

This proves that blending, **not dithering or RGB bit-depth**, can
independently reproduce the actual failing R207 GUI pixels on the
GitHub runner. The R219 probe reported 8/8/8 RGB and the standalone
opaque accent under disabled blending was exact.

The native renderer and the UI-owned R207 and R209 tests are NOT
changed here, and the production renderer is NOT declared fixed.

## R221 candidate remedy test

The separate standalone WGL probe keeps
`glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA)` **enabled for
translucent row background**, then draws the fully opaque accent
with one of these independently measured choices:

- Mode 0: unchanged standard SRC_ALPHA blend (control)
- Mode 1: GL_BLEND enabled; GL_ONE / GL_ZERO for opaque accent
- Mode 2: GL_BLEND disabled for opaque accent

It reads the same accent and row background pixels on the private
backbuffer and emits `R221_OPAQUE_MODE=... ACCENT=... DARK=...`.
All three have the same row draw prior to accent.

Local Windows source-exact MSVC Release built and **PASS**;
all three accents F35D12 and dark rows 0B0B0E. The GitHub
Windows runner result is pending.

If GitHub mode 1 or 2 preserves exact F35D12 while maintaining
its unchanged blended dark row, PR #1's GUI owner has a minimal
code-level strategy to test in an isolated source branch, with
explicit GL state restore and existing negative controls. A
remaining slight dark-row readback difference is implementation
specific and should be tested with a separately grounded
blended-color oracle rather than broad thresholds.

This R221 change adds no live hooks, modules, JNI state
transitions, source imports, production UI modifications, or
new original-client fidelity claims. Keep PR #2 draft/unmerged
and the PR #1 GUI owner's code independent.
