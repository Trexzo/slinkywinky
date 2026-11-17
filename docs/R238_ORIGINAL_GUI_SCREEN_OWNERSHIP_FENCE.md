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

## Verification

Pending exact implementation CI.
