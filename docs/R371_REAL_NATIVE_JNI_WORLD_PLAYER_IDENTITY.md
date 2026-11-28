# R371 — Real original world/player epoch tokens in native JNI

The original standalone Lag Range bridge previously used two fixed
placeholder IDs (1001,1002) for native attack/event15, tick/event18
and outbound packet/event3. This was insufficient to distinguish
new world/player epochs.

R371 allocates strictly positive, unique, monotonically increasing
**primitive token pairs** after a fully validated original
Minecraft read-only observation. Tokens remain stable when the
ray target changes, and are regenerated after any world/player
identity change, disconnect or rejoin. The token sequence is
never reset by release/re-attach within the same game JVM,
and overflow fails closed. Null world/menu epochs use distinct
opaque tokens too; tokens are NOT proof of a real loaded world.
`Object.hashCode` and `identityHashCode` are never used.

Normal original click return sampling exports the SAME stable
world/player IDs in `AttackClockFrame`. The real native C++
Lag Range event15, event18 and event3 callbacks now share the
actual game's current Java-sampled identity labels rather than
hardcoded placeholders. No Java world/player/entity/Packet
references cross JNI; only primitive long tokens.

Both named synthetic and original obfuscated Java8 mapping
regressions now verify token positivity and stability across
repeats/target loss, generation change, disconnect, reentry,
invalid samples and reattach. This improves the JNI ownership
contract but does NOT prove original on-wire timing,
packet retention or full Slinky fidelity.
