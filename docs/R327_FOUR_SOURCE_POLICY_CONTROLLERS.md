# R327 — four candidate-source packet and sprint policies

Reference: the user's **c++_possibly_OpenSlinky-main.zip** and decrypted archive, kept outside Git. Baseline R326 `0f98ce355854987d7e091d76a250c9e2c6e6a9af`.

Added independent source-informed C++17 offline controllers, with adversarial CTest:
- Blink: direction-specific incoming/outgoing verdicts, feedback bypasses, countdown, dig/interact and attack proposals, world reset.
- Fake Lag: independently budgeted incoming/outgoing delay, pulse windows at 50ms tick increments, attack-triggered 20-tick eligibility, weapon/world gating and damage-feedback bypass. Newly detected attacks do not turn the budget on until the following tick, matching the candidate callback ordering.
- Sprint Reset: 3-state idle/wait/key-changed sequence, delayed physical-key release/restore, mode-specific sprint/sneak behavior, optional supplied random-walk offsets and world cleanup.
- Knockback Delay: candidate late-tick target/weapon/mouse/air eligibility, own-entity velocity packet, held transaction fencing, maximum delay, hurt feedback and batch-mismatch release.

All packet types and player/key states are **value-only inputs**, and outputs are **proposals**. They do not hold/send a real packet, synthesize input, modify the Minecraft client, or connect to the existing live native manager. The C++ candidate is a reconstruction of uncertain authenticity, not proven byte-equivalent proprietary source. The original random-walk functions are not reconstructed: randomized decisions are supplied as explicit test inputs. Safety-validation guards may differ from raw candidate behavior for invalid settings.

Acceptance: the new CTest plus existing R316 standalone-controller-header compilation, CI matrix and independent original-client runtime verification. No live gameplay, 1:1 fidelity, or standalone injectable completion claimed. PR #1 remains untouched; PR #2 stays draft/unmerged.

Further required: source-accurate sampling and JNI ownership, active module toggles, bounded network queue with full lossless close/flush semantics, physical-input restoration, and real-world test/GUI synchronization. The existing R315 gap ledger is historical; count coverage from the current tree rather than incrementing that document blindly.
