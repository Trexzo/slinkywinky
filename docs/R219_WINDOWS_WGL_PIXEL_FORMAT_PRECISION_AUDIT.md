# R219 — Windows WGL pixel-format precision audit

## Evidence and motivation

The R218 read-only P6 capture on GitHub Actions Windows (Debug and Release)
at PR #2 commit `bbbfd1f` returned the following **actual framebuffer** values:

| GL coordinate | Local Windows | GitHub Windows |
|---|---|---|
| (666,612) ArrayList accent | F35D12 | F05B10 |
| (666,590) ArrayList accent | F35D12 | F05B10 |
| (662,612) row background | 0B0B0E | 0A0A0D |
| (4,628) clear background | 000000 | 000000 |

The reproducible variation strongly suggests pixel-format/channel-depth
quantization. It is not evidence that native No Slow gameplay or the whole
ArrayList render is missing. The hypothesis is **unconfirmed** until a
GitHub runner reports its actual WGL color bits.

## Independently compiled format probe

`tools/r219_wgl_probe/` is a separate Win32 C++17/CMake target that
reproduces the existing R67 disposable WGL fixture's
`ChoosePixelFormat` request (PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
PFD_DOUBLEBUFFER, RGBA, requested 24 color bits, 16 depth bits).
It creates its own invisible 676x630 window, reads `DescribePixelFormat`
and `glGetIntegerv(GL_RED_BITS / GREEN_BITS / BLUE_BITS / ALPHA_BITS /
DEPTH_BITS)`, paints one tiny orange swatch on its private backbuffer,
then samples that swatch. The process does not reference Minecraft,
injector, launcher, input hooks, running user processes or commercial
assets.

The existing PR #2 `.github/workflows/core.yml` runs this build/execution
**only if a Windows CTest has already failed**, after the R218 P6
diagnostic. It does not change test outcome, baseline color assertions,
other branch code or user-facing behavior.

## Local Windows validation

MSVC 19.51, standalone CMake Release build: **PASS**.
The disposable context emitted:

```
R219_WGL_PFD_INDEX=20 COLOR_BITS=32 RGB_BITS=8,8,8 ALPHA=8 DEPTH=16
R219_WGL_GL_BITS=8,8,8,8 DEPTH=16
R219_WGL_REQUESTED_ACCENT=F35D12 DRAWN_ACCENT=F35D12
```

The local source-exact R207 WGL test also **PASS** with exact accent
F35D12, as described in R218.

## Pending

- Check the GitHub runner's R219 output in both Debug and Release.
- If it reports lower-precision RGB, PR #1 GUI owner should make its
  WGL/JNI pixel readback assertions precision-aware, preserving structural,
  visibility, order, and negative tests.
- If GitHub reports 8/8/8, investigate compositing, dither, blend state
  and OpenGL implementation differences before changing assertions.
- PR #1 was informed via comment; this commit changes **only PR #2**.

R219 does **not** increase the independently implemented modules
(16) or callbacks (22 / 171) and does not establish original-client
fidelity or production injection. Keep PR #2 draft and unmerged.
