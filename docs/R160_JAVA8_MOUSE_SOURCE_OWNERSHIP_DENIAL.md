# R160 - Java 8 mouse event ownership model (dormant)

**Target:** Trexzo/slinkywinky, draft PR #1, reconstruction/r1-core-fidelity. No live original-game mouse interception is enabled.

## Source audit and release gate

The existing R119 read-only audit of SHA-pinned original Mojang Minecraft 1.8.9 (ave.s()V) proves a single Mouse.next() event loop **before** the Keyboard.next() loop, three mouse-button-state reads, one wheel read, 17 direct keyboard-state polls, 13 held-keybinding consumers and three press-binding consumers. Swapping one queue call cannot yield exclusive GUI ownership. No caller of MouseQueueOwner.bind, arm, or next is added to the production agent or native JNI, and the extended R119 regression rejects one if introduced.

R160 adds an original Java 8, source-shaped MouseQueueOwner with exact mocked LWJGL2 Mouse method signatures, one owner thread, explicit separate original classloader/source attestations, monotonic epochs, recent focus heartbeat, full input ownership attestation, 320-8192px bounded viewport, all-controls-released admission and a same-thread pointer sink. Its next() preserves the original queue and metadata in VANILLA mode; in test-only MODAL mode it returns no game events, bounds consumption to 128 per poll, converts bottom-left mouse coordinates into top-left panel pixels, admits only left-button edges/movement, and quarantines wheel/right mouse.

The model refuses unknown buttons, stale coordinate bounds, callback faults, lost foreground, wrong owner, replayed epoch and unauthenticated restoration. GUI close drains held releases before source pass-through resumes. Source failure revokes, requiring independently proven bytecode restoration and an empty source queue before unbinding. This is an **offline component test**, not an input ownership attestation from a commercial game JVM. A boolean supplied by a test must never be construed as an actual host proof.

## Verification

r160_java8_mouse_owner_no_game_hook compiles with Java 8 and executes only a disposable org.lwjgl.input.Mouse mock; the mock class and smoke test are not packaged into the runtime agent. It covers VANILLA pass-through, correct LWJGL-to-client mouse coordinates, rising-edge/repeat handling, movement, wheel withholding, right-button quarantine, close-and-release drain, replay rejection, foreign threads, focus failure, malformed events, callback exceptions, bounded event floods and restoration.

No native JNI receiver is invoked by Java, no original ave.class transformation is installed, no gameplay mouse/keyboard suppression is claimed, and no movement or combat modules become active. Before live integration, the project still needs an authenticated complete original-game input-owner and rollback proof, including keyboard held-binding paths and physical-focus loss testing. The PR remains draft.
