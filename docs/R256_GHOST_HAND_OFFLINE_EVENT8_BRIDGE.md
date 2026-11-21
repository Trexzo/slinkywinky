# R256 — Ghost Hand event8 shares the offline Reach query

The reconstructed user-supplied GhostHandModule.java specifies
category COMBAT and event8, not the inherited matrix's block category.
The original native generated mod_ghost_hand.cpp verifies its setting
registrations, not callback priority. Priority 0 in the new fixture
is **not** an authenticated original priority.

R256 binds an exact single event8 Ghost Hand descriptor (category0)
to the existing ModuleExecutionBridge. Its typed payload has two
separate outputs: event8 query_override (always true when the
registered callback executes) and can_interact_through (independently
computed using the recovered R255 target/item predicate).

A single simulated event8 can now deliver both Reach and Ghost Hand.
These proposals are not a real raycast or a Minecraft attack/interaction.
The bridge's existing typed-count validation was extended only for
the exact two-policy event8 combination. Other unrelated typed payload
combinations remain denied.

CTest verifies name/category/slot/extra-subscription negatives,
real first-party Reach and Ghost Hand event8 dispatch order, item
and relationship denial despite the independent query flag,
disabled callback isolation, malformed event and shutdown.

Still missing: source-exact event8 native priority; authentic
Minecraft entity and relationship snapshots, target visibility,
host setting values, and runtime interaction acceptance.
