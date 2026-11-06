# R99 — Persistent first-party menu model inside an original Minecraft 1.8.9 render callback

**State:** code pushed to `reconstruction/r1-core-fidelity` draft PR #1; locally validated with Windows MSVC Java 8/JNI/WGL (28/28 CTest), strict Python receipt regression (34 tests, one platform-specific skip, zero failures), and **a successful supervised real ORIGINAL vanilla 1.8.9 JVM live admission**. This is not a finished injectable ClickGUI or functional module implementation.

## The gap addressed

In R91–R94, every native menu draw built a brand-new `MenuState` and `MenuProfileBridge`. Those draws proved a first-party menu could appear on top of Minecraft without modifying GL state, but could **not** retain the selected row, tab, or local configuration across successive render callbacks. R96/R97/R98 provided actual navigation and safe click handling in the *standalone* Windows preview, but did not bridge that state-lifetime gap.

R99 adds `WglRetainedMenuPreview` in `include/slinkywinky/windows_retained_menu_preview.hpp` and `src/windows_retained_menu_preview.cpp`:

- The first explicit **owner-render-thread** draw constructs a `MenuState`, `MenuProfileBridge`, and `MenuNavigation`, which persist for the lifetime of the finite R99 admission. No GLFW, Forge, WndProc swap, input hook, Java game object manipulation, packets or combat actions are involved.
- Every draw uses the same R93 compact first-party GL11 renderer and the R96 visible-row focus accent, but creates and **closes a stack-scoped WGL host on that same original render thread**, preserving prior GL fixed-function state. The menu model (not the GL host) persists across frames. Wrong owner thread, HGLRC, HDC or viewport are rejected; an explicit fresh session is required for a changed graphics epoch.
- `apply(MenuNavAction)` is owner-thread-only, cannot act before the first validated draw, and refuses unsupported modules via the existing local `MenuProfileBridge`. It does not activate Minecraft features. The read-only test snapshot may expose counts, current tab, focused catalog index, and a single local preview flag but never raw WGL pointers or game addresses.
- Existing R94 manual show/hide/stop JNI exports and their disposable-per-frame menu continue to behave as before. All new retained JNI exports require an **independent** `nativeArmRetainedPreview` session: `nativeDrawRetainedPreview`, `nativeRetainedPreviewAction`, `nativeRetainedPreviewSnapshot` and `nativeDiscardRetainedPreview`. The model is lazily allocated on the admitted original WGL owner, not on the Java Attach worker. The separate disposal can run only after the original callback fence drains, with no outstanding WGL resources held by the model.

## New explicit original-only admission mode

`--render-retained=72` is accepted by the first-party Windows loader and Java 8 Attach helper only when `SLINKYWINKY_RENDER_RETAINED=1`, `SLINKYWINKY_RENDER_OVERLAY=1`, `SLINKYWINKY_RENDER_CONTEXT=1`, and `SLINKYWINKY_RENDER_CALLBACK=1` are explicitly set, and the target is the exact SHA-pinned original Mojang Minecraft 1.8.9 client. It is **not activated by default** and refuses Attach test-mode bypass.

The original render method `bfk.a(FJ)V` is source-pinned and source-verified, instrumented only with the independently audited at-RETURN callback, and **restored with R90 verification** before the admission lock can release. Exactly **eight** R78-style preflight frames are required, followed by **64** finite retained native GL11 draws. A bounded cleanup worker drains all callbacks, discards the retained model after the drain, and verifies original renderer restoration.

To prove persistence without introducing Minecraft keyboard/mouse handling, the original-game render thread runs **four deterministic, local preview-only model actions** on the first four admitted draws: move from catalog index 0 to 1, expand row 1, toggle row 1's local preview flag, and change from Combat to Move (focused row 17). After **every** draw, a native owner-thread JNI snapshot is checked for exact frame count, action count, tab, focus and profile flag. The final 60 frames must preserve the resulting Move-tab focus and local preview state. Any divergence or missing/duplicate receipt fails closed.

The sole R99 complete-source/owner/WGL receipt is `R99_RETAINED_MENU_DRAW_PASS frames=64 actions=4 tab=1 focus=17`, and it is required **after** original-class execution-shape restoration, stable owner/current WGL and full bounded callback acceptance and **before** the restored admission release. This marker establishes deterministic retained model behavior; **it does not show user-originated key or mouse control**.

## Verified local evidence

- Windows x64 MSVC Release full build: PASS. All **28/28 native/Java 8/Attach/JNI/WGL CTests** PASS, including the production JNI test that arms, draws, changes local UI state, draws again, rejects unknown actions or foreign-thread calls, ends, re-arms and discards a retained session on a disposable WGL context.
- Strict Python original-game receipt verifier: **34 tests run, 1 skip, 0 failures**, including forged/duplicate/reordered R99 receipts and cross-mode confusion with R92 or R94.
- **Real isolated original vanilla Minecraft 1.8.9 PID 8156**, supervised through six separately verified stages:
  1. Exact original renderer class preflight and admission release.
  2. Eight source-pinned original render callbacks and restoration.
  3. Eight read-only stable WGL context samples and restoration.
  4. R91 one-frame native overlay and restoration.
  5. R92 64-frame bounded overlay and restoration.
  6. **R99 retained 64-frame, four-action local navigation proof** and restoration.

Original target JVM stdout independently contained:

```text
R99_RENDER_CALLBACK_ARMED_LIMIT=72
R90_RESTORED_SOURCE_PINNED_EXECUTION_PASS
R99_RENDER_CALLBACK_BOUNDED_PASS accepted=72 owner=Client thread#1
R99_WGL_CONTEXT_STABLE_PASS attempts=72 current=72 absent=0 invalid=0 changed=0 stable=true frames=64 retained=true actions=4 tab=1 focus=17
R99_RETAINED_MENU_DRAW_PASS frames=64 actions=4 tab=1 focus=17
R99_RETAINED_LOCAL_PREVIEW_NO_GAME_INPUT_ACTIONS
R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS
```

The supervised launcher reported `R99_ORIGINAL_189_RETAINED_MENU_RECEIPTS_PASS pid=8156`. Its fresh target-log receipts and sanitized six-stage report are in the local disposable workspace at `vanilla189-isolated/r99-retained-receipts-8156.json`. The test ended its own vanilla JVM and the subsequent process inspection found **no Minecraft-like JVM**. No other game process was injected or stopped.

The separate machine-readable report deliberately remains `visual_game_acceptance=false` and `in_game_menu_verified=false`: the receipt script verifies actual original-JVM JNI WGL draws and navigation state, **not an independent screenshot** of the changing selected row in the visible game window.

## What remains

R99 establishes **native retained menu state with a source-verified original game render bridge**. It does **not** turn the original Minecraft overlay into a user-operated ClickGUI. Next, a **separately opted-in** command channel must dispatch bounded, validated owner-thread navigation and/or safe foreground-gated input, with each action positively attributed; the previously refused R95 OS Insert test cannot be silently bypassed. Before gameplay interaction, certify a no-click-through modal policy, focus/capture loss, safe original-bytecode unload, genuine in-game visual inspection, and feature-by-feature gameplay tests. The 58 unsupported catalog entries remain inert and no combat module was activated.

PR #1 must stay draft/unmerged until those gates pass. GitHub CI on the final documentation revision must be checked separately from the local Windows evidence.
