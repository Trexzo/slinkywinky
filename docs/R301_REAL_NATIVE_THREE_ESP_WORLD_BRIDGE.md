# R301 — R300 world data to real C++ ESP controllers via JNI

R301 adds Java8 `EspNativeReadOnlyPreview` and a strict native JNI
transport into **the existing** R297 Chest, R298 Item and R299 Player
ESP source models. It does not create duplicate algorithms or enable
gameplay. Each immutable R300 snapshot is marshalled to bounded arrays,
rechecked natively, and admitted only from the creating owner thread
with monotonic same-world ticks and generation-proven world swaps.

The native session invokes all three C++ controllers and returns a
versioned count-only receipt containing owner tokens, generation,
tick, source entity counts and actual native eligible-row counts.
JNI rejects malformed dimensions, nonfinite values, broken booleans,
unowned sessions, replay and incoherent generation changes. No native
calls receive Minecraft object pointers, UUIDs, or Java game references.

For this first experimental integration the controller settings are
explicit **diagnostic** settings: all four chest types included, all
dropped items eligible, source count merging enabled and no relationship
filter or target coloring. R301 does not imply that original GUI profile
settings have been recovered or applied. Forge preview integration is
separately opted in under R300 isolated TEMP owner admission.

`r301_java8_real_native_three_esp_controllers` uses a Java8 synthetic
receipt through actual JNI and verifies source filters in three C++
controllers, replay denial, generation changes and thread ownership.

**Unfinished:** original Chest event0 vs Item/Player event18 timing,
live rendering, GL teardown, projection and original color policies,
name tags/count-text drawing, GUI profile controls, vanilla injectable
client integration and visual parity. The original decrypted DLL remains
an unauthenticated candidate reference. No PR merge.
