# R168 — Java 8 recovered Boolean / settings-page integration

**Target:** `Trexzo/slinkywinky`, branch `reconstruction/r1-core-fidelity`, draft PR #1. The prior R167 commit `170389d` passed all four GitHub CI workflows.

## Delivered

R168 extends the isolated Java 8 → JNI → first-party retained C++ WGL ClickGUI path beyond the R166 module toggle and R167 Auto Block numeric slider.

- A narrow **read-only** `WglRetainedMenuPreview::settings_page(index)` returns the actual active owner-thread menu's setting-page offset, with strict active, owner, catalog, tab and expansion checks. It does not grant input ownership or gameplay access.
- The disposable `slinkywinky_r166_pointer_test_jni.dll` exports test-only `pageOffset`, `page` and `booleanTarget` for Auto Block. Only native `NextSettingsPage` and `PreviousSettingsPage` actions are allowed, and only when Auto Block is the focused, expanded source module. The test-only target resolver refuses all pages except exact source offset 10, builds the same recovered-widget source slice, control-column/vertical layout and hit-test as the real GUI, and returns physical window-pixel coordinates for **Auto Block / Conditions / Damaged**.
- `R168RecoveredBooleanJniSmoke` sends those pointer transactions to the actual native retained menu, reads the validated `block_requires_damage` field, and confirms false → true → false with redraw persistence. The test verifies offsets 0 → 5 → 10, rejection beyond page three, and backward navigation 10 → 5 → 0.
- The regression also denies collapsed/unfocused pages, wrong-thread access, synthetic exclusive/foreground loss, viewport changes, cancellation and stale mouse-up after a page switch. None of the rejected gestures may change the native profile.

A new `r168_java8_jni_recovered_boolean_settings_paging` CTest target runs only under Windows with Java 8 and the optional JNI test fixture. The original R166/R167 tests continue unchanged. The Java smoke class is excluded from both the injectable agent JAR and the original-Mojang read-only audit JAR. Production-source checks forbid accidental use of R168's disposable Java class.

## What this does not establish

The mouse/foreground permission parameters in this test are synthetic acknowledgements; they are not authentic original Minecraft input-ownership receipts. **No original Minecraft 1.8.9 game process, LWJGL original queue, loaded game class or gameplay action is modified or intercepted.** The additional read-only native accessor does not install an input hook. The retained GUI now has stronger first-party controls in a fixture, but the real live ClickGUI and all 67 reconstructed module behaviors remain incomplete.

Next acceptance must verify original game input ownership and restoration, then connect that authenticated input path to the production retained GUI and separately implemented module models. Keep PR #1 draft until independent original-client acceptance.
