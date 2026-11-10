# R191 — connect R190 handoff and R108 keyboard routing to the rendered GL11 ClickGUI

**Target:** `Trexzo/slinkywinky`, `reconstruction/r1-core-fidelity`, open draft [PR #1](https://github.com/Trexzo/slinkywinky/pull/1). Separate OpenSlinky C++ comparison PR #2 is untouched.

## Why this integration matters

R190's paired keyboard/mouse input handoff ledger was an isolated state machine, and R108's keyboard router manipulated a separate first-party MenuNavigation. R159–R189's retained WGL ClickGUI provided a genuinely rendered and source-verified menu but did not have a keyboard router tied to that **same** native UI instance. Passing these three independent offline tests did not establish that the real native retained menu could respond to keyboard navigation without stale click geometry.

R191 ties a **caller-owned** `MenuKeyboardRouter` to the retained menu's actual `navigation_` with an explicit `fixture_routes(navigation_)` identity check, and adds a tightly scoped, **test-only** `fixture_begin_keyboard`, `fixture_keyboard_event`, and `fixture_end_keyboard` surface. The router is deliberately external, so the established `WglRetainedMenuPreview` object layout remains unchanged and existing disposable Java 8/JNI WGL fixtures retain binary compatibility. These methods deliberately are not callable through the original game JNI bridge or Java instrumenter. A new regression in the R119/R160 source guard scans both and would fail if any fixture entry point is wired into the real injected agent.

## Same-GUI WGL integration

The fixture session requires the R190 ledger in `MenuFixture`, matching strictly increasing epoch, the real pinned WGL owner thread and current context/DC/viewport, no pending synthetic mouse gesture and a painted retained frame. The keyboard router is constructed by the test from `fixture_navigation()` and **must** be identity-matched to this exact retained GUI before a synthetic key is admitted. Wrong-epoch, foreign-thread, wrong-menu router and incomplete two-stage handoff events are refused.

A fixture-only synthetic keyboard event is validated against both R190's keyboard state and R108's key dispatcher. Only if both accept can the router modify **the retained menu's own navigation**, rather than a separate dummy MenuState. The existing source LWJGL2 Tab key 15 changes the menu from Combat to Move in the disposable WGL test. A successful navigation immediately invalidates the previous painted frame and its source hit rectangles; another GL11 draw is required before mouse targeting. Profile settings remain unchanged by tab navigation.

During fixture close, old releases remain quarantined and R190 restoration requires the independent synthetic attestation that the original input path is restored. A subsequent higher-epoch session can begin after the previous model's clean drain. A real WGL viewport change while a key remains held revokes the ledger, denies the keyboard event and requires a new paint. Synthetic input never enters Minecraft.

## Verification

`tests/test_windows_retained_pointer.cpp` now tests initial state, incomplete handoff, foreign-thread refusal, real same-menu tab change, stale pointer-frame denial, subsequent GL11 repaint, native profile immutability, keyboard Up, release drain, denied restore, successful separately attested restore, stale epoch and a second session with viewport loss during a held key. An additional R160 source-only negative test ensures `jni_bridge.cpp` and every original Java agent source contain **no** R191 fixture handoff entry point.

Windows x64 Release build and **93/93 CTest** passed on the revised external-router implementation. The first member-router design produced actual heap-corruption and Java 8/JNI WGL test failures; it was **not shipped**. Moving the router outside the retained preview restored the original object layout, after which the previously failing JNI/WGL fixtures and the complete parallel suite passed. The independently launched original vanilla Minecraft 1.8.9 read-only acceptance also passed with six restoration/admission stages, 64 retained GL11 frames, 60 source-visible slider draw frames and 63 passive samples. The original-JVM Win32 focus state remained **foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0**, source exact hover hits=0, and live click delivery disabled.

This is a **native WGL integration regression**, not a UI screenshot parity test, hardware keyboard test, exclusive LWJGL event ownership, or an original Slinky binary behavior proof. The commercial Slinky ClickGUI and all gameplay modules remain incomplete.

## Strict outstanding acceptance

The latest R191 noninteractive original vanilla Minecraft 1.8.9 run passed 64 retained frames and 63 passive samples but had `foreground=0`, `background=63`, `exact_hits=0`. The strict original-window source foreground hover/focus-cycle test still needs operator-driven evidence. Actual mouse/keyboard event interception must not be activated without source coverage, independent game/GUI consumer suppression, hardware edge validation and bytecode restoration. Keep original native click dispatch disabled and PR #1 draft/unmerged.
