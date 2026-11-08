# R176 — actual original Mojang 1.8.9 retained-render hover receipts

**Repository:** `Trexzo/slinkywinky` · branch `reconstruction/r1-core-fidelity` · draft PR #1. OpenSlinky C++ integration stays in separate draft PR #2.

## Actual original vanilla 1.8.9 acceptance

The existing supervised `tests/windows_vanilla189_isolated_manual.py --run --render-retained-acceptance` launched a **fresh, separate original Minecraft 1.8.9 Java 8 client** on Windows, using the SHA-256-pinned vanilla game JAR, a sandbox game directory and an offline test identity. It did not attach to another existing Minecraft process, run a gameplay module or alter the installed original JAR. Its own process was terminated after acceptance.

The baseline pre-R176 client run confirmed the original R99 bounded retained GUI sequence, including **64 original-game WGL frames**, four local-only menu navigation actions, exact original-class restoration and release of the renderer gate.

After adding the R176 receipts, an independent **second original-client** run also passed. Original-JVM log contained exactly:

`R176_ORIGINAL_WINDOW_PASSIVE_HOVER_RECEIPT_PASS frames=64 samples=63 foreground=0 numeric=0 bind=0 native_click_delivery=disabled`

The subsequent loader-side independently verified:

`R176_ORIGINAL_VANILLA_189_HOVER_COUNTERS_PASS samples=63 foreground=0 numeric=0 bind=0`

The same original JVM produced `R99_WGL_CONTEXT_STABLE_PASS ... frames=64 retained=true actions=4 tab=1 focus=17`, `R90_RESTORED_SOURCE_PINNED_EXECUTION_PASS`, and `R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS` after the retained menu's source-validated final observation.

**Interpretation:** the actual original game called the new first-party native passive hover code **63 times** over the 64 retained menu draws and returned a source-bound JNI receipt on the original render owner. Because the isolated game remained **backgrounded for all 63 hover observations**, this run proves only that the input-free hover path and clearing behavior ran successfully. It does **not** verify a physically foreground cursor on a slider, an actually displayed tooltip, input interception, simultaneous click suppression, live gameplay-module behavior or 1:1 fidelity to the commercial Slinky client.

## Production C++ and JNI changes

- `WglRetainedMenuPreview::observe_hover()` now monotonically counts **accepted hover samples**, those on an admitted foreground window, those that hit an original recovered numeric control and those that hit a bind control. No raw cursor coordinates, mouse button data, account information or original-game input are logged.
- `hover_evidence(context, dc, width, height)` is read-only. It rejects nonowner threads, changed WGL context/DC or viewport and counters that violate `numeric + bind <= foreground <= samples`.
- `RendererWglEvidence.nativeReadRetainedHoverEvidence` returns exactly five integer counters, with a strict integer-range boundary. The original 64-frame renderer reads the evidence on its *actual game render thread*, before restoring the original class and closing the WGL menu.
- Java 8 `validRetainedHoverEvidence` requires `frames=64`, `samples=63`, nonnegative and mutually consistent counts. `retainedVerified()` now requires this provenance check in addition to the existing GL, GUI and source-class validations.
- `RendererFrameDiagnostic` prints the concise aggregate-only R176 marker when the existing R99 original-render, class restoration and cleanup conditions all pass. There is **no click dispatch or newly enabled mouse hook**.

## Testing

- `R175RetainedHoverJniSmoke` now reads the real C++ WGL hover counters through the **disposable** JNI DLL and verifies default zero, a source-exact numeric slider hit, repeated identical hover samples, native WGL redraw, foreign-thread refusal and after-close refusal.
- `r176_java8_original_retained_hover_receipt` checks valid and invalid counter shapes, including missing, duplicate, negative, overflowing or internally inconsistent samples.
- `r176_original_client_hover_marker_negatives` checks strict original-JVM marker format; rejects fabricated suffix/prefix, duplicates, non-64-frame receipts, impossible counts and any suggestion that click delivery was enabled.
- The original standalone launcher validates the one exact full-line R176 marker in the fresh JVM stdout, after its independent R99 source-hash, WGL, restoration and release acceptance. **No such marker is trusted in isolation.**
- Test fixture classes remain outside both the agent JAR and the original-class audit JAR. The disposable R166 JNI DLL remains separate from the production JNI library.
- All Windows Debug and Release CTest suites must pass before the commit is published, and four existing GitHub CI workflows must be checked afterward.

## Remaining blocker

A **controlled foreground, authentic Minecraft 1.8.9 hover test** must actually position the cursor over a source-validated numeric or bind target and independently confirm the visible tooltip, without hijacking an unrelated desktop session. That cannot be inferred from this backgrounded run.

Exclusive original mouse/keyboard ownership, source-wide input suppression, live release and class restoration under a held click, genuine native ClickGUI click delivery and 67 gameplay modules are **still not complete**. Keep draft PR #1 unmerged.
