# R224 — Original Slinky native slider live-drag preview

Date: 2026-11-15. Chat 1 native ClickGUI; PR #1 draft.

## Source parity

In pinned OpenSlinky src/engine/gui/gui_controls.cpp,
draw_slider_track evaluates a held slider against the mouse position
during dragging, not just on button release. draw_range_slider_row
also pushes the opposite range endpoint on a crossing.

Previously, WglRetainedMenuPreview::pointer used the release-only
numeric setter. Its separate existing MenuState numeric-drag preview
was already paintable by GL11 but never wired to this retained native
WGL pointer path. Worse, the temporary range preview clamped the
crossing handle while R220's actual commit pushed it. This could
render stale values during drag and then jump at release.

## Changes

- Begin a source-identified, temporary numeric drag on an admitted
  native WGL Down. Move updates only that render-preview sidecar,
  quantized to existing source step and bounds.
- Temporary range dragging now pushes the opposite endpoint, like
  the original handler and our R220 release setter.
- A following native WGL draw paints the updated slider/range value;
  no profile or gameplay state is committed before a valid Up.
- Release commits through the existing exact-source gesture and
  clears the temporary sidecar. Focus loss, rejected paint, explicit
  cancel, navigation and close clear pending numeric preview.
- No original Minecraft mouse hook, GL context takeover, standalone
  Forge module, packet modification or runtime gameplay action.

## Verification scope

The source-level R148 temporary drag test now asserts original two-
handle push and matching tooltip. The original production retained
WGL fixture R196 now repaints *while* the press is held, examines
the actual source-quantized native MenuRenderFrame, verifies both
crossing directions and checks that Up clears the temporary preview
and commits only the press-owned endpoint.

Full build, configured CTest and live acceptance status will be added
only after actually running them. Human physical input in unmodified
Minecraft remains unverified; PR #2 gameplay owner untouched.