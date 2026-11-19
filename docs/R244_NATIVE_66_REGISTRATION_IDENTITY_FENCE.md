# R244 — Source-exact 66 native registration identity fence

2026-11-19 | Chat 1, draft PR #1 (`reconstruction/r1-core-fidelity`).

## Why this matters for full Slinky reconstruction

The first-party GUI preserves **67** recovered candidate descriptor names.
The independent R193 native module-manager audit on draft PR #2
(`integration/openslinky-fd9c592-reference`), checked against pinned
Skid-Technology/OpenSlinky@fd9c592, identified **66** actual manager
registrations after removing a surplus Chest Stealer entry in the
candidate registration table.

Prior to R244, the PR #1 ArrayList consumer refused more than 66 rows
but accepted **any subset** of the 67 GUI labels, including the one
unregistered manager descriptor. Consequently, an incorrect producer
could publish an unregistered `Chest Stealer` as active, and the
display might falsely imply that it had a running native module.

## First-party change

- Expose `candidate_native_manager_registered(name)`, a pure
  source-catalog identity check that admits exactly the 66 candidate
  manager registrations and rejects `Chest Stealer` and unknown names.
- Apply that predicate to every `ModuleArrayListState` before it can
  become an active row or be admitted into the feed, even when an
  unregistered candidate is requested=false and active=false.
- Preserve the GUI's exact candidate 67-label catalog and order.
  No setting, click, game action, source descriptor or ABI changes.
- Reject malformed owner snapshots and retire any older ArrayList
  snapshot, rather than retaining a stale visible activation claim.

## Regression assertions

The original portable `r207_module_arraylist_runtime_state_only`
CTest now verifies 66 accepted registration names, one explicitly
excluded catalog descriptor, valid full 66-record inactive snapshot,
and rejection of 67 rows and 66-row subsets contaminated with
the excluded descriptor. A malformed source update must quarantine
the entire feed and remove its previous active display.

This guard **does not** implement Chest Stealer or promote any of
the other 66 modules to gameplay parity. PR #2 R235 currently describes
**23 first-party offline policies / 41 of 171 native callback
bindings**, with 130 still missing. These are neither 23 playable
modules nor proof of complete original Slinky reconstruction.

No Minecraft process was launched. PR #2 remains unmodified.
Keep PR #1 draft and unmerged.

## Exact-head CI
To be recorded after final GitHub checks have actually completed.
