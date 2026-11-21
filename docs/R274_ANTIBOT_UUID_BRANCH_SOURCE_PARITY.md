# R274 — critical Anti Bot UUID predicate fidelity correction

Rechecked directly against the supplied verified `AntiBotModule.java`
inside `Slinky-Verified-SourceBuild.zip`. The reconstructed Java has
the condition `mode==0 || ((int)msb & 61440)!=8192 ||
(mode!=2 && !(1.0>travelledSquared))` for the **allow/proceed**
branch, not the reject branch.

Its meaning is intentionally counterintuitive: mode1 rejects the
UUID version nibble **equal** to 0x2000 until movement accumulates
at least one squared unit; mode2 rejects equality even after movement.
A nibble **different** from 0x2000 proceeds through the UUID filter.

R270's original C++ model had inverted this condition and its own
test reinforced the bug. R274 corrects the C++ condition, adds a
complete 3 modes × 2 nibble classes × 2 movement states truth table,
and corrects the R271 combined Teams+AntiBot test fixture.
No native Minecraft entity or RelationshipService fields are modified.

This milestone illustrates why isolated model tests can pass while
authentic source fidelity fails: each decision branch must be compared
with the recovered Java and/or original binary, not inferred from
settings names.
