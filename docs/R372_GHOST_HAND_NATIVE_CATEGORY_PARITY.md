# R372 — native Ghost Hand category reconciliation

**Scope:** PR #2 offline reference event bridge only. No gameplay action, input,
renderer injection, world mutation or network packet modification.

## Why this change was necessary

The recovered native 66-pointer module registration inventory
(`docs/R311_CONSOLIDATED_REFERENCE_INVENTORY.md`, independently SHA-pinned by
`tools/audit_r311_reference_inventory.py`) records **Ghost Hand** in native
category **Block** (numeric category 3). The older Java-derived
`docs/R256_GHOST_HAND_OFFLINE_EVENT8_BRIDGE.md` deliberately used Java
`COMBAT` (numeric 0). That cross-reference classification discrepancy was
not reconciled in the executable offline `ModuleExecutionBridge`.

As a result, `bind_ghost_hand()` rejected a correctly registered
native candidate Block module, while accepting the wrong Combat category.
This was a static metadata admission defect; the downstream policy
was not independently proven to match the original binary.

## Implemented

- `bind_ghost_hand` now requires native category Block = 3 and
  still requires the exact canonical name, single event-8 subscription,
  unbound state, correct owner lifecycle and valid event slot.
- The R256 source event-8 regression uses Block=3 for its positive
  shared Reach/Ghost Hand fixture and explicitly **rejects Combat=0**.
- R265 Hitboxes+Reach+Ghost Hand and R273 Piercing+Hitboxes+Reach+
  Ghost Hand combined-event fixtures now use Block=3. Original
  callback dispatch ordering and typed payload behavior remain unchanged.
- The legacy Java candidate category remains documented as a
  **different provenance**; no assertion that its Combat value is
  identical to the native 66-module manager.

## Acceptance

Relevant CTests:
- `r256_offline_ghost_hand_shared_reach_event8`
- `r265_hitboxes_reach_ghost_event8`
- `r273_piercing_event8_separate_rank_preview`

The regular Windows/Ubuntu Debug/Release and sanitizer matrix must pass
at the exact final commit before this change is marked fully verified.
Passing synthetic event replay confirms native-registration admission
only; it does not certify live Minecraft Ghost Hand behavior, original
anti-cheat interaction semantics or a working standalone client.

**Next step:** source-audit all remaining bridge category/name/slot
guards against the pinned native registration manifest, without
silently conflating reconstructed Java category constants with
native manager categories.
