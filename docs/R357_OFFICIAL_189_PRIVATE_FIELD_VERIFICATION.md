# R357 — SHA-pinned Mojang 1.8.9 private field validation

The R355/R356 original-candidate `rdata.s` / `jni_ids.cpp`
indicate, for variant=1 (1.8.9), obfuscated
`EntityLivingBase=pr` and `EntityOtherPlayerMP=bex`.
The corresponding original candidate JNI primitive field mappings are
`pr.bl:I` (`lastAttackerTime`) and `bex.c:D`, `bex.d:D`,
`bex.e:D` (`otherPlayerMPX/Y/Z`). The earlier R355 Java fixture
verified the *candidate mapping table*, not the shipped Mojang bytes.

R357 adds an **independent, read-only** JVM ClassFile metadata parser
to the existing official-original 1.8.9 GitHub audit pipeline.
It uses only the official Mojang JAR already downloaded temporarily
by `tests/official_189_render_audit.py`, admitted by exact
SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908` and
file size 8,461,484 bytes. It parses only `pr.class` and
`bex.class`, verifies exact class owners and field types,
prints metadata-only PASS markers, and writes no original bytecode,
decrypted asset, JAR or game classes into the Git repository.

Synthetic independent negative controls deny wrong primitive types,
wrong class owner, missing or duplicate fields, truncated and invalid
classfiles. The existing original-render audit and vanilla binary
deletion remain unchanged.

**Important:** this proves or falsifies the exact *vanilla original*
field names, not that Forge/multiplayer/private reobfuscation mappings
exist or a live JVM allows reflection. No gameplay, packet interception,
native injection or complete original client is established.
PR #2 remains draft; PR #1 unchanged; verify exact-head CI before
claiming the original-jar audit passed.
