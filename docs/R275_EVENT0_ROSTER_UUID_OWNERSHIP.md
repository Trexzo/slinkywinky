# R275 — Teams/Anti Bot shared-roster UUID and owner proof

The recovered TeamsModule.java and AntiBotModule.java both use
`context.world().playerEntities` and exclude the local player by
object identity. Their original Java source does **not** claim that
a displayed name is a stable entity key.

The R271 offline bridge checked only remote player display names.
R275 adds strict source-world, source-owner UUID, and ordered remote
UUID evidence to the shared event-0 frame. An omitted identity,
mismatched world, swapped owner, reordered player, or two players
sharing the same names cannot pass. Standalone Teams payloads do not
need AntiBot evidence. No real Minecraft world, player, relationship
or network data is modified.

Regressions include duplicate names with reordered UUIDs,
mismatched world/owner tokens, missing UUID roster, and the existing
independent bit-1/bit-2 event policy tests.
