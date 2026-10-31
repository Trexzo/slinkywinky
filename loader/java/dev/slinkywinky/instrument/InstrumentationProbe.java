package dev.slinkywinky.instrument;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.ProtectionDomain;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** Opt-in, read-only, bounded Java 8 tick-hook experiment. */
public final class InstrumentationProbe {
    private static final String PREFIX = "[SlinkyWinkyInstrument] ";
    private static final AtomicInteger ticks = new AtomicInteger();
    private static final AtomicBoolean cleanupStarted = new AtomicBoolean();
    private static final AtomicBoolean hookInstalled = new AtomicBoolean();
    private static volatile boolean active;
    private static volatile Instrumentation installedVia;
    private static volatile Class<?> installedClass;

    private static boolean recognized(String name, boolean test) {
        return test ? "dev.slinkywinky.attach.AttachTarget".equals(name) :
            ("net.minecraft.client.Minecraft".equals(name) || "ave".equals(name));
    }
    private static boolean isTickMethod(String method, boolean test) {
        return test ? "tick".equals(method) :
            ("runTick".equals(method) || "func_71407_l".equals(method));
    }

    /** Runs on the actual target thread. Never changes Minecraft state. */
    public static void onGameTick() {
        if (!active) return;
        final int n = ticks.incrementAndGet();
        if (n == 1) {
            System.out.println(PREFIX + "GAME_THREAD_TICK_ENTRY_PASS thread=" +
                Thread.currentThread().getName());
            System.out.flush();
        }
        if (n >= 20 && cleanupStarted.compareAndSet(false, true)) {
            active = false;
            System.out.println(PREFIX + "GAME_THREAD_TICK_20_PASS ticks=" + n);
            System.out.flush();
            final Instrumentation instrumentation = installedVia;
            final Class<?> target = installedClass;
            Thread cleanup = new Thread(new Runnable() {
                @Override public void run() {
                    try {
                        instrumentation.retransformClasses(target);
                        System.out.println(PREFIX + "GAME_THREAD_HOOK_CLEANUP_PASS");
                    } catch (Throwable problem) {
                        System.err.println(PREFIX + "GAME_THREAD_HOOK_CLEANUP_FAIL " + problem);
                    } finally {
                        installedVia = null;
                        installedClass = null;
                        hookInstalled.set(false);
                    }
                }
            }, "SlinkyWinky-ReadOnly-HookCleanup");
            cleanup.setDaemon(true);
            cleanup.start();
        }
    }

    public static void agentmain(String options, Instrumentation instrumentation) {
        final boolean test = "probe".equals(options) || "hook-probe".equals(options);
        final boolean hook = "hook-probe".equals(options) || "minecraft-hook".equals(options);
        if (!test && !"minecraft".equals(options) && !hook)
            throw new IllegalArgumentException("Unsupported instrument mode");
        if (!instrumentation.isRetransformClassesSupported())
            throw new IllegalStateException("Live retransformation unavailable");
        Class<?> target = null;
        for (Class<?> c : instrumentation.getAllLoadedClasses()) {
            if (recognized(c.getName(), test)) { target = c; break; }
        }
        if (target == null || !instrumentation.isModifiableClass(target))
            throw new IllegalStateException("Target class not loaded or unmodifiable");
        final AtomicInteger seen = new AtomicInteger();
        final AtomicInteger modified = new AtomicInteger();
        ClassFileTransformer transformer = new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader, String name, Class<?> type,
                                               ProtectionDomain protection, byte[] bytes) {
                if (type == null || !recognized(type.getName(), test)) return null;
                seen.incrementAndGet();
                if (!hook) return null;
                ClassReader reader = new ClassReader(bytes);
                ClassWriter writer = new ClassWriter(reader, ClassWriter.COMPUTE_MAXS);
                final AtomicInteger exact = new AtomicInteger();
                ClassVisitor visitor = new ClassVisitor(Opcodes.ASM5, writer) {
                    @Override public MethodVisitor visitMethod(int access, String method,
                            String descriptor, String signature, String[] exceptions) {
                        MethodVisitor downstream =
                            super.visitMethod(access, method, descriptor, signature, exceptions);
                        if (isTickMethod(method, test) && "()V".equals(descriptor) &&
                            (access & (Opcodes.ACC_STATIC | Opcodes.ACC_ABSTRACT)) == 0) {
                            exact.incrementAndGet();
                            return new MethodVisitor(Opcodes.ASM5, downstream) {
                                @Override public void visitCode() {
                                    super.visitCode();
                                    super.visitMethodInsn(Opcodes.INVOKESTATIC,
                                        "dev/slinkywinky/instrument/InstrumentationProbe",
                                        "onGameTick", "()V", false);
                                }
                            };
                        }
                        return downstream;
                    }
                };
                reader.accept(visitor, 0);
                if (exact.get() != 1)
                    throw new IllegalStateException("Exact tick mapping unavailable: " + exact.get());
                modified.incrementAndGet();
                return writer.toByteArray();
            }
        };
        if (hook && !hookInstalled.compareAndSet(false, true))
            throw new IllegalStateException("Read-only hook already installed");
        if (hook) {
            ticks.set(0);
            cleanupStarted.set(false);
            installedVia = instrumentation;
            installedClass = target;
            active = true;
        }
        instrumentation.addTransformer(transformer, true);
        try {
            instrumentation.retransformClasses(target);
            if (seen.get() != 1 || (hook && modified.get() != 1))
                throw new IllegalStateException("Expected exactly one transformed target");
            System.out.println(PREFIX + (hook ? "GAME_THREAD_HOOK_INSTALLED" :
                "INSTRUMENT_RETRANSFORM_PASS") + " class=" + target.getName());
            System.out.flush();
        } catch (Throwable problem) {
            active = false;
            if (hook) {
                installedVia = null; installedClass = null;
                hookInstalled.set(false);
            }
            System.err.println(PREFIX + "INSTRUMENT_TRANSFORM_FAIL: " + problem);
            throw new IllegalStateException("Instrumentation admission failed", problem);
        } finally {
            instrumentation.removeTransformer(transformer);
        }
    }
}
