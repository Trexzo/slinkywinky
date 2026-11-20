# R260 — original Minecraft 1.8.9 outgoing attack-attempt callsite

Date: 2026-11-20. Chat 1 draft PR #1. PR #2 remains unchanged.

The Minecraft 1.8.9 `joined.srg` mapping establishes:

- `ave.aw()V` = `Minecraft.func_147116_af` / `clickMouse()`
- `bda.a(Lwn;Lpk;)V` =
  `PlayerControllerMP.func_78764_a` / `attackEntity(EntityPlayer,Entity)`
- `wn.f(Lpk;)V` =
  `EntityPlayer.func_71059_n` / `attackTargetEntityWithCurrentItem(Entity)`

R260 inspects **only** the SHA-1/size-validated original Mojang
1.8.9 client, and separately SHA-256-pinned `ave.class`,
using the ASM library already employed by the prior original
client read-only audits. It verifies a unique `ave.aw()V`,
a unique outgoing `INVOKEVIRTUAL bda.a(Lwn;Lpk;)V`
from that method, and a declared matching `bda` method.
It reports the directly observed `wn.f(Lpk;)V` calls in
`bda.a` and the direct client tick-to-click calls, but
does not promote an unverified count into an authorization.
The Mojang JAR lives only in the temporary CI directory,
and the R260 verifier is compiled into the read-only audit JAR,
**not** the injected production agent.

The disposable Java8 ASM smoke test generates synthetic
class bytes for matching and multiple non-matching graphs,
including missing/duplicate callsites, wrong method/owner
descriptor, static click, absent method and incorrect class.
Neither test runs Minecraft or emits modified class binaries.

**Semantic boundary:** A click-to-attackEntity callsite
demonstrates a potential **outgoing attack attempt**, not a
confirmed landed hit, packet delivery or server acknowledgement.
It cannot be substituted for `pr.bg()I`'s *incoming*
last-attacker timestamp. R260 does not install a callback,
write player state, derive Reach activation timings or
enable other reconstructed modules. Runtime observation
and controlled game-thread callback mapping remain
a separate uncompleted step.

## Verification

Exact commit and CI evidence to be recorded after runs.
