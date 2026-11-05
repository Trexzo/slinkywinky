# R55 — thread-safe client-to-render HUD snapshot channel

The R53 observation ledger deliberately rejects reads from threads other than the Minecraft **client thread**. A rendering/UI thread therefore needs a copied, immutable, bounded telemetry snapshot rather than calling the client engine or holding entity pointers.

R55 introduces a first-party C++17 **HudSnapshotMailbox** for each bounded native observation session. The owner thread can publish only after a fully validated frame returns a **zero-action** result from the C++ engine. Other threads read coherent copies under a short mutex, including a monotonically increasing sequence number. A mailbox never stores Minecraft objects, Java references, pointers or packet data.

- A publication is blocked if the creator thread differs, flags/coordinates/angles are invalid, time or generation rolls back, or accepted tick counts fail to increase.
- Reads are permitted on other threads; publishes and revocation remain owner-thread-only.
- The mailbox has fixed capacity **one current status**. No unbounded event queue and no stale-frame backlog.
- Closing a native observation context revokes its mailbox and removes the opaque token from the existing bounded session registry. Forged, double-closed, revoked and unknown tokens cannot retrieve HUD telemetry.
- Java 8 JNI adds a **read-only** `nativeReadPublishedStatus(long)` returning nine status values and a tenth publication sequence. This is a same-process interface. Existing owner-thread-only `nativeReadObservationStatus` remains unchanged.

Tests: a separate portable C++ mailbox suite publishes 3,000 observations under four concurrent readers, confirms coherent snapshots, rejections, a usable `HudPanel`, revocation and wrong-thread publication denial. The disposable Java 8 JNI test verifies cross-thread publication reads, coherence with the original owner-only ledger, 2,000 rejected wrong-thread engine calls during accepted owner-thread updates, and cleanup of registry snapshots.

This is **not** yet a Minecraft rendering hook or a link from the **standalone preview EXE** to a game process. The Windows R54 preview still uses synthetic data, and the R55 Java-to-native bridge has not received a fresh in-game acceptance. No combat/input/packet actions are enabled.
