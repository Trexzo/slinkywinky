# R355 — Minecraft 1.8.9 source JNI private field mapping repair

R349 and R353 used read-only reflection of
`EntityLivingBase.lastAttackerTime` and
`EntityOtherPlayerMP.otherPlayerMPX/Y/Z`.
They previously looked up **only the deobfuscated development
names**, which may not exist after ForgeGradle reobfuscation.
A successful ForgeGradle compilation cannot prove runtime reflection
field ownership or access.

The retained private candidate `c++_possibly_OpenSlinky-main.zip`,
`src/data/rdata.s` and `src/engine/gui/jni_ids.cpp`,
explicitly expose the JNI name/type table for the **1.8.9** variant:

| Field | MCP | SRG | 1.8.9 obfuscated | Java type |
| --- | --- | --- | --- | --- |
| last attack time | `lastAttackerTime` | `field_142016_bo` | `bl` | `int` |
| remote X | `otherPlayerMPX` | `field_71185_c` | `c` | `double` |
| remote Y | `otherPlayerMPY` | `field_71182_d` | `d` | `double` |
| remote Z | `otherPlayerMPZ` | `field_71183_e` | `e` | `double` |

The older-source variant uses different obfuscated names; mixing
versions is not acceptable. The independent Java 8 helper
`SourceCandidateField189` accepts only these exact names and
primitive types, rejects ambiguous mixed namespaces, missing,
unrecognized or inaccessible fields, and never substitutes arbitrary
similar game fields.

`ForgeSourceWorldPort` now uses the same resolver for both
R349 Backtrack reach evidence and R353/R354 Lag Range attack
evidence. The existing actual JNI source admission remains
**opt-in/read-only**, fail-closed if any required field is unknown.

`r355_java8_source_mc189_private_field_mappings` tests all three
complete namespaces, private int/double access, conflicting/missing
and wrong-type fields, old-version obfuscated names and unknown owner.
ForgeGradle also checks the helper is packaged with the read-only mod.

Still missing: real injected-client runtime, original event/packet
timing, actual original gameplay and positive local-world field
acceptance receipt; PR #2 stays draft and unmerged.
