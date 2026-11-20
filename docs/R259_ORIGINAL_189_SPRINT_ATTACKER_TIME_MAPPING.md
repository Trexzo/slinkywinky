# R259 — exact original vanilla 1.8.9 sprint and last-attacker ABI

Pinned public MCP 1.8.9 `joined.srg` identifies
`pk.aw()Z` as Entity `func_70051_ag` / `isSprinting`
and `pr.bg()I` as EntityLivingBase `func_142013_aG` /
`getLastAttackerTime`. The latter records the time when the
entity's last attacker was set, **not necessarily the player's last
outgoing attack**. The source-based Reach cooldown interpretation
must not be trusted as an outgoing-hit observation solely from it.

The original-client audit, during its existing SHA-1-pinned
temporary Mojang 1.8.9 JAR inspection, now invokes Java 8 javap
on actual `pk` and `pr` classfiles and rejects absent/malformed
inheritance and method descriptors. No game process starts and
the JAR is never committed or uploaded.

The Java 8 disposable exact-name fixtures now model `pr`
extending `pk`, with both read-only methods. No proprietary
original game classes are distributed. R259 additional sampler
coverage follows separately and remains read-only.
