# R188 — bracket the original foreground pointer sample with exact HWND audits

**Branch:** `reconstruction/r1-core-fidelity`, draft/unmerged [PR #1](https://github.com/Trexzo/slinkywinky/pull/1). PR #2 (the separate C++ OpenSlinky reference integration) remains untouched.

## Defect and scope

In R183 the original-Minecraft renderer sampled an optional foreground-only physical pointer via `audit_original_render_pointer`, committed a passive GUI hover sample, and **then** independently sampled exact Win32 foreground identity for the R183 tri-state trace. If the OS foreground changed while the cursor was being read, the earlier pointer could have been classified as a foreground hover based on a different moment from the OS-focus trace. A later R183 mismatch was refused, but R176/R182 had already advanced the original passive counters.

R188 adds a conservative and strictly **read-only** paired-window admission: exact original window audit **before** the cursor query, foreground-only pointer query, and exact original-window audit **after** it. The pure `reconcile_original_pointer_focus(before,pointer_admitted,after)` only returns `ValidForeground` when both audits were foreground **and** the cursor sample succeeded, `ValidBackground` when both audits were background **and** the foreground-only cursor was absent. Any disagreement, invalid HWND/WGL state, or missing cursor during otherwise valid foreground is **Refused**.

The reconciled state is used consistently by both `observe_hover` and `observe_original_window_focus`. An ambiguous sample is never credited as foreground source hover, never mistaken for background, and never used to fabricate an OS focus-loss/reacquisition sequence. No raw cursor data is logged and nothing is sent to the input queue.

This closes the observed **between-audits inconsistency**, but is **not atomic**: focus could switch away and back between two reads, and Win32 GetKeyState is not a physical hardware-event stream. It does not establish exclusive mouse ownership, safe input consumption, a real ClickGUI click or commercial Slinky module fidelity.

## Regression evidence

The disposable Windows GL11 native test exercises **all 18** combinations of before/after exact-focus status (`Refused`, `ValidBackground`, `ValidForeground`) and pointer success/failure. Only the two fully consistent combinations survive. The same fixture calls the real read-only Win32 WGL audit around a physical cursor read on its own disposable viewport without changing user focus, cursor position or button state. External focus races remain a valid `Refused` result.

The full native Windows / Java 8 / JNI CTest suite passed **92/92** after the R188 changes, including R182/R183 focus, JNI Attach, retained WGL draw and pointer transactions. The separately launched, isolated **original vanilla Minecraft 1.8.9** R99 acceptance also exited successfully with six source restoration/admission stages, 64 retained render frames, 60 source-visible slider draw frames and 63 read-only hover samples. The original-JVM R183 receipt records `foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0`; R179 source `exact_hits=0`, and `native_click_delivery=disabled`. Thus the original foreground-slider manual gate **has not been satisfied**: all original-client samples were background, not physical foreground hover. Live click delivery stays disabled and PR #1 stays draft.

## Manual gate still outstanding

The exact original-client 64-frame read-only acceptance can demand `--require-foreground-source-hits --require-focus-cycle-source-hit --require-exact-os-focus-cycle`, but requires an actual operator-driven foreground/background/foreground cycle and cursor hover over the source slider while the finite renderer trace runs. Automatic focus switching, sending mouse input or bypassing the terminal requirement to fabricate the result is not permitted.
