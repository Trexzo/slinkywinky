# R361 — SHA-pinned original Minecraft attack-callback mapping

The recovered private candidate maps `Minecraft_clickMouse` through
`ptr_to_str_al` (original 1.8.9 variant 1) to `ave.aw()V`.
This is **not** the same as the original Slinky `PhaseHook`; it is a
Minecraft client mouse-attack method that may return without an entity hit.

R361 extends R357's read-only, strict SHA-1 and size-pinned Mojang
1.8.9 JAR audit to parse JVM `method_info` and bounds-check
`Code` attributes, verifying that `ave.aw()V` and existing
`ave.s()V` are concrete instance methods with executable Code.
The class name, descriptor, access flags and uniqueness are all enforced.
Synthetic control tests reject wrong owner, missing tick, wrong method
name and descriptor, and malformed metadata.

The audit reads only the two exact original method names from the
ephemeral official Minecraft JAR. Original JAR/classes/bytecode
remain outside Git and are deleted after the audit.

**Limits:** Static mapping alone does not establish exact source phase
1, a successful entity attack, a safe live injector callback, real
packet ownership or full original Slinky parity. Next step is a
separate finite original-only click-exit observer with game-thread
ownership and double-dispatch prevention, contingent on verified CI.

PR #1 unchanged; PR #2 draft/unmerged.
