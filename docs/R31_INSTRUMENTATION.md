# R31: Java 8 post-startup Instrumentation proof (no Forge)

An additional, **opt-in** late-attach path uses Java 8 `VirtualMachine.loadAgent` and the public `java.lang.instrument` API rather than unsupported late JVMTI breakpoint capabilities. The C++ agent and standalone executable remain the primary native loader.

Run `slinkywinky_loader.exe attach PID JDK8_HOME --instrument-probe` to test an already-running Minecraft JVM. The bundled agent JAR requests only `Can-Retransform-Classes: true`. It selects an actual loaded `net.minecraft.client.Minecraft`/`ave` class, installs a Java ClassFileTransformer that returns **null (no bytecode edits)**, invokes `retransformClasses`, checks the callback, and immediately removes the transformer.

A smoke test uses `SLINKYWINKY_ATTACH_TEST=1` and a disposable Java8 `AttachTarget`. That test has a matching mode and verifies the late instrumentation attachment. Normal release mode rejects non-Minecraft JVMs.

**Limits:** A successful read-only retransformation is not yet a game tick event and does not create a usable Slinky client. It is a prerequisite for implementing an exact-class tick callback on a dedicated opt-in path; there is no input, combat action, packet processing, GUI or client unload functionality. Forge is not a dependency.