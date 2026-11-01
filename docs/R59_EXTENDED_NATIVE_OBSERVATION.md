# R59 — explicitly bounded native observation windows (no Forge)

**Status:** source implemented and deterministic test coverage added; CI and independent in-game validation must be assessed separately.

R57's injected C++ engine exposes useful, immutable HUD observations but the Java 8 experimental hook closes after exactly 20 client-thread callbacks. R59 retains this default and makes two **finite, opt-in read-only** windows possible without claiming persistent combat, graphics, networking, packet edits or native agent unload.

## Native standalone loader

Default, unchanged:

```powershell
$env:SLINKYWINKY_EXPERIMENTAL_HOOK = '1'
.\slinkywinky_loader.exe attach <explicit-Minecraft-PID> '<JDK8_HOME>' --hook-probe
```

Extended, separately authorized **in the attaching process**:

```powershell
$env:SLINKYWINKY_EXPERIMENTAL_HOOK = '1'
$env:SLINKYWINKY_EXTENDED_DIAGNOSTIC = '1'
.\slinkywinky_loader.exe attach <explicit-Minecraft-PID> '<JDK8_HOME>' --hook-probe=120
# Or, for at most 600 callbacks: --hook-probe=600
```

No broad or arbitrary duration is admitted. The launcher rejects non-numeric / out-of-list values (including 0, 20 as a redundant suffix, 601, negative values and "unlimited"). The Java 8 helper requires the existing `SLINKYWINKY_EXPERIMENTAL_HOOK=1` and, for either longer window, **also** requires `SLINKYWINKY_EXTENDED_DIAGNOSTIC=1` before Java Attach API is invoked.

The two optional durations are *maximum callback counts*, not promises of wall-clock duration or frame rate. In the commonly observed ~20-tick/s Minecraft runtime, 120 callbacks is roughly 6 seconds and 600 callbacks roughly 30 seconds, but this is not a timer guarantee. On an invalid sample or native failure, the callback aborts early.

The optional R57 local HUD IPC requires **the target Minecraft JVM** to have `SLINKYWINKY_HUD_IPC=1` in its own environment before launch; setting it only on the attaching helper is not sufficient. HUD IPC is read-only and may miss a sample instead of blocking the game thread. It is neither an in-game ClickGUI nor a certified long-running overlay.

## Lifecycle invariants

- Explicit PID and recognized target game signature, no stealth or arbitrary JVM scanning.
- An exact single class and void tick mapping, one native decision context owned by the client thread.
- Strictly zero gameplay actions (input, rotation, attack, reach, packet changes all absent).
- The existing state validation, thread owner checks, generation/clock checks and native read-only snapshot rules remain in force.
- At the chosen callback limit or invalid sample, mark inactive; close the C++ context on the owning thread; release game/reflection references; asynchronously retransform the target class and validate the *pre-injection SHA-256 fingerprint*.
- Any failed restoration retains a no-reattach lock; do not claim complete native agent unloading merely because the Java hook has been restored.
- Two successive bound sessions may be admitted only after the first hook fully cleans up.

## CI and acceptance

The existing disposable Java 8 attach regression now exercises:
1. Default 20-tick attachment and original class restoration.
2. Refusal of 120-tick attachment without the *separate* extended opt-in.
3. Refusal of arbitrary/unsupported `--hook-probe=999`.
4. Opted-in second attachment for exactly 120 callbacks and a second bytecode restoration.
5. Deterministic unit rejection of unlisted durations in `HookAdmissionSmoke`.

The CTest runs using a disposable mapped Java target and must never launch Minecraft from hosted CI. Original vanilla Minecraft runtime acceptance in R48 applies to older source only and must be repeated against R59 before claiming in-game compatibility. Actual world/target observation, HUD lifespan, and full native ClickGUI are still separate milestones. All proprietary/decrypted materials stay offline and uncommitted.
