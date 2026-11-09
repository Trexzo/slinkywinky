# R187 — WGL binding-loss and original focus-pair fence

**Branch:** `reconstruction/r1-core-fidelity`, draft/unmerged PR #1. PR #2 (the separate OpenSlinky C++ comparison track) is not modified.

## Defect

R184/R185/R186 require source-matched GL11 paint geometry, cancel gestures after rejected paints, and reject physical `GL_VIEWPORT` drift after drawing. However, the **passive** native `observe_hover` and `source_hover_target` paths returned early on a missing or moved **actual WGL context/DC** without invalidating the previously painted source geometry. A later rebind of the same context could reveal that stale source target without another successful GL11 render. In addition, `observe_hover` previously checked `pointer_gesture_.pending()` before the live viewport check: a pending model press could hide a viewport change observed by passive hover.

The R183 companion focus sampler also returned false for a changed context/DC but did not actively revoke the render epoch or refuse an otherwise paired sample after a viewport mutation.

## Change

- `WglRetainedMenuPreview::observe_hover` rejects foreign threads before mutation. On the **real pinned owner thread**, it now revokes the last painted frame, cancels pending synthetic gestures, clears local hover hints and breaks R182/R183 focus chronology when the live WGL context/DC, declared original handles or exact viewport are inconsistent. Check actual WGL and viewport **before** pending-press logic. These rejected samples never increment foreground, numeric, source-hit or OS-focus counters.
- `source_hover_target` likewise revokes old geometry on a same-owner rejected WGL binding or viewport. Rebinding the previous WGL context/DC cannot restore stale hit rectangles; only a new successful GL11 draw can.
- `observe_original_window_focus` now checks the same live WGL owner and viewport **before** pairing a companion tri-state OS result with a previously observed passive source hover. A lost binding breaks the chronology; it is not treated as a newly observed OS-background state.
- Foreign-thread passive calls remain denied **without invalidating** the real owner's geometry. No new original Minecraft mouse/keyboard hook or gameplay dispatch is introduced.

## Isolated Windows verification

The native disposable WGL regression deliberately releases the current context with `wglMakeCurrent(nullptr,nullptr)` after a successful draw, queries the source hit rectangle, rebinds the original context, and confirms that no hit is available until another valid paint. A second run begins an inert slider press, loses WGL binding, attempts passive hover, restores the original context, and verifies that the old Up cannot mutate the source-validated native profile.

Additional cases: a foreign-thread passive hover cannot revoke valid real-owner geometry; a real GL viewport mismatch *during a pending press* cancels it; and a companion synthetic OS-focus status presented after a WGL/viewport loss cannot complete the preceding hover sample or create a false focus receipt. The final test also checks that the full isolated test's GL state is restored. These are **synthetic native fixtures**, not physical original-client input.

The R175 disposable Java 8/JNI hover regression previously expected a numeric source hit after deliberately submitting a wrong WGL viewport. R187 changes it to require a new actual native GL11 paint first. A pre-paint hover is explicitly counted EMPTY; the expected draw/sample totals reflect that additional safe observation. This preserves the original source-hover control behavior without reviving stale geometry.

## Original vanilla Minecraft 1.8.9 gate

A new noninteractive exact original-JVM test was run **after the R187 changes**, on 9 November 2026. It exited successfully and passed six isolated original-JAR restoration/admission stages, 64 retained native GL11 frames, 60 source-visible slider draws and 63 passive observations. Its independently logged R183 exact HWND/OS focus state was `foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0`, with R179 `exact_hits=0` and `native_click_delivery=disabled`. The original window was **background for all 63 samples**. The earlier R186 original-client acceptance also passed with the same background-only limitation. Neither run is a manual foreground acceptance or physical-hover proof.

A genuinely supervised terminal run remains required for the existing strict mode:

```powershell
$repo = 'C:\Users\Felix\AppData\Local\Temp\SlinkyWinky-R172-Chat'
$jdk = 'C:\Program Files\Eclipse Adoptium\jdk-8.0.504.1-hotspot'
& 'C:\Users\Felix\AppData\Local\Programs\Python\Python312\python.exe' "$repo\tests\windows_vanilla189_isolated_manual.py" --workspace $repo --jdk8 $jdk --loader "$repo\build-r184-clean\Release\slinkywinky_loader.exe" --run --render-retained-acceptance --require-foreground-source-hits --require-focus-cycle-source-hit --require-exact-os-focus-cycle
```

This command requires an actual interactive terminal and operator-driven focus/cursor changes during the short sample window. **Never** fake input or set focus automatically merely to obtain a pass. The manual result, even if successful, would validate repeated physical-window cursor observations and OS-focus chronology; it would **not** prove hardware origin, input exclusivity or live mouse-click safety.

Until independently verified, keep native click delivery disabled and PR #1 draft/unmerged.
