# R52 — class-bound observation cache and atomic session identity

The previous Java 8 observation sampler resolved multiple reflective `Field` members and the player's eye-height `Method` on **every Minecraft tick**. That unnecessarily repeated class-hierarchy searches on the game's client thread and risked added latency once the native client becomes long-running.

R52 replaces that with **bounded class-identity bindings**:

- One current player runtime class: X/Y/Z, yaw, pitch and eye-height method.
- One current ray-hit entity class: X/Y/Z.
- One current mouse-over result class: selected-entity reference.

These are **not unbounded maps**: a change in a player's class, target class, or ray-result class replaces only its corresponding binding. Exact obfuscated vanilla 1.8.9 mapping remains separate from the named/SRG development profile; unknown member names reject sampling instead of guessing. Camera rotations and derived eye coordinates must be finite, as must every target coordinate.

A malformed frame now cannot advance `oldWorld`, `oldPlayer` or the generation counter: session identity is committed **only after all fields have been sampled and validated**. The existing `ObservedState.release()` clears every binding, reflection accessor, and world/player strong reference.

## Offline regression evidence

The required Java 8 smoke fixtures check the following:
- An initial populated mock creates exactly three bindings, followed by 200 same-class observations without any rebind.
- Changing the target class rebinds once without resetting player session generation.
- Changing the player class rebinds once and advances session generation.
- An invalid player-without-world frame leaves identity unchanged; the resumed valid frame advances it exactly once.
- An invalid NaN position cannot produce a valid frame.
- Release empties the cache, disallows sampling until admission, and permits a new clean binding.
- The separate synthetic **obfuscated vanilla** `ave/pk/auh` fixture repeats 200 frames, confirms three stable bindings, clears them, and re-admits successfully.

These tests run in disposable Java 8 processes under the existing Windows CTest/native-attach CI workflow. They never launch Minecraft or access an active game. The R48 runtime acceptance remains historical evidence for earlier code; **R52 has not yet been validated inside a live vanilla game**. Combat actions, packet changes, GUI and native unload remain outside scope.
