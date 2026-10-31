package dev.slinkywinky.instrument;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.ProtectionDomain;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;
import java.lang.reflect.Method;
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
    private static volatile byte[] originalClassFingerprint;
    private static native int nativeZeroActionTick();
    private static native long nativeOpenObservation();
    private static native int nativeObserveFrame(long handle,long timeMs,long generation,int flags,
                                                  double[] xyz,float[] angles);
    private static native int nativeCloseObservation(long handle);
    private static volatile long nativeContext;
    private static volatile boolean nativeSessionCloseOk;

    private static byte[] sha256(byte[] bytes) {
        try { return MessageDigest.getInstance("SHA-256").digest(bytes); }
        catch (NoSuchAlgorithmException impossible) {
            throw new IllegalStateException("JDK SHA-256 missing", impossible);
        }
    }

    private static String hex(byte[] bytes) {
        StringBuilder out = new StringBuilder(bytes.length * 2);
        for (byte b : bytes) out.append(String.format("%02x", b & 0xff));
        return out.toString();
    }
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
        boolean nativeOk = false;
        ObservedState.Frame observed=null;
        try {
            observed=ObservedState.sample();
            if(nativeContext==0) {
                nativeContext=nativeOpenObservation(); // Runs on exact admitted game thread.
                if(nativeContext!=0) {
                    System.out.println(PREFIX+"NATIVE_SESSION_OPEN_PASS");
                    System.out.flush();
                }
            }
            nativeOk=nativeContext!=0 && nativeObserveFrame(nativeContext,
                observed.timeMs,observed.generation,observed.flags,
                observed.xyz,observed.angles)==1;
        }
        catch (Throwable failure) {
            System.err.println(PREFIX + "NATIVE_GAME_THREAD_PROBE_FAIL " + failure);
        }
        if(n==1 && nativeOk && observed!=null)
            System.out.println(PREFIX+"NATIVE_OBSERVATION_PASS world="+((observed.flags&1)!=0?1:0)+
                " player="+((observed.flags&2)!=0?1:0)+" target="+((observed.flags&4)!=0?1:0));
        if (n == 1 && nativeOk) {
            System.out.println(PREFIX + "NATIVE_GAME_THREAD_CXX_PASS thread=" +
                Thread.currentThread().getName());
            System.out.flush();
        }
        if (!nativeOk) {
            System.err.println(PREFIX + "NATIVE_GAME_THREAD_PROBE_REJECTED tick=" + n);
            active = false;
        }
        if (n == 1) {
            System.out.println(PREFIX + "GAME_THREAD_TICK_ENTRY_PASS thread=" +
                Thread.currentThread().getName());
            System.out.flush();
        }
        if ((n >= 20 || !nativeOk) && cleanupStarted.compareAndSet(false, true)) {
            active = false;
            // Destroy on the same client thread that created this C++ context.
            long closing=nativeContext;
            nativeContext=0;
            boolean closed=closing==0;
            try {
                if(closing!=0) closed=nativeCloseObservation(closing)==1;
            } catch(Throwable problem) {
                System.err.println(PREFIX+"NATIVE_SESSION_CLOSE_FAIL "+problem);
            }
            nativeSessionCloseOk=closed;
            if(closed) System.out.println(PREFIX+"NATIVE_SESSION_CLOSE_PASS");
            else System.err.println(PREFIX+"NATIVE_SESSION_CLOSE_REJECTED");
            if(n>=20 && nativeOk) System.out.println(PREFIX+"GAME_THREAD_TICK_20_PASS ticks="+n);
            else System.err.println(PREFIX+"GAME_THREAD_HOOK_ABORT ticks="+n);
            System.out.flush();
            final Instrumentation instrumentation = installedVia;
            final Class<?> target = installedClass;
            Thread cleanup = new Thread(new Runnable() {
                @Override public void run() {
                    boolean restored = false;
                    final byte[] expected = originalClassFingerprint;
                    final AtomicInteger observed = new AtomicInteger();
                    final AtomicBoolean sameBytes = new AtomicBoolean();
                    ClassFileTransformer verifier = new ClassFileTransformer() {
                        @Override public byte[] transform(ClassLoader loader, String name,
                                Class<?> type, ProtectionDomain protection, byte[] bytes) {
                            if (type == target) {
                                observed.incrementAndGet();
                                sameBytes.set(Arrays.equals(expected, sha256(bytes)));
                            }
                            return null;
                        }
                    };
                    try {
                        if (expected == null) throw new IllegalStateException("Missing baseline fingerprint");
                        instrumentation.addTransformer(verifier, true);
                        try {
                            // The hook transformer was removed after installation.
                            // This second retransform restores its unmodified input.
                            instrumentation.retransformClasses(target);
                        } finally {
                            instrumentation.removeTransformer(verifier);
                        }
                        if (observed.get() != 1 || !sameBytes.get())
                            throw new IllegalStateException("Original class fingerprint mismatch");
                        if(!nativeSessionCloseOk) throw new IllegalStateException("Native client-thread context was not released");
                        restored = true;
                        System.out.println(PREFIX + "CLASS_BYTECODE_RESTORE_PASS sha256=" +
                            hex(expected));
                        System.out.println(PREFIX + "GAME_THREAD_HOOK_CLEANUP_PASS");
                    } catch (Throwable problem) {
                        System.err.println(PREFIX + "GAME_THREAD_HOOK_CLEANUP_FAIL " + problem);
                    } finally {
                        if (restored) {
                            originalClassFingerprint = null;
                            installedVia = null;
                            installedClass = null;
                            hookInstalled.set(false);
                        } else {
                            // A failed restore must not permit reinstrumentation.
                            active = false;
                            System.err.println(PREFIX + "HOOK_REATTACH_LOCKED_AFTER_RESTORE_FAILURE");
                        }
                    }
                }
            }, "SlinkyWinky-ReadOnly-HookCleanup");
            cleanup.setDaemon(true);
            cleanup.start();
        }
    }

    /**
     * LaunchWrapper may try findClass before parent delegation. Its documented
     * exclusion API makes the agent's existing system-loaded class visible to
     * the Minecraft class loader. Validate exact identity *before* retransform.
     */
    private static void verifyCallbackVisibility(Class<?> target) {
        final ClassLoader loader = target.getClassLoader();
        if (loader == null) throw new IllegalStateException("Unexpected bootstrap target");
        if (loader != InstrumentationProbe.class.getClassLoader()) {
            if (!"net.minecraft.launchwrapper.LaunchClassLoader".equals(loader.getClass().getName()))
                throw new IllegalStateException("Unsupported Minecraft class loader: " + loader);
            try {
                Method exclude = loader.getClass().getMethod("addClassLoaderExclusion", String.class);
                exclude.invoke(loader, "dev.slinkywinky.instrument.");
            } catch (ReflectiveOperationException failure) {
                throw new IllegalStateException("Cannot register safe class-loader delegation", failure);
            }
        }
        try {
            Class<?> resolved = Class.forName(InstrumentationProbe.class.getName(), false, loader);
            if (resolved != InstrumentationProbe.class)
                throw new IllegalStateException("Callback resolved to a different class loader");
            System.out.println(PREFIX + "CALLBACK_CLASSLOADER_VALIDATED loader=" + loader);
            System.out.flush();
        } catch (ClassNotFoundException failure) {
            throw new IllegalStateException("Game loader cannot resolve callback", failure);
        }
    }

    public static void agentmain(String options, Instrumentation instrumentation) {
        final int delimiter = options == null ? -1 : options.indexOf('|');
        final String mode = delimiter < 0 ? options : options.substring(0, delimiter);
        final boolean test = "probe".equals(mode) || "hook-probe".equals(mode);
        final boolean hook = "hook-probe".equals(mode) || "minecraft-hook".equals(mode);
        if (!test && !"minecraft".equals(mode) && !hook)
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
                originalClassFingerprint = sha256(bytes);
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
        if (hook) {
            if (delimiter < 0 || delimiter == options.length() - 1)
                throw new IllegalArgumentException("Missing exact JNI library path");
            String path = options.substring(delimiter + 1);
            if (!path.endsWith("slinkywinky_jni.dll"))
                throw new IllegalArgumentException("Unexpected JNI library path");
            System.load(path);
            if (nativeZeroActionTick() != 1)
                throw new IllegalStateException("Disabled C++ decision returned action/error");
            System.out.println(PREFIX + "NATIVE_CXX_PREFLIGHT_PASS");
            try { ObservedState.admit(target); }
            catch (ReflectiveOperationException mappingFailure) {
                throw new IllegalStateException("Unknown Minecraft observation mapping", mappingFailure);
            }
            System.out.println(PREFIX + "OBSERVATION_MAPPING_ADMITTED");
            verifyCallbackVisibility(target);
        }
        if (hook && !hookInstalled.compareAndSet(false, true))
            throw new IllegalStateException("Read-only hook already installed");
        if (hook) {
            ticks.set(0);
            cleanupStarted.set(false);
            nativeContext=0;
            nativeSessionCloseOk=false;
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
