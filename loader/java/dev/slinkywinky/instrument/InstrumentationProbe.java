package dev.slinkywinky.instrument;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.ProtectionDomain;
import java.util.concurrent.atomic.AtomicInteger;

/** Java 8 agent attached after JVM startup. Pass-through probe; no bytecode changes. */
public final class InstrumentationProbe {
    private static boolean permitted(String className, boolean test) {
        return test
            ? "dev.slinkywinky.attach.AttachTarget".equals(className)
            : ("net.minecraft.client.Minecraft".equals(className) || "ave".equals(className));
    }

    public static void agentmain(String options, Instrumentation instrumentation) {
        final boolean test = "probe".equals(options);
        if (!test && !"minecraft".equals(options))
            throw new IllegalArgumentException("Unsupported instrumentation mode");
        if (!instrumentation.isRetransformClassesSupported()) {
            System.err.println("[SlinkyWinkyInstrument] RETRANSFORM_UNAVAILABLE");
            throw new IllegalStateException("JVM does not support live class retransformation");
        }
        Class<?> target = null;
        for (Class<?> type : instrumentation.getAllLoadedClasses()) {
            if (permitted(type.getName(), test)) { target = type; break; }
        }
        if (target == null || !instrumentation.isModifiableClass(target))
            throw new IllegalStateException("Target class not loaded or not modifiable");
        final AtomicInteger seen = new AtomicInteger();
        ClassFileTransformer transformer = new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader, String name,
                    Class<?> type, ProtectionDomain domain, byte[] bytes) {
                if (type != null && permitted(type.getName(), test)) {
                    seen.incrementAndGet();
                }
                // Intentionally no edits: a pass-through capability and class mapping probe.
                return null;
            }
        };
        instrumentation.addTransformer(transformer, true);
        try {
            instrumentation.retransformClasses(target);
            if (seen.get() != 1)
                throw new IllegalStateException("Expected one target transform callback, got " + seen.get());
            System.out.println("[SlinkyWinkyInstrument] INSTRUMENT_RETRANSFORM_PASS class=" +
                target.getName() + " loader=" + target.getClassLoader());
            System.out.flush();
        } catch (Exception error) {
            System.err.println("[SlinkyWinkyInstrument] INSTRUMENT_RETRANSFORM_FAIL: " + error);
            throw new IllegalStateException("Read-only retransformation failed", error);
        } finally {
            instrumentation.removeTransformer(transformer);
        }
    }
}
