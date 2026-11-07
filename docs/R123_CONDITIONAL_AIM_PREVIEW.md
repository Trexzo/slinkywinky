# R123 — Source-conditional Aim Assist ClickGUI preview

**Repository:** `Trexzo/slinkywinky`  
**Branch:** `reconstruction/r1-core-fidelity` / Draft PR #1

## Provenance

R122 recovered 24 static Aim Assist GUI widget callsites from the user's
`decrypted files.zip` candidate archive, including mode-dependent duplicate
Horizontal and Vertical speed controls. The original archive is not
independently authenticated as a signed official Slinky release.

A list of source callsites **is not** the set of controls displayed at one
moment. R123 introduces an independently selectable, strictly **read-only**
preview context to filter those callsites without altering the 385-entry
forensic catalog, executing original code, or changing Minecraft settings.

## Implemented

- A pure `recovered_widget_visible_count` and
  `recovered_widget_visible_page` select the relevant Aim Assist callsites
  using the R122 source-derived visibility predicate. All other module
  pages retain their previous read-only behavior. The old
  `recovered_widget_count/page` functions continue returning the entire
  unfiltered forensic catalog and are still used for source auditing.
- The standalone GUI's Aim Assist preview can cycle **Regular, Linear,
  Lock-on and Silent** with **F7**, and **Single/Switch** target mode with
  **F8**. Actions require a focused, expanded Aim Assist card on the
  Combat tab. They operate only on local `MenuState`, not gameplay state.
  Held-key autorepeat is refused by the separate Win32 preview.
- Each mode change resets Aim Assist's settings-page offset to zero.
  Page Up/Page Down and mouse-wheel browsing now page through the
  *visible* controls, not through conditional callsites that should be
  hidden. The WGL header indicates the selected preview mode/target.
- `MenuRenderFrame` snapshots the preview selection, so a frame cannot
  silently acquire updated control visibility. The shared renderer,
  five-item page reader, height calculator, scroll/wheel hit tester and
  offset validation all use the same selection context.
- `MenuMouseGesture` cancels any pending click when the preview mode or
  target mode changes, even when the module, page offset and position
  remain identical. Forged/invalid mode values fail closed before
  rendering or hit testing.

## Recovered conditional rules

| Aim mode | Visible widget callsites in the synthetic preview |
|---|---:|
| Regular | 18 |
| Linear | 18 |
| Lock-on | 17 |
| Silent | 19 |

Single vs Switch changes **which** target option appears, but not the
number of visible controls. The static native Aim Assist inventory
still has **24** entries because conditional branches are retained for
forensic review. The full source catalog remains **385 records**, across
67 modules, and numeric values/ranges are still illustrative.

The branch selection does not claim to replicate exact official runtime
timing, animations, complex UI popups or the actual module's control
logic.

## Regression checks

- Tests enumerate every combination of four Aim modes and two target
  modes, prove the correct conditional callsites are returned once
  each, bound page lengths to five, and reject missing/invalid
  context or stale offsets.
- Navigation tests reject switching when the Aim Assist card is closed
  or not focused; they verify page reset, cycling, invalid enums and
  wrong-tab refusal.
- Mouse-gesture tests verify that switching Aim or Target mode
  invalidates held input even with unchanged card geometry.
- WGL tests draw multiple mode selections in a disposable standalone
  900×630 OpenGL context, require GL state restoration, reject forged
  context modes, and permit optional readback to files when requested
  by the CI/local operator. No original game input was intercepted.
- GitHub workflow status must be confirmed independently for the
  exact R123 commit after push.

## Not yet finished

This is **not a completed ClickGUI**. Source-derived section labels,
control types and conditional visibility are being recovered; live
settings editing, reliable native numeric defaults, authentic font
metrics, animation, popup behavior, profile persistence and in-game
input capture still need independent evidence. R119's guard against
unverified original Minecraft keyboard interception remains in place.
None of the 67 modules is certified as fully reconstructed by this GUI
work. Keep PR #1 draft and unmerged.
