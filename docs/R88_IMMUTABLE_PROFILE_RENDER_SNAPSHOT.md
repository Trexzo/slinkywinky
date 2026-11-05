# R88 — immutable expanded native-profile values in renderer-neutral frames

**Scope:** SlinkyWinky first-party C++ native UI infrastructure. No injected-game UI, Java Attach, Forge runtime, combat dispatch, Minecraft memory, or packet write was introduced.

R87 let users change one numeric `ClientSettings` field per supported module, but GL11 still rendered only static placeholder captions (while the standalone GL preview placed the value in its Windows title). R88 passes that value through the same owner-bound model that already renders module switches.

## Model and renderer changes

- `MenuRenderSession::next(menu, profile)` accepts an *optional* typed, first-party `MenuProfileBridge` reference and rejects a bridge bound to any different `MenuState`, without consuming a sequence.
- Each accepted frame copies immutable `MenuRenderParameter {catalog_index, caption}` records only for **currently visible, expanded, implemented** rows. The snapshot owns the resulting strings. No pointers to game objects, Java references, mutable profiles or Win32 resources cross the render boundary.
- Captions are formatted by the owner thread using the already-validated first-party setting values, with stable decimal precision. Old snapshots do not change when the profile is edited.
- The GL1.1 renderer draws the frozen value within the module row. It rejects more parameter records than visible rows, duplicate records, hidden/unexpanded/unsupported indexes, captions longer than 48 bytes, nonprintable or non-ASCII text. Validation occurs before GL attribute or matrix changes.
- GDI preview consumes the same snapshot; standalone GL preview and optional bounded-overlay host pass a bridge explicitly. Existing `next(menu)` and `draw(menu)` callers remain supported and render the earlier read-only placeholder when no bridge is supplied.

## Verification

Expanded native render-session CTest covers initial/edited immutable captions, wrong-menu bridge refusal, scroll and tab hiding. Windows GL11 CTest covers legitimate numeric frames, mismatched/unsupported/duplicate or malformed captions, and preserved WGL state.

Run all platform-supported CI workflows and local native CTest on the exact final commit; do not call the change verified from a source push alone.

## Deliberate nonclaims and gate

R88 makes a useful piece of the future injectable UI reusable, **not** a playable injected client. It does not establish that `ave.av → bfk.a(FJ)V` has an approved WGL context in an actual original Minecraft 1.8.9 process. The R81 real-game supervised receipt check and visual game-window acceptance remain blockers for enabling production live GL menu drawing. Keep PR #1 draft/unmerged.
