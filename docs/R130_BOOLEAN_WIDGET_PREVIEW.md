# R130 — Typed Boolean editing for recovered ClickGUI preview records

**Scope:** Separate first-party Windows GL1.1 preview ONLY. No game actions, Java/packet changes, Forge dependency, or original Slinky DLL redistribution.

The candidate decrypted GUI catalog currently presents **385** typed setting-widget records, **206** of which are Boolean. R125 already handled Piercing's seven Boolean controls. R130 adds one shared, source-indexed preview editing path for the other Boolean records across module cards, including conditionally visible Aim Assist controls.

## Implementation

- Each entry is keyed by its exact position in the immutable `recovered_widget_catalog()` array, not label text; this prevents collisions between repeated setting labels or conditional branches.
- `MenuState` stores a nullable Boolean override per catalog record. `nullopt` displays the catalog's **unverified example state**; an explicit override affects only the local preview, never `ClientSettings` or the gameplay module's active state.
- A click only qualifies when its module category and catalog index match the currently selected tab; the module is expanded; the callsite is in the five currently visible recovered controls; and the widget is of Boolean kind.
- Input hit-testing uses the exact existing renderer's 34×18 switch pill, not label text or card header. The immutable `MenuRenderFrame` carries the override array; `MenuMouseGesture` compares it to reject stale in-flight clicks.
- Piercing retains its existing isolated 7-control handler and cannot be modified by the generic path. Open dropdowns and the Unload modal block generic input.

## Test and limitations

The newly registered `native_recovered_boolean_preview_safety` CTest covers all three preview panel modes, actual switch vs label hitboxes, source-index validation, wrong module and off-page refusal, stale-held-click invalidation, modal protection, Piercing non-interference, and branch-hidden Aim Assist Lock-on-only Boolean settings.

The source-side fixture currently has 206 Boolean records; many values and defaults remain **estimated**. Visual interaction is not evidence of exact original commercial module logic, and no gameplay module has been newly enabled.

**Verification status:** Pushed to the draft branch; complete Windows Release, CTest, actual OpenGL screenshot and Minecraft checks on the new commit are pending. Desktop Commander is offline.
