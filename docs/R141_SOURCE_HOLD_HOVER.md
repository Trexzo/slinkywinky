# R141 — Source-style hover-only HOLD chips

**Branch:** `reconstruction/r1-core-fidelity` · draft PR #1, unmerged.

## Why this changes R140

The user's candidate HTML transcription in `decrypted files.zip/re/clickgui.tpl.html` has explicit CSS:

```css
.chip.hold { display:none }
.row:hover .chip.hold.can,.chip.hold.on { display:flex }
```

R140 originally displayed the HOLD chip persistently whenever its module had a key, which diverged from that reference. R141 implements the evidence-backed rule instead: HOLD appears when **that module's first row is hovered while a key is assigned**, or stays visible when HOLD is already on.

## Changes

- `MenuState::set_bind_hover_row` accepts only a source-supported, visible module in the selected category and rejects modal/popup conflicts. It cannot mutate gameplay state, key hooks, module enablement, persisted profiles or the injector.
- `MenuRenderFrame` snapshots `bind_hover_row`; `MenuMouseGesture` refuses a held input whose underlying hover state changed.
- Both native GL11 drawing and renderer-neutral hit-testing use the same rule: `bound && (hold || hover)`. Unbound modules and NOBIND modules never expose a HOLD action.
- The standalone Windows preview only samples `WM_MOUSEMOVE` on an idle, foreground-owned window, computes its visible first-row rectangle using shared card geometry, and cancels hover through `WM_MOUSELEAVE` and focus loss. It does not register global mouse hooks, and it does not read other application windows or alter Minecraft input.
- `native_recovered_keybind_preview_safety` now checks hidden-before-hover, visible-during-hover, and persisted-when-selected HOLD across standalone, bounded and compact panel modes.

## Fidelity caveats

Source CSS animations and hover shades are not yet visually compared pixel-by-pixel against decrypted native screenshots. The binding control currently uses a conservative fixed-width approximation for key and HOLD chip layout, not a raster-accurate copy of Inter Medium/Bold metrics. R139 Friends actions remain inert because the supplied HTML contains no handler. No native Minecraft binding integration is claimed by this GUI-only work.

**Verification:** status must be read from GitHub Actions for the exact branch head; passing a previous build does not prove this hover change passed.
