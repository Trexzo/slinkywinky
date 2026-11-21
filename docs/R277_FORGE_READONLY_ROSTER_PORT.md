# R277 — isolated Forge 1.8.9 read-only roster Port

This adapter connects R276's tested Java 8 RelationshipRosterSampler.Port
to Minecraft 1.8.9 client APIs. It only reads playerEntities and
the source fields used by AntiBotModule.java (identity/UUID, name,
previous/current X/Z, age, sleeping, invisibility relative to owner,
and observed nonempty armor slots).

It requires a local client-side WorldClient and integrated singleplayer
world. The creator thread is pinned and every iteration rechecks host
identity; R276 validates the source again before issuing a receipt.
It never writes RelationshipService, inventory, movement, packets or
player state.

This adapter is NOT yet invoked by the Forge preview and is NOT built
by the primary CI. A separate isolated R278 Forge hook and real
Minecraft/Forge compilation/launch verification are required before
claiming real player observations.
