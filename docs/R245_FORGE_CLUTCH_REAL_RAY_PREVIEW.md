# R245 — opt-in real Minecraft 1.8.9 prior-block Clutch ray preview

Date: 2026-11-20. Draft SlinkyWinky PR #2 only; PR #1 unchanged.

## Purpose

The previous R237/R239 Forge preview captured all 167 original-source
candidate positions and could produce native aiming values, but it set
the previous-block request to null every tick. R244 introduced a
two-phase JNI ray transaction; R245 connects that path to the real Forge
client-world `ForgeClutchWorldPort`, **strictly for observation**.

## Admission and behavior

To activate, both JVM properties must be true:

- `-Dslinkywinky.r237ClutchReadOnlyProbe=true`
- `-Dslinkywinky.r245ClutchPriorReadOnlyProbe=true`

The existing R237 gates are unchanged: explicit Sprint observation,
a specifically named TEMP Forge game directory, the isolated dummy
`R237Offline` account, a manually entered local singleplayer client
world, and F9 activation. Action trials are incompatible. Setting the
R245 property alone is now an initialization error. Ordinary profiles
and default preview remain unchanged.

During at most 20 client `PlayerTickEvent.END` samples, the preview:

1. Samples the real 167-cell candidate prefix, ground and previous-block
   solidity using the bound `WorldClient` / `EntityPlayerSP`.
2. When an earlier **read-only native proposal** selected a block, carries
   its coordinates and 3D target forward to the next tick. The Java
   snapshot authenticates its previous block before crossing JNI.
3. Asks the R244 native preflight whether the previous target actually
   needs a ray. Only then calls the Minecraft 1.8.9
   `world.rayTraceBlocks(..., false, true, false)` adapter on the client
   thread. Null hit is a known result, not guessed success.
4. Feeds that answer back to the same native session/tick and reports the
   original source planner's final read-only proposal.
5. Logs actual ray call count, prior request count and preserved-prior
   count. On new world/player context, resets sampler, native session and
   cached target. Any JNI/world failure stops the preview.

Every return value remains a **diagnostic only**: no camera or player
rotation, use clicks, packets, key synthesis, placement, or inventory
actions. A retained prior is an earlier *proposal*, not proof that a
block was placed or that original Clutch gameplay semantics are matched.

## Validation limits

R237 pure Java 8 workflow checks source-table parity and sampler tests.
The R239 JNI workflow checks synthetic Java-to-C++ prior transactions.
The independent legacy Forge 1.8.9 build must also compile the modified
Forge preview, and a manually admitted local singleplayer run must
confirm actual per-tick ray observations. Until those pass, this remains
an implemented but unverified live-integration candidate.

Minecraft original-vanilla injectable integration (without Forge), the
original event dispatch/rotation/click/hotbar/restore mechanics, and the
remaining Slinky modules are still incomplete. No commercial Slinky
binary, source instructions or protected asset is published.
