# R238 — Exact original GuiScreen ownership fence for retained native ClickGUI

Date: 2026-11-17. Chat 1 original-client reconstruction on draft PR #1, not PR #2.

## Discovered gap

R237 observes source-matched physical button-state transitions over the native Auto Block Range control on the original WGL render owner, but a separate vanilla Minecraft `GuiScreen` (inventory/options/main menu) could still be open. The presence of the native overlay does not give it exclusive mouse input. Even a perfectly matched source-target press/release must not be interpreted as eligible native input while vanilla GUI is active.

## R238 correction

- Reuse the existing source-verified `OriginalGuiScreenProbe`: it reflects only the pinned already-loaded `ave.A().m`, with the exact source-loader and render-thread checks. Enable this mandatory probe in R99 retained-preview mode as well as existing R101 window-audit mode.
- The original retained Java callback now samples GuiScreen presence on **every** admitted renderer draw and passes an explicit tri-state status to the original native retained-render JNI call. A failed, absent or wrong-thread probe does not silently become NO_SCREEN: R99 fails closed. The external operator control path passes `REFUSED` explicitly because it does not have source-verified screen ownership.
- R237's real original-render pointer edge trace now takes an independently sampled original-GuiScreen-absent flag. If vanilla GUI is open or its presence is uncertain, an in-progress native source press is cancelled, clean-release arming is reset, and no matched pointer pair can be counted. The original native overlay may still render as read-only GL11, and Java's operator controls continue to work.
- The screen bit is **only an observation**; it never suppresses a vanilla handler, installs a hook, captures LWJGL2 events, or grants live mouse/keyboard ownership. Input delivery and suppression are permanently disabled.

## Coverage

The new R238 Windows C++ regression checks GUI appears while source left button held, cancellation, blocked initially-held input, recovery after a new clean release, and all corresponding exact aggregate counters. It runs beside all R237 source-owner and WGL/renderer tests.

### Outstanding gate

A genuine original Minecraft physical-input handoff requires a verified reversible transformation of **every relevant vanilla input consumer**, plus control of the original GuiScreen and focus lifecycle. R194/R195 found many possible input consumers, so replacing a single LWJGL queue call is not sufficient. The connected Windows desktop is offline, so no new real-original hardware-input acceptance is claimed. PR #1 remains draft and unmerged.

## Verification — GitHub Actions

Corrected R238 implementation/test commit `da342cbb2de8beff5174657936f3823206bfeaee` is **8/8 PASS**:

- Native C++ Debug/Release on Windows and Linux, Linux sanitizers and JNI smoke: **6/6 success** — https://github.com/Trexzo/slinkywinky/actions/runs/44068459192
- Windows native JNI bridge build/roundtrip: **success** — https://github.com/Trexzo/slinkywinky/actions/runs/44068459190
- Windows Java 8 standalone native loader and actual Attach API: **success**, including the previously failing R209 reflective retained-menu/ArrayList smoke — https://github.com/Trexzo/slinkywinky/actions/runs/44068459333

The R238 **original-client-class-audit** also passed on the preceding implementation commit `fd792b07849a796c1b21785ca3292cc9fb967bfe`: https://github.com/Trexzo/slinkywinky/actions/runs/44067907648. That earlier standalone-loader run failed only because the existing R209 disposable JNI smoke still requested the old four-argument method signature. The follow-up `da342cb` corrected the smoke to use the new explicit fifth GuiScreen-state argument, with UNKNOWN (-1) in the disposable test, and reran the full 8-check matrix successfully. **Only test code changed between these two commits.**

These are build, offline source/ABI audit, and disposable test results, not a fresh original Minecraft runtime session with physical mouse/keyboard. The original-client Windows desktop connection was offline. Real live input ownership, vanilla event suppression and reversible game-handler restoration remain unverified and disabled. Keep draft PR #1 unmerged.
