# R270 — Anti Bot source history and relationship-bit-2 policy

Source: user-supplied verified reconstructed
dev/slinky/module/impl/AntiBotModule.java. This module listens on
event0, iterates remote players and proposes RelationshipService.set
(player, 2, rejected) based on a cached per-UUID History. Generated
native source mod_anti_bot.cpp establishes setting registrations,
but original native event priority and defaults remain uncertain.

The first-party C++ controller models the Java's exact ordered gates:
travelledSquared accumulates (prevX-x)^2+(prevZ-z)^2 each event,
always_stationary when <1, invalid UUID version when (low-msb &
0xf000)!=0x2000 with the mode1/mode2 movement distinction,
sleeping, always invisible without ever-seen armor, fractional
minimum ticksExisted, and the source's permissive validName scanner
(skips section-sign formatting and accepts *any* ASCII letter,
digit or underscore). Only after an accepted decision do the
original sticky armorSeen, ageAccepted and nameAccepted flags update.

Safety-specific differences: relationship updates are proposals
only; world identity is supplied by an opaque owner token; names
are UTF-16; malformed or nonfinite samples fail atomically;
256 remote entities per offline frame maximum. The live game may
not expose such owner tokens. World unload and disable return only
a flag-2 clear request and destroy history; no mutable game state.

The CTest regression covers accumulated movement, invalid UUID
modes, sleeping, armor-seen persistence, fractional min age,
username handling, owner exclusion, cross-world resets, malformed
batch atomicity and lifecycle clear masks.

Next: bind this controller to event0 alongside Teams, preserving
their separate relationship masks (Anti Bot=2, Teams=1) without
asserting either module's original native dispatch priority.
