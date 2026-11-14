# R220 — Windows WGL blend/dither differential

## Finding from R219 CI

At PR #2 source head `6ee777f`, the independent disposable
format probe passed on both Windows GitHub configurations and reported:

```
R219_WGL_PFD_INDEX=4 COLOR_BITS=32 RGB_BITS=8,8,8 ALPHA=0 DEPTH=16
R219_WGL_GL_BITS=8,8,8,0 DEPTH=16
R219_WGL_REQUESTED_ACCENT=F35D12 DRAWN_ACCENT=F35D12
```

The original disposable ArrayList render still produced `F05B10`
instead of `F35D12` on GitHub and `0A0A0D` instead of
`0B0B0E`. **Actual framebuffer channel-depth reduction is not
the explanation**: both local and GitHub report RGB 8/8/8.

The R219 probe explicitly disabled GL_BLEND and GL_DITHER, while
the real ArrayList renderer uses
`GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA` for a translucent
background and opaque accent. This difference required a separate
controlled reproduction rather than guessing a fix.

## R220 isolation

The read-only standalone disposable WGL probe now draws a
small background row `rgba(0x101014,180)` and an opaque
`rgba(0xF35D12,255)` accent, with all four combinations of:
- GL_BLEND disabled / enabled
- GL_DITHER disabled / enabled

The probe reads GL RGB bytes separately for a row and the accent.
It runs inside the existing **failure-only** Windows
CI diagnostic step and does not alter test assertions, renderer
behavior, GUI-owned code, JNI callbacks, or game processes.

## Local exact-source evidence

MSVC Release build and probe: **PASS** on the connected Windows PC.

```
R220_BLEND=0 DITHER=0 ACCENT=F35D12 DARK=101014
R220_BLEND=1 DITHER=0 ACCENT=F35D12 DARK=0B0B0E
R220_BLEND=0 DITHER=1 ACCENT=F35D12 DARK=101014
R220_BLEND=1 DITHER=1 ACCENT=F35D12 DARK=0B0B0E
```

The difference between dark-background values matches the
expected 180/255 alpha blending on this local implementation.
The opaque accent remains exact locally.

## Decision gate

Wait for the **actual** GitHub runner's four outputs.
If a specific blend/dither combination reproduces
`F05B10` and `0A0A0D`, the PR #1 GUI owner can address
that rendering-state-dependent precision while keeping
actual pixel/visibility and negative regression coverage.
If it does not, inspect differences in state restoration,
alpha-test, alpha scaling, and framebuffer readback without
loosening assertions on a hunch.

PR #2 remains draft and unmerged; original-client fidelity,
production injection and module coverage are unchanged.
