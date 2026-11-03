# R73 — loaded original 1.8.9 renderer passive admission

**PR #1 remains draft.** The R73 feature is a separate **opt-in, read-only JVM classfile preflight**, not a live render callback or working in-game ClickGUI.

## What it establishes

R71 found original vanilla 1.8.9's `ave.av → bfk.a(FJ)V` from its Mojang SHA-1-verified original JAR. R72 proved the planned ASM method-entry prefix can be inserted **offline** without altering the original method's instruction trace. Neither confirmed that an actual running Minecraft instance loaded the original renderer classes or that an OpenGL context is owned by a safe render thread.

R73 adds `--render-preflight` to the standalone native loader. The Java 8 Attach helper requires **both** an explicitly selected Minecraft-candidate JVM PID and the separate operator opt-in `SLINKYWINKY_RENDER_PREFLIGHT=1`. The preflight does not use the tick probe or native JNI bridge.

In the target JVM, the agent:
1. Acquires the existing single-instrumentation-session guard. It refuses an overlapping owner instead of disrupting an active tick hook.
2. Finds **exactly one loaded** `ave` and `bfk` class; requires both modifiable and loaded by the exact same system classloader or recognized LaunchWrapper classloader.
3. Installs a temporary, passive Java `ClassFileTransformer` that **always returns `null`** and takes immutable byte copies only for the two target classes. It requests retransformation for those two classes once, then unconditionally removes the passive transformer.
4. Requires exactly one callback per class; verifies the full unmodified classfile SHA-256 values from R71, class names, the `ave` instance renderer field, `ave.av` virtual invocation and one public instance `bfk.a(FJ)V` method.
5. Logs `R73_LOADED_ORIGINAL_VANILLA_189_RENDERER_PREFLIGHT_PASS` only after all checks pass. An opt-in attempt on a game with transformed/unknown class bytes must **fail closed**.

Production admission always uses the pinned original class hashes; the configurable-hash helper exists solely for generated fixture tests and cannot change these required values. Existing OpenGL preview, JNI and runtime combat modules are untouched.

## Manual acceptance (authorized vanilla 1.8.9 only)

- Use original Java 8 vanilla 1.8.9, launch into a menu/world, and identify its **specific** PID with `slinkywinky_loader.exe list JDK8_HOME`.
- From the same Windows command environment that invokes the loader, explicitly set `SLINKYWINKY_RENDER_PREFLIGHT=1`. Then execute `slinkywinky_loader.exe attach PID JDK8_HOME --render-preflight`.
- Check the selected JVM's own log for `R73_LOADED_ORIGINAL_VANILLA_189_RENDERER_PREFLIGHT_PASS`, the no-hook marker, and the loader's attach result. Record the exact original class hashes and JVM runtime identity. Do not assume the loader's success line alone proves renderer admission; inspect the target JVM's diagnostic log.
- Repeat the denial on a different or modded client **only in an expendable test JVM**; do not use a real gameplay session to force unsafe diagnostics.

The desktop was offline when this code was developed, so a real in-game R73 acceptance is **not performed**. The passive retransformation may cause a brief JVM safepoint, and unfamiliar agents or launchers may have other transformers—reject them rather than silently claiming compatibility.

## CI gates

- Windows Java 8 CTest uses synthetic ASM fixtures to test valid structure, incorrect hashes, invalid owners, wrong method access, duplicated virtual render calls, corrupt bytes and unrelated-JVM denial. It also checks disabled-by-default operator gate and non-Minecraft PID refusal.
- The separate official-client audit downloads Mojang's version-pinned vanilla original into a temporary directory, independently confirms SHA-1/size, then validates R73's **same static original-class policy** against original bytes. No Minecraft artifacts or transformed bytes are committed or uploaded.
- Independent C++ core, Windows injector and JNI regression workflows remain required.

**R73 does not yet certify:** actual callback execution, render-thread identity, WGL/LWJGL state, overlay visibility in Minecraft, graphics teardown, settings activation, original Slinky module behavior or proprietary UI parity. Those gates belong to later explicitly authorized live-game work. No silent production hook enabled.
