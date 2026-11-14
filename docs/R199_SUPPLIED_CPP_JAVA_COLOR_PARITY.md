# R199 — verified color-picker state against user-supplied Java and C++ archives

## Exact inputs inspected, without executing any native code

Both files were supplied directly by the user on 14 November 2026. Their contents were read from ZIP members; neither a DLL, injector nor original game was launched. No original source archive, proprietary asset or binary is committed to this repository.

| Input | SHA-256 | Examined entries |
|---|---|---|
| C++ `slinky-main (1)(1).zip` | `073b77d2bb77ce5cccf9eff57aa940eb558d425083cae4acd744a837d9e228b1` | `slinky/src/data/data.s`, `slinky/src/engine/gui/gui_popups.cpp` |
| Java `OpenSlinky-main(2).zip` | `364846830fcc3820b64eafe3ebd0639f71e9dafcf06d6f9fad20170f278afd6c` | `openslinky/SlinkyClient/src/main/java/dev/slinky/client/gui/GuiMath.java` and `ClickGuiScreen.java` |

Both archive versions independently supported the same two R199 corrections:

1. **Eight black history slots at startup, not eight absent values.** Native C++ assembly declares `gui_popups_recent_colour_ring_position: .zero 16` and `recent_colors_ring: .zero 96`. The latter corresponds to eight 12-byte RGB float triples. Its popup code checks all eight slots for an exact color match, including the initially black entries, before shifting its ring index. The Java source likewise declares `float[][] history = new float[8][3]`, which initializes all entries to RGB black, and `historyIndex` starts at zero. The R198 preview incorrectly modeled an uninitialized history as eight empty `std::optional` values, causing black deduplication and display to diverge. R199 models the history as an eight-element *zero-filled* RGB ring and checks black selection/duplicate handling with always-active tests.
2. **Independent HSL float state.** Native `gui_popups.cpp` stores `popup_hsl.h/s/l` separately from `popup_rgb`; the Java picker stores distinct `rgb` and `hsl` arrays. The R198 preview recomputed HSL from 8-bit RGB on every `hsl()` query, losing the slider's fractional position after quantization. R199 stores independent `MenuPopupHsl hsl_`, preserving the precise HSL sliders when `set_hsl` succeeds, and resynchronizing HSL from RGB when opening, accepting a hex edit or selecting a recent swatch.

## Code changes

- `include/slinkywinky/menu_color_popup_preview.hpp`: source-accurate initial recent-color slots; independent HSL floats updated on the appropriate operations. The model stays outside WGL/JNI/Minecraft and has no callback to the game.
- `tests/test_menu_color_popup_preview.cpp`: Release-active tests for eight initial black swatches, black deduplication, ability to select the black swatch, correct post-selection HSL, preserving fine-grained HSL values across RGB byte rounding, recomputing HSL after a hex edit, color conversion, history rollover and duplicate handling.

## Executed verification

A read-only Python ZIP inspection verified **7/7 source facts** from the **exact user-supplied files**, including both independent HSL stores, two history arrays and native ring initialization: `R199_UPLOAD_SOURCE_PARITY_EVIDENCE_PASS`.

The updated pure C++17 model and its complete regression were compiled and executed **twice** in an isolated local Linux environment with GCC 14.2:

- `g++ -std=c++17 -DNDEBUG -O2 -Wall -Wextra -Wpedantic -Werror ...` — `R199_CPP17_RELEASE_BLACK_SELECTION_AND_HSL_PASS`
- `g++ -std=c++17 -O0 -g -Wall -Wextra -Wpedantic -Werror ...` — `R199_CPP17_DEBUG_BLACK_SELECTION_AND_HSL_PASS`

No Windows/MSVC or full-project CTest execution is claimed for R197–R199. The authorized Windows PC was **offline**. The existing `r198_native_color_popup_hex_hsl_recent_fixture` CTest target exercises these extra R199 assertions when Windows testing resumes.

## Important unfinished work

R199 is **a reference-grounded correction to a portable popup state model**, not a rendered, interactable original Slinky color modal. It does not implement the six-gradient hue bar, saturation/lightness bars, exact popup geometry, in-game text edit/keyboard routing, swatch click routing, real keybinds, profile persistence, or the 66 native module behavioral paths. Likewise, float-to-byte rounding is a first-party approximation pending original arithmetic acceptance, and the model currently rejects opening a second popup until the first is explicitly closed whereas the C++ reference auto-closes the existing popup.

The source archives are candidate reconstructions, not independently signed authentic Slinky releases; their agreement increases confidence but cannot prove entire-client 1:1 fidelity. PR #1 stays **draft**, PR #2 stays independent, and live Minecraft input handling remains disabled.
