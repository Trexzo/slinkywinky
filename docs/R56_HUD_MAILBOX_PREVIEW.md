# R56 — standalone native HUD preview consumes the actual thread-safe snapshot API

R55 introduced a bounded, per-session `HudSnapshotMailbox` so a future native render/UI thread can read a copy of telemetry produced only by Minecraft's admitted client thread. This avoids the data race of reading the C++ `ObservationLedger` directly from an unrelated render thread.

R56 connects that mailbox to the existing **standalone Win32/GDI preview window**:
- A dedicated producer thread creates and owns a mailbox, publishes bounded, validated **synthetic** observation updates at 50 ms intervals, and revokes them before exiting.
- The Win32 GUI message/paint thread only reads immutable mailbox snapshots and formats them through the original C++ `make_hud_panel` model.
- State selection (keys 1–4) is transferred through an atomic variable; GUI-only compact/angle toggles remain on the GUI thread.
- A mutex-protected shared mailbox reference prevents lifetime/use-after-free races between producer and consumer; the mailbox itself protects its copied status.
- Closing the preview stops the producer, joins the worker and ensures its snapshot is revoked. No detached thread remains.
- The producer counts world/player availability changes as synthetic session transitions, ensuring the display exercises the state-change path rather than fixed counters.

## Acceptance boundaries

The standalone preview remains entirely **offline**. It does not attach to Minecraft, inject any module, draw into the game's graphics context, send mouse/keyboard/packets, or inspect processes. Its sample data is **simulated**, although its cross-thread mailbox is the same first-party C++ component used by the JVM's live observation bridge. An actual game HUD still requires a separately verified render-thread graphics lifecycle.

Portable C++ unit tests and disposable Java 8 JNI tests verify the mailbox; GitHub Windows CI compiles the native GDI preview into a downloadable EXE. Building the window does **not** establish successful on-screen appearance: the desktop is offline and no graphical launch was performed in this milestone.
