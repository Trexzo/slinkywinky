# R186 — post-paint GL viewport continuity for the retained ClickGUI

**Scope:** SlinkyWinky native Windows x64 ClickGUI reconstruction; `reconstruction/r1-core-fidelity`, draft PR #1. PR #2 and the C++ OpenSlinky comparison track are unchanged.

## Defect

R184 bound model pointer actions and passive source hover to the last successful GL11-painted frame. R185 invalidated that source geometry after a **rejected render attempt**. Neither rule checked the actual `GL_VIEWPORT` at the **time of a later pointer or hover operation**. The original renderer can change the viewport after a successful paint without changing its WGL DC/context or calling the retained menu `draw` again. A caller claiming the old width/height could therefore use the previous visible source hit rectangle after the graphics state had changed.

## Implementation

- Use a read-only `glGetIntegerv(GL_VIEWPORT, ...)` check on the already pinned WGL render owner to require the exact `[0,0,width,height]` viewport. Never infer this from user-supplied dimensions.
- On a mismatch observed by the inert local pointer model, passive hover, or source-target lookup, invalidate the prior GL11-painted frame, cancel the pending synthetic press, clear local hover hints, and break both R182 passive focus and R183 exact OS-focus chronology **without inventing focus samples**.
- Restoring the GL viewport alone does not restore the old hit rectangle. Only a **new successful retained GL11 draw** may create another model target.
- Reuse the same owner-only invalidation routine for R185 rejected paints and R186 post-paint viewport drift, avoiding distinct partial cleanup paths.
- The R166/R168/R169 disposable Java 8 JNI tests now explicitly refuse clicks before the necessary repaint after wrong-viewports, then verify that subsequent intended Boolean, dropdown and profile operations work after redraw.
- A foreign thread is still refused before it can alter the source WGL owner. This does not add an input owner.

## Original vanilla 1.8.9 acceptance, 9 November 2026

A noninteractive, isolated original Minecraft 1.8.9 `--run --render-retained-acceptance` was executed with the rebuilt native loader and pinned installed Mojang JAR, **not** Forge or a custom simulation. The initial run exposed a pre-launch Python defect: assigning a local variable named `os` in `windows_vanilla189_isolated_manual.py::main` shadowed the imported Python `os` module before argparse defaults evaluated. The local variable and its six receipt references were renamed to `os_counts`. The R183 negative fixture now exercises the actual CLI `--help` entrypoint without launching Minecraft to prevent this regression.

The subsequent isolated original-process run exited successfully with six receipt stages and original-JVM markers:

- `R99_ORIGINAL_189_RETAINED_MENU_RECEIPTS_PASS`
- `R176_ORIGINAL_WINDOW_PASSIVE_HOVER_RECEIPT_PASS frames=64 samples=63 foreground=0 numeric=0 bind=0 native_click_delivery=disabled`
- `R179_ORIGINAL_NATIVE_SOURCE_TARGET_PASS frames=64 source_draws=60 exact_hits=0 native_click_delivery=disabled`
- `R182_ORIGINAL_NATIVE_FOCUS_CYCLE_PASS frames=64 losses=0 regains=0 post_hits=0 native_click_delivery=disabled`
- `R183_ORIGINAL_EXACT_WINDOW_OS_FOCUS_PASS frames=64 foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0 native_click_delivery=disabled`

The exact game window stayed **background** during all 63 passive samples. No foreground source hover, physical input, source-hit focus cycle, exclusive click owner, or real mouse down/up is established by this run. The new viewport-drift negative cases are proven by the disposable WGL fixtures, not by forcing an OpenGL viewport change on the original game.

## Verification and limits

The native disposable Windows WGL test changes actual GL viewport origin, width and height **after** a successful paint and confirms source-target denial; checks passive hover refusal without a numeric hover leak; denies a stale Down; and denies a held press released after a viewport drift, preserving the previous settings profile. The R166, R168 and R169 Java 8/JNI source navigation fixtures cover the stricter repaint requirement.

These are **disposable WGL/JNI test fixtures**, not a successful hardware-cursor interaction with an original Minecraft 1.8.9 foreground window. A strict original-client test still requires independently supervised physical foreground mouse and focus observations and an exclusive/reversible LWJGL input owner. No original Minecraft click dispatch, keyboard injection or gameplay module activation is enabled by R186. PR #1 remains draft.
