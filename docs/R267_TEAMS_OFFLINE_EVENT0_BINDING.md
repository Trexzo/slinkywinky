# R267 — Teams semantic event0 offline bridge

Reconstructed Java source: TeamsModule.java SHA256
5f7c4fd36327e8de251d36e92a350f32f88626205e35ab30c61ac1761962ec09.
Native src/gen/mod_teams.cpp SHA256
ec016912aae94753b1e7de50b4ae9a7ac06c5afaa62bc2722b4f8da617928212.
The Java event callback subscribes to event 0, but native subscription
priority is not independently authenticated; a priority-50 Teams
descriptor is used only as an offline fixture.

R267 binds the pure R266 UTF-16 name-color controller into the existing
ModuleExecutionBridge, validates the utility module's exact single
event-0 subscription, and accepts value-only snapshots of the owner
and remote-player display names/settings. Each remote name gets an
independent proposed RelationshipService bit-1 result. Disabled or
not-ready modules emit no proposals. Output size is limited to 256
players to prevent unbounded allocation by synthetic offline fixtures.

The typed event0 dispatcher now supports Teams alongside Safe Walk,
Fast Place, Fast Mine, Clutch, and Block In. The previous
combined_edge_place state validation had a latent null dereference
when a valid frame contained Clutch+BlockIn but no Safe Walk/Fast Place/
Fast Mine: R267 replaces that unsafe fallback with an optional
source in-game observation. Actual conflicting in-game evidence is
still rejected.

CTest covers membership, explicit colors, toggles, frame readiness,
same-event other modules, optional-state corner case, contradictory
states, false event, metadata negatives and shutdown.

No real RelationshipService modification, network packet, interaction
or Minecraft player access. Event ordering and origin tracing still
require disposable Minecraft 1.8.9 evidence.
