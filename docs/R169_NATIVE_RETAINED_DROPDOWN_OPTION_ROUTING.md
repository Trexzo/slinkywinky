# R169 — first-party retained dropdown option delivery through Java 8 JNI

**Target:** `Trexzo/slinkywinky`, branch `reconstruction/r1-core-fidelity`, draft PR #1.

R168's four GitHub workflows passed. R166–R168 had already verified Java 8 → test-only JNI → native WGL ClickGUI toggles, slider values, recovered Booleans, and source-paged settings. But `WglRetainedMenuPreview::pointer()` still discarded the recovered dropdown hits identified by the original GUI source catalog. R169 closes that specific **native GUI interaction** gap, without enabling a Minecraft input hook.

## Production first-party C++ GUI change

The retained menu now dispatches the existing, source-verified `MenuHitKind` identifiers:

- `AimModeControl` → `MenuState::open_preview_dropdown(AimMode,sourceIndex)`
- `TargetModeControl` → `MenuState::open_preview_dropdown(TargetMode,sourceIndex)`
- `RecoveredChoiceControl` → `MenuState::open_recovered_choice_preview(moduleIndex,widgetIndex)`
- `DropdownOption` → `MenuState::select_preview_dropdown_option(ordinal)`, with unsigned-bounds validation

The existing menu model checks current tab, expanded module, visible page, exact source widget identity and valid option ordinal. The existing `MenuMouseGesture` requires a matched owner-thread mouse-down/release on the same retained frame, preventing selection after a changed modal state. No new input hooks, module execution, keybind polling, JVM retransformation, packet logic or filesystem writes were added.

Two read-only, active/owner-checked accessors return the local Aim preview mode and whether a dropdown is currently open. These are **preview-only** state receipts and are not evidence of gameplay or an authenticated Minecraft input owner.

## Isolated Java 8 JNI regression

The existing **disposable** JNI DLL adds three test-only methods: `modeReceipt`, `aimControlTarget`, and `aimOptionTarget`. Each validates the live test WGL owner and computes exact source hit-test coordinates using the same `menu_module_card_rect`, `menu_setting_column`, `menu_settings_control_offset`, `menu_dropdown_popup_geometry`, and `hit_test_menu` functions as the retained renderer.

`R169DropdownJniSmoke` tests actual source Aim Assist / Aim / Mode dropdown opening, rejection of unarmed and off-thread calls, denied synthetic exclusive/foreground/viewport inputs, popup modal ownership preventing click-through to the underlying module toggle, invalid option refusal, cancellation after a held option, and valid option selection twice (Regular → Lock-on → Linear). It verifies popup closure, redraw persistence and the fact that changing the **preview-only** Aim mode does not mutate `ClientSettings` or create gameplay actions. The test releases the disposable WGL context and verifies calls are refused afterward.

The new `r169_java8_jni_retained_dropdown_option_routing` CTest uses Java 8 and the existing segregated `slinkywinky_r166_pointer_test_jni.dll`; it is never linked into `slinkywinky_jni.dll`. Java source guards and agent JAR checks assert the test class is not packaged in `slinkywinky_instrument.jar` or the original-Mojang source-audit JAR.

## Outstanding

R169 does **not** route real original Minecraft 1.8.9 mouse events. Synthetic test flags do not establish exclusive live input ownership; reversible restoration of all original input owners and game-thread integration are still required. Only recovered GUI *preview* modes and already modeled configuration can be edited, not all 67 gameplay modules. The draft PR remains unmerged pending independent original-client acceptance.
