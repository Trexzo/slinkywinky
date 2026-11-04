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
    private static final int DEFAULT_TICK_LIMIT = 20;
    // Stop-command driven observation is still bounded if the operator forgets to stop.
    // 36,000 client ticks are approximately 30 minutes at nominal 20 TPS.
    private static final int MAX_CONTINUOUS_TICKS = 36000;
    private static final AtomicBoolean stopRequested = new AtomicBoolean();
    // An extended diagnostic is bounded and explicitly authorized by the loader.
    private static volatile int tickLimit = DEFAULT_TICK_LIMIT;
    private static final AtomicBoolean cleanupStarted = new AtomicBoolean();
    private static final AtomicBoolean hookInstalled = new AtomicBoolean();
    private static volatile boolean active;
    private static volatile Instrumentation installedVia;
    private static volatile Class<?> installedClass;
    private static volatile byte[] originalClassFingerprint;
    private static native int nativeZeroActionTick();
    private static native long nativeOpenObservation();
    private static native int nativeEnableStopSignal(long handle);
    private static native int nativePollStopSignal(long handle);
    private static native int nativeObserveFrame(long handle,long timeMs,long generation,int flags,
                                                  double[] xyz,float[] angles);
    private static native int nativeCloseObservation(long handle);
    /** R53: read-only native sample status, query on the owning client thread. */
    private static native double[] nativeReadObservationStatus(long handle);
    /** Read-only cross-thread copy; null for unknown, closed or unpublished sessions.
     * The tenth item is a strictly increasing per-session publication sequence.
     */
    private static native double[] nativeReadPublishedStatus(long handle);
    private static volatile long nativeContext;
    private static volatile boolean nativeSessionCloseOk;
    // R60: counters are owned solely by the admitted Minecraft client thread.
    // They record validated read-only frames, never entity identities or locations.
    private static int acceptedFrames, worldPlayerFrames, rayTargetFrames, generationChanges;
    private static long lastAcceptedGeneration = -1L;

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
    private static boolean isTickMethod(String method,boolean test,boolean vanilla) {
        if(test) return "tick".equals(method);
        return vanilla?"s".equals(method):
            ("runTick".equals(method)||"func_71407_l".equals(method));
    }

    private static int parseExtendedLimit(String value) {
        if ("120".equals(value)) return 120;
        if ("600".equals(value)) return 600;
        throw new IllegalArgumentException("Unsupported diagnostic tick limit");
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
                    if(tickLimit==MAX_CONTINUOUS_TICKS &&
                       nativeEnableStopSignal(nativeContext)!=1)
                        throw new IllegalStateException("Win32 stop-only gate unavailable");
                    System.out.println(PREFIX+"NATIVE_SESSION_OPEN_PASS");
                    System.out.flush();
                }
            }
            if(nativeContext!=0 && tickLimit==MAX_CONTINUOUS_TICKS) {
                final int signal=nativePollStopSignal(nativeContext);
                if(signal<0) throw new IllegalStateException("Win32 stop gate lost");
                if(signal==1) stopRequested.set(true);
            }
            nativeOk=nativeContext!=0 && nativeObserveFrame(nativeContext,
                observed.timeMs,observed.generation,observed.flags,
                observed.xyz,observed.angles)==1;
        }
        catch (Throwable failure) {
            System.err.println(PREFIX + "NATIVE_GAME_THREAD_PROBE_FAIL " + failure);
        }
        if (nativeOk && observed != null) {
            acceptedFrames++;
            if ((observed.flags & 3) == 3) {
                worldPlayerFrames++;
                if (worldPlayerFrames == 1)
                    System.out.println(PREFIX+"NATIVE_WORLD_PLAYER_OBSERVED tick="+n);
            }
            if ((observed.flags & 4) != 0) {
                rayTargetFrames++;
                if (rayTargetFrames == 1)
                    System.out.println(PREFIX+"NATIVE_RAY_ENTITY_OBSERVED tick="+n);
            }
            if (lastAcceptedGeneration != -1L &&
                observed.generation != lastAcceptedGeneration) generationChanges++;
            lastAcceptedGeneration = observed.generation;
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
        final boolean requestedStop = stopRequested.get() && tickLimit == MAX_CONTINUOUS_TICKS;
        if ((n >= tickLimit || requestedStop || !nativeOk) &&
            cleanupStarted.compareAndSet(false, true)) {
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
            // Summarize native-accepted observations before the Java sampler
            // releases all references. A UI cannot mistake this for combat proof.
            System.out.println(PREFIX+"NATIVE_OBSERVATION_SUMMARY accepted="+acceptedFrames+
                " worldPlayer="+worldPlayerFrames+" rayEntity="+rayTargetFrames+
                " generations="+generationChanges);
            // Drop Java world, player and reflection references on the exact client
            // thread before the asynchronous class restoration worker begins.
            ObservedState.release();
            System.out.println(PREFIX+"OBSERVATION_RELEASE_PASS");
            if(closed) System.out.println(PREFIX+"NATIVE_SESSION_CLOSE_PASS");
            else System.err.println(PREFIX+"NATIVE_SESSION_CLOSE_REJECTED");
            if(nativeOk && requestedStop) {
                System.out.println(PREFIX+"CONTINUOUS_STOP_ON_CLIENT_THREAD_PASS ticks="+n);
            } else if(n>=tickLimit && nativeOk) {
                System.out.println(PREFIX+"GAME_THREAD_TICK_"+tickLimit+"_PASS ticks="+n);
            } else {
                System.err.println(PREFIX+"GAME_THREAD_HOOK_ABORT ticks="+n);
            }
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
    static void verifyCallbackVisibility(Class<?> target) {
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

    /** R75 hook gate stays held permanently if restoration cannot be proven. */
    static void finishRenderDiagnostic(boolean restorationVerified) {
        if(restorationVerified) hookInstalled.set(false);
    }

    /**
     * R80: only RendererFrameDiagnostic's pre-weave catch may use this path.
     * It has not registered the bytecode-changing transformer. Never call
     * this after a transformation attempt, even if that attempt throws.
     */
    static void finishUnmodifiedRenderAdmission() {
        hookInstalled.set(false);
    }

    public static void agentmain(String options, Instrumentation instrumentation) {
        // R78 is an entirely separate, explicitly gated read-only native WGL
        // observation mode. Original Mojang class SHA admission is still
        // required before loading JNI or weaving any bytecode.
        final String contextPrefix="minecraft-render-context-r78|";
        if(options!=null && options.startsWith(contextPrefix)) {
            if(!hookInstalled.compareAndSet(false,true))
                throw new IllegalStateException("Minecraft instrumentation already active");
            try {
                RendererFrameDiagnostic.start(instrumentation,
                    options.substring(contextPrefix.length()));
            } catch(Exception error) {
                throw new IllegalStateException("R78 passive render-context audit refused",error);
            }
            return;
        }
        // R75: explicit finite original-only renderer diagnostic. Neither
        // continuous nor auto-enabled. Does not render or read game state.
        if ("minecraft-render-callback-r75".equals(options)) {
            if (!hookInstalled.compareAndSet(false,true))
                throw new IllegalStateException("Minecraft instrumentation already active");
            try {
                RendererFrameDiagnostic.start(instrumentation);
            } catch(Exception error) {
                // A failed installation may have touched class bytes. Only
                // RendererFrameDiagnostic's verified restore releases the gate.
                throw new IllegalStateException("R75 render diagnostic refused",error);
            }
            return;
        }
        // R73: strictly passive, explicitly opted-in loaded renderer fingerprint
        // preflight. This never installs a render callback or loads native GL.
        if ("minecraft-render-preflight".equals(options)) {
            if (!hookInstalled.compareAndSet(false,true))
                throw new IllegalStateException("Minecraft instrumentation already active");
            try {
                RendererLivePreflight.run(instrumentation);
            } catch (Exception error) {
                throw new IllegalStateException("R73 renderer preflight refused",error);
            } finally {
                hookInstalled.set(false);
            }
            return;
        }
        final int delimiter = options == null ? -1 : options.indexOf('|');
        final String mode = delimiter < 0 ? options : options.substring(0, delimiter);
        // A stop request is delivered through a separate, stop-only Win32
        // event. Never recursively Java Attach into a live instrumented JVM.
        final boolean continuous = "hook-continuous".equals(mode) ||
                                   "minecraft-continuous".equals(mode);
        final boolean test = "probe".equals(mode) || "hook-probe".equals(mode) ||
                             "hook-continuous".equals(mode);
        final boolean hook = "hook-probe".equals(mode) ||
                             "minecraft-hook".equals(mode) || continuous;
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
                        if (isTickMethod(method,test,"ave".equals(type.getName())) && "()V".equals(descriptor) &&
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
        // Claim admission BEFORE loading native state or resetting mapped observations.
        // A competing attach must not disturb an already-running game-thread hook.
        // One class retransformation at a time: even a passive probe could
        // otherwise restore an active diagnostic hook unexpectedly.
        if (!hookInstalled.compareAndSet(false, true))
            throw new IllegalStateException("Minecraft instrumentation already active");
        if (hook) {
            try {
                if (delimiter < 0 || delimiter == options.length() - 1)
                    throw new IllegalArgumentException("Missing exact JNI library path");
                String path = options.substring(delimiter + 1);
                int requestedLimit = continuous ? MAX_CONTINUOUS_TICKS : DEFAULT_TICK_LIMIT;
                final int durationDelimiter = path.indexOf('|');
                if (durationDelimiter >= 0) {
                    if (continuous)
                        throw new IllegalArgumentException("Continuous mode takes no tick suffix");
                    requestedLimit = parseExtendedLimit(path.substring(durationDelimiter + 1));
                    path = path.substring(0, durationDelimiter);
                }
                if (!path.endsWith("slinkywinky_jni.dll"))
                    throw new IllegalArgumentException("Unexpected JNI library path");
                System.load(path);
                tickLimit = requestedLimit;
                if (nativeZeroActionTick() != 1)
                    throw new IllegalStateException("Disabled C++ decision returned action/error");
                System.out.println(PREFIX + "NATIVE_CXX_PREFLIGHT_PASS");
                try { ObservedState.admit(target); }
                catch (ReflectiveOperationException mappingFailure) {
                    throw new IllegalStateException("Unknown Minecraft observation mapping", mappingFailure);
                }
                System.out.println(PREFIX + "OBSERVATION_MAPPING_ADMITTED");
                verifyCallbackVisibility(target);
            } catch (RuntimeException | LinkageError failure) {
                ObservedState.release();
                hookInstalled.set(false); // No transformer was installed.
                throw failure;
            }
        }
        if (hook) {
            System.out.println(PREFIX+"DIAGNOSTIC_TICK_LIMIT="+tickLimit);
            ticks.set(0);
            cleanupStarted.set(false);
            nativeContext=0;
            nativeSessionCloseOk=false;
            stopRequested.set(false);
            acceptedFrames=0;
            worldPlayerFrames=0;
            rayTargetFrames=0;
            generationChanges=0;
            lastAcceptedGeneration=-1L;
            installedVia = instrumentation;
            installedClass = target;
            active = true;
        }
        boolean transformerInstalled = false;
        boolean retransformStarted = false;
        try {
            instrumentation.addTransformer(transformer, true);
            transformerInstalled = true;
            retransformStarted = true;
            instrumentation.retransformClasses(target);
            if (seen.get() != 1 || (hook && modified.get() != 1))
                throw new IllegalStateException("Expected exactly one transformed target");
            System.out.println(PREFIX + (hook ? "GAME_THREAD_HOOK_INSTALLED" :
                "INSTRUMENT_RETRANSFORM_PASS") + " class=" + target.getName());
            System.out.flush();
        } catch (Throwable problem) {
            active = false;
            if (hook && retransformStarted) {
                // A partially applied transform is an unknown JVM state. Never
                // allow another attachment to overwrite it until process exit.
                System.err.println(PREFIX + "HOOK_REATTACH_LOCKED_AFTER_TRANSFORM_FAILURE");
            } else if (hook) {
                ObservedState.release();
                installedVia = null; installedClass = null;
                hookInstalled.set(false); // Nothing was transformed.
            }
            System.err.println(PREFIX + "INSTRUMENT_TRANSFORM_FAIL: " + problem);
            throw new IllegalStateException("Instrumentation admission failed", problem);
        } finally {
            if (transformerInstalled) instrumentation.removeTransformer(transformer);
            if (!hook) hookInstalled.set(false); // Pass-through probe is no longer active.
        }
    }
}
