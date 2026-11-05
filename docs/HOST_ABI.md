# R8 explicit game-host contract (Java 8)

GameHost is an opt-in bridge for a host-owned Minecraft client integration.
It neither injects, launches, hooks, reads process memory nor acquires Minecraft instances.
An actual Minecraft/Forge adapter must supply observations on the client thread and
apply resulting decisions using only permitted game APIs.

## Input format

Native clocks (long[2]): monotonic milliseconds, world/player generation.

Native flags (int[12]): valid-world, gui-open, mouse-left physical,
mouse-right physical, weapon-held, sprinting, hurt-time, last-attacker-time,
valid-target-present, target-visible, target-alive, target-id.

Native coords (double[6]): player eye XYZ, target XYZ; rotation (float[2]): yaw/pitch.

Native settings (float[19]): clicker enabled, mouse requirement, CPS,
aim enabled, aim mouse requirement, aim range, horizontal speed, vertical speed,
aim FOV, AutoBlock enabled, require damage, block range, max hurt time,
Reach enabled, Reach distance, Reach activation ticks, sprint requirement,
HitSelect enabled, minimum hurt time.

Native decision (float[10]): left edge, right edge (0 none/1 press/2 release),
left held, right held, attack pulse, rotation present, yaw, pitch,
Reach present, reach distance.

**Outputs are decisions only.** The host is responsible for applying actions safely,
respecting GUI/manual-input state, and never forwarding this data to network packet codecs
without separate admission and authority checks.

## Guarantees tested

- GameHost accepts only its original Java thread.
- New world identity or player identity increments generation and releases synthetic inputs.
- World leave, action exception, time reversal and shutdown release synthetic inputs.
- The host can't use a closed engine and never applies a stale decision after a failed call.
- JNI validates vector dimensions and non-finite positions/configuration.

## Not yet verified

- Forge/Minecraft 1.8.9 linkage, API mappings, real input-edge application.
- Exact original Slinky behavior, rendering, protocol parsing and in-game combat parity.
- Actual game reconnect/reinjection and platform-specific native packaging.

## Settings admission (R9)

HostSettings validates every incoming field and provides a defensive-copy immutable profile.
Safe defaults use candidate-reference numeric values with all action modules explicitly disabled.
The host rejects non-finite, out-of-range and nonboolean settings before JNI or game actions.
