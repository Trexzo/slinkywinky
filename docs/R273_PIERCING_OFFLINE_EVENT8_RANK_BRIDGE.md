# R273 — Piercing event8 override, separate candidate-ranking API

Recovered Java PiercingModule.onEvent(type8) does exactly one thing:
event.queryOverride=true. The Java source's rank() and
pierceBlocks() are separate methods called by the ray/target host;
R273 deliberately does NOT move ranking into the event callback.

R273 adds the exact single event8 COMBAT module handler to the
offline ModuleExecutionBridge. Its queryOverride flag is emitted
only for a registered enabled module. Event8 may also carry
Reach, Ghost Hand and Hitboxes proposals, with four-module typed
event allowlisting and no leaked cross-event payloads.

preview_piercing_rank and preview_pierce_blocks call R272's pure
controller ONLY when a Piercing module is registered/bound/enabled.
A caller supplies the already cached Weapons eligibility and
relationship bit masks; this is a value-only preview, not a
Minecraft item or raycast observation.

The regression covers four-way event ordering (fixture priorities,
NOT verified native scheduling), independent rank previews,
disabled module isolation, priority overrides, malformed
event/registration metadata and stop.

Still missing: original native callback priority, genuine
objectMouseOver/ray candidate iteration, Minecraft 1.8.9 host
observations and live acceptance. No gameplay writes or packets.
