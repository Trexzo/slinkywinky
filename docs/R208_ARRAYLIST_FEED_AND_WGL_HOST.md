# R208 — Chat 1 native ArrayList state publication and WGL host handoff

**Owner:** PR #1, `reconstruction/r1-core-fidelity`. Chat 2 / PR #2 is unchanged.

## What this implements

R207 added a real fixed-function WGL ArrayList painter, but its input was an isolated `ModuleArrayListSnapshot` fixture. R208 makes a concrete **bounded state publication API** available to the GUI shell without taking ownership of Java/Forge objects or pretending that UI preview switches are active modules.

- `ModuleArrayListFeed::publish` validates a complete, copy-only snapshot against the R207 source catalog, field consistency and the version-1 interface. It accepts only strictly increasing source sequence numbers and nonregressing world generations.
- `latest()` copies under a mutex; no producer string storage or lifetime token escapes to the renderer. The frame painter receives a stable snapshot value rather than a mutable reference to the publisher.
- Duplicate/replayed sequences, regressing generations, malformed module names, contradictory active state and invalid version **clear the displayed state and quarantine the feed**. A new attach session needs a new feed; a bad producer cannot restore previously active rows.
- A valid world-loss snapshot with `world_ready=false` and inactive records draws zero rows, even where requests remain enabled. `retire()` revokes the state permanently for the current feed.
- `WglMenuOverlayHost::draw_active_modules(feed)` now calls the existing GL11 ArrayList painter only after context ownership and exact viewport admission. It does not turn ClickGUI preview flags into active runtime state.
- Existing `WglMenuOverlayHost::draw` and JNI/layout interfaces remain unchanged.

## Acceptance boundary

Portable tests validate deep copy, increasing sequence, world loss, new generation, explicit retirement, duplicate/replayed snapshot quarantine, invalid version quarantine and generation regression. The Windows WGL regression calls the new host method with a real context, checks GL restoration, and proves retired feed state is rejected.

**Still not implemented:** Chat 2's owner-thread publisher -> feed wiring, producer/session ownership across injection and unload, true interactive original-game ClickGUI, and original-client in-game ArrayList drawing. In particular, a validated synthetic WGL snapshot proves the *GUI-shell consumer*, not that real gameplay modules work or that injection has become a finished client. The runtime needs to quiesce rendering before final `retire()` so an already-copied frame cannot race final shutdown.

## Shared contract with Chat 2

Chat 2 should publish an immutable version-1 complete state snapshot with canonical names, requested and actual active flags, real world readiness and generation, and strictly increasing `source_sequence`. Source sequence may **not** reset or wrap within one feed session. Reattachment must allocate a fresh feed. The graphics-owner thread draws the latest admitted snapshot only, and module/gameplay mutation remains owned by Chat 2.

No common header was duplicated across branches and no source/reference binary was committed. PR #1 remains **draft/unmerged**.
