# R254 — Free Look event13 offline binding

The reconstructed Java FreeLookModule in the user's verified source
archive subscribes to event13, captures camera yaw/pitch and
third-person view on enable, and restores them on disable/unload.
The decrypted original native registration excerpt confirms
Free Look enum settings but does NOT establish the event13 priority.
The tests' priority 0 is explicitly a fixture, not verified native data.

PR #2 now binds Free Look event13 through the existing typed
ModuleExecutionBridge. Clutch and Block In can share this offline
event13 dispatch without overwriting each other's proposal payloads.
The native rotation arbitration policy is NOT reconstructed here.

Enable, disable and unload return value-only proposals when the
module is registered, bound and enabled. Applying a disable without
an owner-bound restore discards the stored camera state rather
than changing a different player/world. Shared RNG dispatch errors
roll back Free Look along with the existing Clutch/Block In snapshot.

A CMake/CTest target checks binding metadata, multi-module event order,
lazy activation, idempotent repeated callbacks, explicit unload,
wrong-owner cleanup, disable/re-enable and stop.

No live camera, mouse, player rotation, render or packet writes exist.
The remaining release gate is exact client-side event13 provenance,
settings parity and isolated Minecraft 1.8.9 acceptance.
