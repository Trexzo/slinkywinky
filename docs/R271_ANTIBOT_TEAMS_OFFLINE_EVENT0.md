# R271 — Anti Bot and Teams source event0 bit isolation

Both user-supplied Java reconstruction modules subscribe to semantic
event 0, but write independent RelationshipService mask bits:
Teams=1, AntiBot=2. R271 adds the source-derived R270
AntiBotController as a typed callback in the existing offline
ModuleExecutionBridge and preserves this separation in proposal
results. No actual RelationshipService data is read or changed.

The offline frame can include Teams, Anti Bot, Clutch, Block In,
Safe Walk, Fast Mine and Fast Place. A frame containing both
relationship policies is rejected before dispatch unless it
describes identical readiness and the same ordered remote-player
name roster. World identity and per-UUID history are owner-bound
to the AntiBot payload. Native callback priorities are NOT
authenticated; supplied order 40/30 is only a test fixture.

AntiBot's per-player history is snapshotted and rolled back
if later synthetic callbacks fail, on both shared-source-RNG and
ordinary dispatch paths. Disable, stop and explicit world reset
clear saved history. An entity/UUID admission error remains a
value-only, fail-closed result.

CTest verifies separate friend/blocked outputs, mismatched rosters,
not-ready sources, foreign events, Clutch co-delivery, disable,
identity/UUID mode behavior (source-verified equality branch), transactional rollback, malformed
metadata and stop. No remote or local Minecraft player object
is accessed or written.

## R275 identity strengthening

A shared frame requires the same explicit nonzero source-world token, local
owner UUID and one ordered remote-player UUID per corresponding name.
Display names alone are not unique and cannot authenticate roster identity.
Standalone Teams frames remain independently testable, but a combined frame
with missing or mismatched provenance is refused before any callback runs.
No game objects or relationship bits are changed.
