# R280 — prove R277/R278 roster classes ship in the Forge JAR

The separate Experimental Forge 1.8.9 JAR build uses authentic
ForgeGradle 2.1, Minecraft 1.8.9 mappings and JDK8. The R278
exact-commit workflow successfully compiled/reobfuscated its
preview JAR. However the previous package check looked for only
ForgePreview.class, not the newly introduced relationship sampler.

R280 now forces this Forge build to run whenever
ForgeRelationshipRosterPort.java or RelationshipRosterSampler.java
changes, and rejects the JAR unless all three classes are packaged:

- dev/slinkywinky/forge/ForgePreview.class
- dev/slinkywinky/forge/ForgeRelationshipRosterPort.class
- dev/slinkywinky/RelationshipRosterSampler.class

This proves compilation and packaging with actual pinned Forge
dependencies; it is distinct from the separate Java-only R276
unit suite and synthetic R279 stub suite.

No actual Minecraft 1.8.9 process is launched in GitHub CI.
No claim of an observed R278 receipt or working ghost-client gameplay
is made. A disposable live game launch is still the necessary gate.
