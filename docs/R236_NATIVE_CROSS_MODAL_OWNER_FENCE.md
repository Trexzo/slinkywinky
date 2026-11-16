# R236 — Native ClickGUI cross-modal owner fence

Date: 2026-11-16. Chat 1, draft PR #1 only.

## Source-level issue

R234 made synthetic source bind and text keyboard modes exclusive in the retained WGL router. But some lower-level MenuState methods still bypassed that guarantee: opening a recovered popup, starting a slider drag, or writing a preview Boolean/slider could silently clear an active keybind or text owner when called directly. The retained WGL pointer dispatcher generally denies such actions through navigation; the underlying source-oriented model should be equally fail-closed.

## Correction

- Reuse the ABI-neutral MenuState::modal_input_owned() predicate in source dropdown, multiselect, color, optional-clear, numeric drag and preview setting entrypoints. This prevents background mutation without requiring a caller to go through MenuNavigation.
- Recovered text focus refuses existing keybinds, other modal owners, numeric drags, or a different focused text widget. Reentrant focus for the *same* text field remains allowed because R235 uses it to validate atomic typed events.
- Preserve explicit Escape/Enter/Cancel and popup close paths; normal menu operations resume only after owner release. No JNI object layout or profile serialization changes.

## Regression scope

Portable C++ tests use the actual recovered Auto Tool module and its Text, MultiSelect, Slider and Boolean callsites, plus Aim Assist's source dropdown. They check active bind/text ownership cannot be replaced by popup/drag/settings writes; pending state and preview values remain intact, while explicit cancellation restores legitimate actions. Windows, Linux, sanitizer, native JNI and Java 8 Attach CI are the authoritative independent build checks.

This is a first-party *native reconstructed GUI model* fix, not a live original Minecraft keyboard/mouse hook. No physical input interception, game event suppression or gameplay module control was introduced. Chat 2 PR #2 remains untouched. Keep PR #1 draft and unmerged.

## Verification status

Pending exact-commit CI results.
