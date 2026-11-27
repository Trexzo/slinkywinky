# R359 — Standalone Windows loader CLI for original native source tick

R358 proved the first read-only original client tick -> native
Lag Range event18 pathway, but its original enablement required
`-Dslinkywinky.r358OriginalSourceTickReadOnly=true` before Minecraft
started. R359 adds an **explicit, finite, operator-initiated** 
`slinkywinky_loader.exe attach PID JDK8_HOME --source-tick-readonly`
option; `--source-tick-readonly=600` is a separately gated
longer bounded acceptance window.

The existing Windows loader validates only the two exact option
spellings and requires its expected Java8 JDK, instrumentation JAR
and exact standalone `slinkywinky_jni.dll`. The Java8 Attach helper
refuses this mode unless the operator set BOTH
`SLINKYWINKY_EXPERIMENTAL_HOOK=1` and
`SLINKYWINKY_SOURCE_NATIVE_READONLY=1` in the launcher environment.
600-tick mode additionally requires
`SLINKYWINKY_EXTENDED_DIAGNOSTIC=1`.
Synthetic Attach-test mode, unknown options, other JVMs, wrong JNI
library and missing game-class fingerprint are rejected.

The new agent option `minecraft-source-readonly` is admitted only
after the pre-existing single instrumentation-owner fence, and
its callback transformation requires the exact pinned original
Mojang 1.8.9 `ave.class` SHA-256. It forwards finite original
tick observations into the existing read-only native e18 source
controller, never invokes packet event3 on actual network traffic,
and never mutates game inputs, timing or positions.

R359 changes the `SourceRuntimePreview` JNI facade from eager
`System.loadLibrary` into a lazy load with an explicit
`admitPreloadedLibrary()` handshake. The vanilla Java8 agent calls
that only **after** it loaded the absolute DLL and
`nativeZeroActionTick()==1`. Existing Forge/Java8 JNI constructors
continue loading the library by name when no authorized preload
was admitted. No process-wide system properties or duplicate
native DLL loads are required.

**This is still a diagnostic and not a working full ghost client.**
No original event18 timing, original target acquisition, packet hold,
attack phase, live gameplay or ClickGUI/ArrayList parity is claimed.
PR #2 remains draft; PR #1 unchanged.
