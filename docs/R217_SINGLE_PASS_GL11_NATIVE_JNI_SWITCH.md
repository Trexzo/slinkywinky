# R217 — one-pass GL11 Sprint rendering with native-authoritative JNI state

**Date:** 2026-11-14
**Owner:** Chat 1 / draft PR #1 (`reconstruction/r1-core-fidelity`). Chat 2/PR #2 left untouched.

## Problem and design

R216 displayed the real PR #2 native-acknowledged Sprint ON/OFF state, but its isolated ForgeDev JNI adapter first painted the retained preview ClickGUI and then drew another entire GL11 menu from an authoritative source-frame copy. That duplicated menu drawing each frame and could invalidate the benefit of the original Minecraft render boundary.

R217 replaces that two-stage rendering with **one GL11 draw per successful Minecraft-owned GuiScreen frame**, following **one extra initial source-navigation draw**. The initial paint is still required to initialize and navigate the real retained GL11 menu to the source-identified Sprint row; afterward every render boundary paints exactly once.

The R217 render-only value crosses a narrow API in `WglRetainedMenuPreview::draw(...,std::optional<bool>)` and `WglMenuOverlayHost::draw(...,std::optional<bool>)`. The existing four-argument API remains intact; native session source and JNI ownership are unaffected. `WglMenuOverlayHost` takes one first-party `MenuRenderFrame` snapshot, verifies compact overlay, Move tab, and the canonical Sprint index 25/title, overrides **only the frame copy's display flag**, then calls the original first-party `draw_menu_gl11` once. It does **not** write back to `MenuState`, `MenuProfileBridge`, saved settings, native module manager, or gameplay.

The admitted Forge JNI bridge obtains a strictly verified single-module `ModuleArrayListFeed` source snapshot (sequence, generation, canonical Sprint, inactive in no-world title screen) **before drawing** and supplies only `requested_enabled` to this render-only override. The retained menu still generates the immutable hit-test frame from the unmodified preview model after its source paint, so the source-exact paired R215 mouse transaction is never retargeted by visual state. Subsequent JNI acknowledgements invalidate the prior pixel receipt until the next GL11 draw.

R216's pixel sample still reads the rendered switch track using `glReadPixels` on the actual Minecraft GL context. R217 additionally checks `native GL11 drawn_frames == Java successful nativePaint calls + 1` on each visual receipt. This ensures the startup navigation exception is the **only** extra pass; a later duplicate draw fails the live acceptance. This check is strict for the deterministic disposable ForgeDev acceptance, not intended to represent global Minecraft draw calls.

## Actual live result

On Windows 11, Temurin Java 8, Minecraft 1.8.9 / Forge 11.15.1.2318, isolated TEMP-only offline game `%TEMP%\slinkywinky-r212-gui-r217-single-pass`:

| Source sequence | JNI requested | GL11 switch pixel | Java nativePaint calls | GL11 menu draws |
| --- | --- | --- | ---: | ---: |
| 1 | false | `404040` | 5 | 6 |
| 2 | true | `F35D12` | 35 | 36 |
| 3 | false | `404040` | 71 | 72 |
| 4 | true | `F35D12` | 104 | 105 |

The underlying preview flag stayed **false** throughout. Three in-process source-pinned mouse-down/up transactions on the actual painted Sprint switch reached PR #2's previously verified R210 JNI library and returned three genuine acknowledgements. R209 native ArrayList stayed at zero active rows in this no-world test. Orphan/non-target/wrong-release controls remained blocked; menu/feed and JNI owner closed and the original vanilla title screen was restored. **Forge `runClient` BUILD SUCCESSFUL, exit 0.**

An independent test validates the real Forge log and **20 mutation-based negative controls**: `tests/r217_forge_one_pass_source_paint.py --log <game dir>/logs/latest.log` and `--selftest`.

## Scope and follow-on

The test is original Minecraft's **real GUI mouse handler and render owner** driven by **scripted in-process callback coordinates**, not a physical hardware mouse validation. It is **not** injected into the untouched original Minecraft client and does not intercept LWJGL or WndProc. PR #2's separate Java class and native library remain outside tracked PR #1; the JNI DLL is the earlier verified R210 build, not a fresh PR #2 HEAD build.

The correct next validation is an isolated **manual physical mouse** run with loss-of-focus and Escape restoration, then integration with a real in-world module activation source; neither is claimed by R217. Both PRs remain draft/unmerged.

## Final regression and distribution audit

The full Windows x64 CMake Release build passed exit 0 with the one-pass renderer. **106/106 CTests PASS**, including the new `r217_forge189_single_pass_gl11_receipt_negative_controls` and all earlier WGL, JNI, input-safety and agent-attach tests, total 23.82 seconds.

A separate **R214 Java-button source-focused legacy integration** launched original isolated Forge 1.8.9 and completed three acknowledged native Sprint transitions with vanilla-screen restoration and clean exit: `R217_R214_LEGACY_FORGE_EXIT=0`. The new single-pass overlay does not disrupt that existing input route.

Without any R212–R217 opt-in, `gradle --offline --no-daemon clean jar` returned `BUILD SUCCESSFUL` / exit 0. Inspection confirmed that `SprintObservationProbe.class` from PR #2 and any `.dll` were **absent** from the default JAR; only dormant Java wrapper classes are packaged.

The R217 live-log validator accepted the real Forge receipt and **20 negative controls** passed. The new path has a verified deterministic invariant: `native GL11 draws = successful nativePaint calls + 1`, with the one extra draw only from initial navigation. That invariant is not a physical hardware-input claim or an in-world module test.
