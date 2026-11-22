# R281 — native owner-bound roster-to-event0 adapter

The source-derived R266/R270 Teams and Anti Bot policies and the
R275 UUID-bound event0 bridge now have a shared **read-only C++ host**.
RosterReadOnlyEvent0 accepts one validated immutable-style value
snapshot (world/owner/generation/tick, owner UUID and remote player
UUID/name/movement/age/visibility/armor) and maps it into the already
tested ModuleExecutionBridge.

It never accesses Java objects, Minecraft game pointers, packets,
input, scoreboard state or RelationshipService. A caller has to
explicitly enable one/both modules; callbacks use offline fixture
priorities only (native source priorities remain unknown).

Teams is fail-closed without *explicit complete* team color evidence:
R276's current real Forge sampler has UUID/name/movement data but
does not yet capture all ChatComponentText display/team-player-list
overrides used by the recovered TeamsModule.java. Consequently
there is NO false claim of source-equivalent team color recognition
from raw names alone. Anti Bot may be previewed independently.

The native adapter verifies owner-thread affinity, nonzero distinct
opaque world and owner tokens, generation monotonicity across owner
changes, strictly increasing ticks per owner (no duplicate movement replay), unique remote UUIDs, finite
movement positions, and internal name/UUID correspondence before
dispatch. Proposed relationship bits stay separate and are never
written to the game.

CTest covers explicit enabling, combined policy decisions, missing
team color evidence, independent AntiBot preview, duplicate-name
UUID collision, owner token/generation, world change, tick regression,
nonfinite source rejection, mismatched name views, foreign thread,
shutdown, and post-shutdown refusal.

NEXT: add a narrow JNI facade that converts R276 Java immutable
receipt values into these C++ structures and returns decisions
without writing any game data. Then exercise the facade from an
isolated Forge PlayerTick receipt.
