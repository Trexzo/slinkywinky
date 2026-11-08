# R167 — Java 8 recovered slider → native validated profile

**Repository:** `Trexzo/slinkywinky` · draft PR #1, branch `reconstruction/r1-core-fidelity`.

R166 proved Java 8 → JNI → actual first-party retained C++ WGL ClickGUI mouse-button ownership and profile toggles in an isolated Windows window, without any Minecraft input interception. **R167 extends that exact same disposable JNI test DLL** to the source-recovered `Auto Block / Main / Range` slider.

## Actual UI integration exercised

The original R166 test-side Java native declarations are made package-visible so a separate R167 Java class can reuse exactly the same test-only WGL owner and pointer JNI functions; no runtime JNI method or injectable Java agent is changed.

Two test-only JNI exports in `tests/r166_retained_pointer_jni.cpp` provide:
- `navigate(int)`: only the explicit `NextRow` (2) and `ExpandFocused` (4) actions, on the already bound WGL owner thread, modify the real retained native menu. Unrelated navigation is rejected by the test boundary.
- `sliderTarget(int fractionPermille)`: derives a visible, source-indexed slider target from the same `MenuRenderSession`, `recovered_widget_catalog`, `menu_numeric_track_geometry` and `hit_test_menu` as the real C++ renderer. It matches exact module `Auto Block`, group `Main`, label `Range`, type `Slider`, visible catalog row 1, and refuses invalid fractions, wrong thread, missing WGL owner, unsupported setting and off-panel coordinates. The target routine only supplies physical client pixels; the real menu's `WglRetainedMenuPreview::pointer()` handles all state changes.

The Java 8 smoke enters an isolated WGL window, moves focus to Auto Block, expands it, resolves slider coordinates, and performs real pointer-down / pointer-up transactions. The profile starts with native `block_range=3.1`. A midpoint source value yields a validated value near 4.5; the profile stays changed across redraw. A high source value (source UI permits up to 8) exceeds the native engine upper limit 6, so the release must be refused **without mutating the native profile**. A low value is accepted and quantized. Cross-thread commands, missing synthetic exclusive ownership/foreground, wrong viewport, cancelled transactions and an invalid source range are all denied or cancelled, preserving the last valid setting.

The independent R166 toggle integration continues passing. `r167_java8_jni_recovered_slider_to_native_profile` is a new Windows/CMake/Java 8 test and uses **only** `slinkywinky_r166_pointer_test_jni.dll` under the `r166_disposable_only` output directory. Java test classes are excluded from both the injected `slinkywinky_instrument.jar` and the read-only original-source audit JAR. A production-input source guard also rejects references to the new R167 Java class.

## Limits

This is a **real first-party C++ GUI settings mutation through a disposable JNI/WGL fixture**, not an original Minecraft 1.8.9 mouse-input hook. The fixture's exclusive/foreground flags are synthetic and **do not provide runtime input-ownership proof**. No original game code, target JVM, packets, movement or combat actions are modified by R167. The 67 recovered module names are not 67 complete game modules.

Next, an authenticated original-game input ownership and reversible loaded-class restoration must be verified before the JNI pointer path can be connected to actual Minecraft mouse events. The PR remains draft and unmerged.
