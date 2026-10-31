# R34: child-first LaunchWrapper classloader admission regression

R32's first real Minecraft hook attempt crashed after its Java bytecode was transformed: Minecraft's `LaunchClassLoader` could not resolve `dev.slinkywinky.instrument.InstrumentationProbe`. This produced a `NoClassDefFoundError` on the first tick. The test remains documented as a **failure**.

R33 adds an explicit preflight: with Minecraft's known LaunchWrapper class loader, register `dev.slinkywinky.instrument.` through its `addClassLoaderExclusion` method so the parent system classloader can resolve the exact existing agent class. Verify identity before retransformation; fail closed otherwise.

R34 adds a child-first classloader simulation to CI. It deliberately refuses agent-class visibility initially, runs the preflight, then asserts that the resolved callback is the exact same JVM Class object. This validates the class-loader fix offline. It is not proof that the R33 change has passed inside real Minecraft; repeat isolated in-game acceptance before enabling any gameplay modules.
