# R57 — opt-in local Windows cross-process read-only HUD transport

The R55–R56 JNI engine already publishes validated, zero-action telemetry to an in-process, one-record mailbox. The standalone Win32 GDI preview previously showed **synthetic** data only because it ran in another process.

R57 adds **optional** native Windows IPC for a real local HUD preview:
- **Publisher:** native JNI session creates one PID-scoped `Local\\SlinkyWinky-HUD-v1-*` shared memory object and a named mutex **only** when the target JVM has `SLINKYWINKY_HUD_IPC=1` in its environment. Without this explicit opt-in, default behavior remains unchanged and no shared memory is created.
- **Consumer:** `slinkywinky_hud_preview.exe --pid <Minecraft-JVM-PID>` uses `OpenFileMappingW(FILE_MAP_READ)`, so the preview maps only a read-only view. It reads a fixed-size structure with a version/magic/PID, last publication time and ten numeric HUD fields; the old standalone synthetic mode remains the default when `--pid` is omitted.
- **No bidirectional commands:** the consumer has no write mapping, remote process handles, packet access, input controls, game pointers, or renderer hooks. There is no network listener. Local Windows object ACLs still matter; HUD telemetry is not intended to be sensitive or authoritative.
- **Safety:** producer takes a mutex without waiting on the Minecraft game thread; skipped publications never block a valid engine tick. Reader copies under the same mutex. Unknown formats, invalid fields, inactive records or timestamps over 1500 ms old are rejected. Publisher revokes the record on normal session close. A crashed publisher leaves at most an expiring record.
- **Lifecycle limitation:** the existing experimental hook still runs only 20 ticks before restoring the original class method. This preview is therefore short-lived while diagnostic admission remains bounded. The viewer must be opened with a known PID, which can be obtained through the existing standalone loader's `list` command.

## Tests

The Windows-only `windows_cross_process_hud_ipc` CTest creates an opted-in publisher, starts a **second process** and verifies its read-only observation, checks wrong-PID refusal, duplicate publisher rejection and session revocation. The Java 8 attachment regression additionally sets `SLINKYWINKY_HUD_IPC=1` for **one disposable test JVM** and verifies that real JNI `nativeObserveFrame` data is visible to a separate Windows reader process. The rest of the default Java tests run without IPC enabled. CI does not launch Minecraft.

**Not yet verified:** actual vanilla Minecraft runtime with this R57 transport, graphics overlay inside the Minecraft render context, persistent running hook, full native client GUI or gameplay feature fidelity. No commercial/reference binary or game asset is included.
