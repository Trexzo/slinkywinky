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
import dev.slinkywinky.SourceRuntimePreview;
import dev.slinkywinky.SourcePacketReadOnlyQueue;
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
    // R359: set only after claiming original agent owner, never a global flag.
    private static volatile boolean r358SourceReadOnly;
    private static final String R358_ORIGINAL_AVE_SHA256 =
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82";
    private static SourceRuntimePreview r358Source;
    private static SourceAttackTickFence r360AttackFence;
    private static SourceClickAttackFence r362ClickFence;
    private static volatile boolean r362ExactClickMode,r362CallbackFault;
    private static Thread r362OwnerThread;
    private static int r358Executed,r358CandidateTargets,r360AttackCallbacks;
    private static long r362ClickReturns,r362AcceptedClicks;
    // R366: exact original NetworkManager packet bridge. Explicit opt-in
    // and NO source packet hold/replay, only read-only proposals.
    private static final String R365_ORIGINAL_EK_SHA256 =
        "0b898cf41a16e432dd4e873ed3bfbaf2536c149d44269910f31dca5f242d9436";
    private static volatile boolean r366PacketMode,r366PacketFault;
    private static volatile Vanilla189OutboundPrimitiveQueue r366Outbound;
    private static volatile Class<?> installedNetworkClass;
    private static volatile byte[] networkOriginalClassFingerprint;
    private static Class<?> r366PacketBase,r366MovementBase;
    private static long r366WorldGeneration,r366Packets,r366NativeEvent3,r366HoldProposals;

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

    /**
     * R366 observational original NetworkManager ek.a(Lff;)V ENTRY.
     * The packet argument is used transiently to copy primitive metadata;
     * this callback NEVER retains a Packet, Channel, World or JNI reference.
     * Network sends always continue through the original method unmodified.
     */
    public static void onOriginalOutboundPacket(Object manager,Object packet){
        if(!active||!r366PacketMode)return;
        try{
            final Class<?> bound=installedNetworkClass;
            if(bound==null||manager==null||manager.getClass()!=bound)
                throw new IllegalStateException("R366 unknown network owner");
            final Vanilla189OutboundPrimitiveQueue ingress=r366Outbound;
            if(ingress!=null&&!ingress.observe(packet,System.nanoTime()))
                r366PacketFault=true;
        }catch(Throwable fail){
            // Never propagate into NetworkManager/sendPacket.
            r366PacketFault=true;
        }
    }

    /**
     * R362 normal RETURN from SHA-pinned original ave.aw()V / clickMouse.
     * Never throws into the game, never cancels a real click and never alters
     * an on-wire packet. Errors abort the finite diagnostic on next tick.
     */
    public static void onOriginalClickReturn(){
        if(!active||!r362ExactClickMode)return;
        if(r362OwnerThread==null||r362ClickFence==null||r358Source==null)return;
        try{
            if(Thread.currentThread()!=r362OwnerThread)
                throw new IllegalStateException("R362 click callback on foreign game thread");
            final ObservedState.AttackClockFrame evidence=
                ObservedState.captureOnOriginalClickReturn();
            ++r362ClickReturns;
            if(evidence!=null&&r362ClickFence.onClick(
                evidence.generation,evidence.clock)){
                final long n=ticks.get();
                if(n<=0)throw new IllegalStateException("No admitted original game tick");
                final SourceRuntimePreview.Receipt attack=r358Source.observe(
                    1001,1002,evidence.generation,n,
                    SourceRuntimePreview.LAG_RANGE_ATTACK,
                    new int[]{1,1},new double[0]);
                if(attack.sourceEvent!=15||attack.at(2)!=1||attack.at(3)!=1)
                    throw new IllegalStateException("R362 native e15 click receipt denied");
                ++r362AcceptedClicks;
                if(r362AcceptedClicks==1)
                    System.out.println(PREFIX+
                        "R362_REAL_ORIGINAL_CLICK_TO_NATIVE_E15 normalReturn=1 "+
                        "timestampMatched=1 gameWrites=0 packetWrites=0");
            }
        }catch(Throwable failure){
            r362CallbackFault=true;
            System.err.println(PREFIX+"R362_CLICK_CALLBACK_ABORT "+failure);
        }
    }

    /** Runs on the actual target thread. Never changes Minecraft state. */
    public static void onGameTick() {
        if (!active) return;
        final int n = ticks.incrementAndGet();
        boolean nativeOk = false;
        ObservedState.Frame observed=null;
        try {
            if(r362CallbackFault)
                throw new IllegalStateException("R362 prior click callback failed closed");
            if(r366PacketFault)
                throw new IllegalStateException("R366 packet entry stream incomplete");
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
            if(nativeOk&&r358SourceReadOnly){
                if(r358Source==null)r358Source=new SourceRuntimePreview();
                final Vanilla189SourceEvidence.Sample remote=observed.source;
                final boolean worldPlayer=(observed.flags&3)==3;
                if(r362ExactClickMode){
                    // R362 precise original vanilla clickMouse normal-return
                    // replaces R360's tick-only candidate in this CLI mode.
                    if(r362ClickFence==null)r362ClickFence=new SourceClickAttackFence();
                    r362OwnerThread=Thread.currentThread();
                    r362ClickFence.prime(
                        worldPlayer?observed.generation:0,
                        worldPlayer?observed.combatClock:null);
                }else{
                    if(r360AttackFence==null)
                        r360AttackFence=new SourceAttackTickFence();
                    // Legacy original prelaunch property mode retains R360
                    // tick-only inference; it never duplicates R362 e15.
                    final boolean phaseCandidate=r360AttackFence.observe(
                        worldPlayer?observed.generation:0,
                        worldPlayer?observed.combatClock:null);
                    if(phaseCandidate){
                        final SourceRuntimePreview.Receipt attack=r358Source.observe(
                            1001,1002,observed.generation,n,
                            SourceRuntimePreview.LAG_RANGE_ATTACK,
                            new int[]{1,1},new double[0]);
                        if(attack.sourceEvent!=15||attack.at(2)!=1||attack.at(3)!=1)
                            throw new IllegalStateException("R360 native e15 phase receipt denied");
                        ++r360AttackCallbacks;
                    }
                }
                final int[] bits={worldPlayer?1:0,0,
                    worldPlayer&&remote!=null?1:0,0,0,0,0,0,0,0,0,0};
                final double[] vals={0,10,0,0,0,150};
                if(worldPlayer&&remote!=null){
                    final double dx=observed.xyz[0]-observed.xyz[3];
                    final double dy=observed.xyz[1]-observed.xyz[4];
                    final double dz=observed.xyz[2]-observed.xyz[5];
                    final double distance=dx*dx+dy*dy+dz*dz;
                    if(!Double.isFinite(distance)||distance>900000000000000d)
                        throw new IllegalStateException("R358 invalid player/target distance");
                    vals[0]=distance;
                    vals[2]=remote.x;vals[3]=remote.y;vals[4]=remote.z;
                    ++r358CandidateTargets;
                }
                // Opaque tokens + actual sampler generation, no Java refs.
                final SourceRuntimePreview.Receipt nativeTick=r358Source.observe(
                    1001,1002,observed.generation,n,
                    SourceRuntimePreview.LAG_RANGE_TICK,bits,vals);
                if(nativeTick.sourceEvent!=18||nativeTick.at(2)!=1)
                    throw new IllegalStateException("R358 native source e18 did not execute");
                ++r358Executed;
                if(r358Executed==1)
                    System.out.println(PREFIX+
                        "R358_REAL_ORIGINAL_CLIENT_SOURCE_NATIVE_E18 generation="+
                        observed.generation+" candidateTarget="+
                        (remote==null?0:1)+" packetWrites=0 gameWrites=0");
                if(r366PacketMode){
                    // R365 primitive-only, 256-cap, client-thread FIFO.
                    // Never schedule JNI on a foreign NetworkManager thread.
                    if(!worldPlayer){
                        if(r366Outbound!=null){
                            r366Outbound.close();r366Outbound=null;
                        }
                        r366WorldGeneration=0;
                    }else{
                        if(r366Outbound==null||
                           r366WorldGeneration!=observed.generation){
                            if(r366Outbound!=null)r366Outbound.close();
                            r366Outbound=new Vanilla189OutboundPrimitiveQueue(
                                r366PacketBase,r366MovementBase);
                            r366WorldGeneration=observed.generation;
                        }
                        if(!r366Outbound.healthy())
                            throw new IllegalStateException("R366 original packet queue poisoned");
                        final SourcePacketReadOnlyQueue.Batch observations=
                            r366Outbound.drain(32);
                        for(SourcePacketReadOnlyQueue.Observation row:observations.rows){
                            final long age=System.nanoTime()-row.nanoTime;
                            if(age<0||age>100000000L)
                                throw new IllegalStateException("R366 stale original packet evidence");
                            final int k=row.kind==SourcePacketReadOnlyQueue.MOVEMENT?1:0;
                            final SourceRuntimePreview.Receipt proposal=r358Source.observe(
                                1001,1002,observed.generation,n,
                                SourceRuntimePreview.LAG_RANGE_PACKET,
                                new int[]{k,row.sequence,row.moving?1:0,1},
                                new double[]{age/1000000L,row.x,row.y,row.z});
                            if(proposal.sourceEvent!=3||proposal.at(2)!=1||
                               (proposal.at(3)!=0&&proposal.at(3)!=1))
                                throw new IllegalStateException("R366 native e3 returned invalid verdict");
                            ++r366NativeEvent3;
                            if(proposal.at(3)==1)++r366HoldProposals;
                        }
                        r366Packets+=observations.rows.size();
                    }
                }
            }
        }
        catch (Throwable failure) {
            // A source JNI refusal must terminate the observation rather
            // than preserving the nativeOk bit from the earlier JNI frame.
            nativeOk=false;
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
            // Retire both native and original player-clock owners on
            // exactly the Minecraft game thread, before class restoration.
            boolean sourceClosed=true;
            if(r366Outbound!=null){
                try{r366Outbound.close();}
                catch(Throwable failure){
                    sourceClosed=false;
                    System.err.println(PREFIX+"R366_PACKET_PRIMITIVE_CLOSE_FAIL "+failure);
                } finally{r366Outbound=null;r366WorldGeneration=0;}
            }
            if(r362ClickFence!=null){
                try{r362ClickFence.close();}
                catch(Throwable failure){
                    sourceClosed=false;
                    System.err.println(PREFIX+"R362_CLICK_FENCE_CLOSE_FAIL "+failure);
                } finally{r362ClickFence=null;}
            }
            if(r360AttackFence!=null){
                try{r360AttackFence.close();}
                catch(Throwable failure){
                    sourceClosed=false;
                    System.err.println(PREFIX+"R360_ATTACK_FENCE_CLOSE_FAIL "+failure);
                } finally{r360AttackFence=null;}
            }
            if(r358Source!=null){
                try {r358Source.close();}
                catch(Throwable failure){
                    sourceClosed=false;
                    System.err.println(PREFIX+"R358_NATIVE_SOURCE_CLOSE_FAIL "+failure);
                } finally {r358Source=null;}
            }
            if(r358SourceReadOnly)
                System.out.println(PREFIX+"R358_NATIVE_SOURCE_SUMMARY executed="+
                    r358Executed+" candidateTargets="+r358CandidateTargets+
                    " attackPhaseCandidates="+r360AttackCallbacks+
                    " clickReturns="+r362ClickReturns+
                    " clickEvidenceNativeE15="+r362AcceptedClicks+
                    " nativeClosed="+(sourceClosed?1:0)+" gameWrites=0");
            if(r366PacketMode)
                System.out.println(PREFIX+"R366_ORIGINAL_NETWORK_SOURCE_SUMMARY "+
                    "packetRows="+r366Packets+" nativeEvent3="+r366NativeEvent3+
                    " hypotheticalHold="+r366HoldProposals+
                    " packetWrites=0 packetCancelled=0");
            // Destroy on the same client thread that created this C++ context.
            long closing=nativeContext;
            nativeContext=0;
            boolean closed=closing==0;
            try {
                if(closing!=0) closed=nativeCloseObservation(closing)==1;
            } catch(Throwable problem) {
                System.err.println(PREFIX+"NATIVE_SESSION_CLOSE_FAIL "+problem);
            }
            nativeSessionCloseOk=closed&&sourceClosed;
            // Summarize native-accepted observations before the Java sampler
            // releases all references. A UI cannot mistake this for combat proof.
            System.out.println(PREFIX+"NATIVE_OBSERVATION_SUMMARY accepted="+acceptedFrames+
                " worldPlayer="+worldPlayerFrames+" rayEntity="+rayTargetFrames+
                " generations="+generationChanges);
            // Drop Java world, player and reflection references on the exact client
            // thread before the asynchronous class restoration worker begins.
            ObservedState.release();
            r366PacketMode=false;r366PacketFault=false;
            r362ExactClickMode=false;r362CallbackFault=false;
            r362OwnerThread=null;
            r358SourceReadOnly=false;
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
                    final Class<?> networkTarget=installedNetworkClass;
                    final byte[] networkExpected=networkOriginalClassFingerprint;
                    final AtomicInteger observed = new AtomicInteger();
                    final AtomicBoolean sameBytes = new AtomicBoolean();
                    final AtomicInteger networkObserved=new AtomicInteger();
                    final AtomicBoolean sameNetworkBytes=new AtomicBoolean();
                    ClassFileTransformer verifier = new ClassFileTransformer() {
                        @Override public byte[] transform(ClassLoader loader, String name,
                                Class<?> type, ProtectionDomain protection, byte[] bytes) {
                            if (type == target) {
                                observed.incrementAndGet();
                                sameBytes.set(Arrays.equals(expected, sha256(bytes)));
                            }else if(networkTarget!=null&&type==networkTarget){
                                networkObserved.incrementAndGet();
                                sameNetworkBytes.set(
                                    Arrays.equals(networkExpected,sha256(bytes)));
                            }
                            return null;
                        }
                    };
                    try {
                        if (expected == null ||
                           (networkTarget!=null&&networkExpected==null))
                            throw new IllegalStateException("Missing original class fingerprint(s)");
                        instrumentation.addTransformer(verifier, true);
                        try {
                            // The hook transformer was removed after installation.
                            // This second retransform restores its unmodified input.
                            if(networkTarget==null)
                                instrumentation.retransformClasses(target);
                            else
                                instrumentation.retransformClasses(target,networkTarget);
                        } finally {
                            instrumentation.removeTransformer(verifier);
                        }
                        if (observed.get() != 1 || !sameBytes.get()||
                            (networkTarget!=null&&
                             (networkObserved.get()!=1||!sameNetworkBytes.get())))
                            throw new IllegalStateException(
                                "Original Minecraft and/or NetworkManager restore fingerprint mismatch");
                        if(!nativeSessionCloseOk) throw new IllegalStateException("Native client-thread context was not released");
                        restored = true;
                        System.out.println(PREFIX + "CLASS_BYTECODE_RESTORE_PASS sha256=" +
                            hex(expected));
                        if(networkTarget!=null)
                            System.out.println(PREFIX+
                                "R366_ORIGINAL_NETWORK_MANAGER_RESTORE_PASS sha256="+
                                hex(networkExpected));
                        System.out.println(PREFIX + "GAME_THREAD_HOOK_CLEANUP_PASS");
                    } catch (Throwable problem) {
                        System.err.println(PREFIX + "GAME_THREAD_HOOK_CLEANUP_FAIL " + problem);
                    } finally {
                        if (restored) {
                            originalClassFingerprint = null;
                            networkOriginalClassFingerprint=null;
                            installedNetworkClass=null;
                            r366PacketBase=r366MovementBase=null;
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
        // R103: explicit original-only, read-only render-HDC-to-HWND audit.
        // No drawing, input forwarding, focus manipulation or gameplay.
        final String windowAuditPrefix="minecraft-render-window-audit-r101|";
        if(options!=null && options.startsWith(windowAuditPrefix)) {
            if(!hookInstalled.compareAndSet(false,true))
                throw new IllegalStateException("Minecraft instrumentation already active");
            try {
                RendererFrameDiagnostic.start(instrumentation,
                    options.substring(windowAuditPrefix.length()),
                    false,false,false,false,false,false,true);
            } catch(Exception error) {
                throw new IllegalStateException(
                    "R103 source-pinned original renderer window audit refused",error);
            }
            return;
        }
        // R100: manual external PID/birth-scoped operator menu commands,
        // acknowledged on the source-pinned original WGL render owner.
        // No game keyboard/mouse hooks or gameplay changes are admitted.
        final String operatorPrefix="minecraft-render-menu-r100|";
        if(options!=null && options.startsWith(operatorPrefix)) {
            if(!hookInstalled.compareAndSet(false,true))
                throw new IllegalStateException("Minecraft instrumentation already active");
            try {
                RendererFrameDiagnostic.start(instrumentation,
                    options.substring(operatorPrefix.length()),
                    true,false,false,false,false,true);
            } catch(Exception error) {
                throw new IllegalStateException("R100 operator original menu refused",error);
            }
            return;
        }
        // R99: 72-frame source-pinned GL11 retained model, with four
        // first-party scripted navigation actions. No game input/control.
        final String retainedPrefix="minecraft-render-retained-r99|";
        if(options!=null && options.startsWith(retainedPrefix)) {
            if(!hookInstalled.compareAndSet(false,true))
                throw new IllegalStateException("Minecraft instrumentation already active");
            try {
                RendererFrameDiagnostic.start(instrumentation,
                    options.substring(retainedPrefix.length()),
                    true,false,false,false,true);
            } catch(Exception error) {
                throw new IllegalStateException("R99 retained original menu refused",error);
            }
            return;
        }
        // R94: admitted only by exact original source and operator consent.
        final String manualPrefix="minecraft-render-manual-r94|";
        if(options!=null && options.startsWith(manualPrefix)) {
            if(!hookInstalled.compareAndSet(false,true))
                throw new IllegalStateException("Minecraft instrumentation already active");
            try {
                RendererFrameDiagnostic.start(instrumentation,
                    options.substring(manualPrefix.length()),true,false,false,true);
            } catch(Exception error) {
                throw new IllegalStateException("R94 manual menu activation refused",error);
            }
            return;
        }
        // R93: isolated, bounded show / hide / show. Never intercepts input.
        final String visibilityPrefix="minecraft-render-visibility-r93|";
        if(options!=null && options.startsWith(visibilityPrefix)) {
            if(!hookInstalled.compareAndSet(false,true))
                throw new IllegalStateException("Minecraft instrumentation already active");
            try {
                RendererFrameDiagnostic.start(instrumentation,
                    options.substring(visibilityPrefix.length()),true,false,true);
            } catch(Exception error) {
                throw new IllegalStateException("R93 scripted visibility refused",error);
            }
            return;
        }
        // R92: finite 8-preflight + 64-overlay original render callbacks.
        // Explicitly separated from read-only R90 and one-frame R91.
        final String multiPrefix="minecraft-render-overlay-r92|";
        if(options!=null && options.startsWith(multiPrefix)) {
            if(!hookInstalled.compareAndSet(false,true))
                throw new IllegalStateException("Minecraft instrumentation already active");
            try {
                RendererFrameDiagnostic.start(instrumentation,
                    options.substring(multiPrefix.length()),true,true);
            } catch(Exception error) {
                throw new IllegalStateException("R92 multi-frame native overlay refused",error);
            }
            return;
        }
        // R91: one-frame GL11 renderer, never on by default. The original
        // source-pinned R90 renderer, owner thread, finite callback and native
        // WGL context gates remain mandatory. No combat or input controls.
        final String overlayPrefix="minecraft-render-overlay-r91|";
        if(options!=null && options.startsWith(overlayPrefix)) {
            if(!hookInstalled.compareAndSet(false,true))
                throw new IllegalStateException("Minecraft instrumentation already active");
            try {
                RendererFrameDiagnostic.start(instrumentation,
                    options.substring(overlayPrefix.length()),true);
            } catch(Exception error) {
                throw new IllegalStateException("R91 one-frame native overlay refused",error);
            }
            return;
        }
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
            boolean inspectorRemovalConfirmed=true;
            boolean preflightSucceeded=false;
            try {
                RendererLivePreflight.run(instrumentation);
                preflightSucceeded=true;
            } catch(RendererLivePreflight.InspectorRemovalUncertain uncertain) {
                // R80: a passive inspector may still be registered. Preserve
                // the lock, even if no renderer bytecode was changed.
                inspectorRemovalConfirmed=false;
                throw new IllegalStateException(
                    "R73 passive renderer inspector removal unconfirmed",uncertain);
            } catch (Exception error) {
                throw new IllegalStateException("R73 renderer preflight refused",error);
            } catch (Error unexpectedVmError) {
                // We cannot prove the passive-inspector lifecycle after a VM
                // or linkage error. Stay locked rather than permit reattach.
                inspectorRemovalConfirmed=false;
                throw unexpectedVmError;
            } finally {
                if(inspectorRemovalConfirmed) {
                    hookInstalled.set(false);
                    // R83: the R73 preflight PASS marker is printed BEFORE
                    // this finally block. The harness must instead wait for
                    // the explicit release receipt to sequence the next Attach.
                    if(preflightSucceeded) {
                        System.out.println("R83_ORIGINAL_PREFLIGHT_ADMISSION_RELEASED_PASS");
                        System.out.flush();
                    }
                }
            }
            return;
        }
        final int delimiter = options == null ? -1 : options.indexOf('|');
        final String mode = delimiter < 0 ? options : options.substring(0, delimiter);
        // A stop request is delivered through a separate, stop-only Win32
        // event. Never recursively Java Attach into a live instrumented JVM.
        final boolean continuous = "hook-continuous".equals(mode) ||
                                   "minecraft-continuous".equals(mode);
        final boolean packetMode="minecraft-network-source-readonly".equals(mode);
        final boolean clickMode="minecraft-click-source-readonly".equals(mode)||packetMode;
        final boolean sourceMode="minecraft-source-readonly".equals(mode)||clickMode;
        final boolean test = "probe".equals(mode) || "hook-probe".equals(mode) ||
                             "hook-continuous".equals(mode);
        final boolean hook = "hook-probe".equals(mode) ||
                             "minecraft-hook".equals(mode) || continuous || sourceMode;
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
        final AtomicInteger modifiedNetwork = new AtomicInteger();
        ClassFileTransformer transformer = new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader, String name, Class<?> type,
                                               ProtectionDomain protection, byte[] bytes) {
                if(type!=null&&packetMode&&type==installedNetworkClass){
                    // Exactly one original, SHA-pinned NetworkManager method
                    // in the SAME original vanilla classloader. No other
                    // method is modified and no send is intercepted.
                    final byte[] digest=sha256(bytes);
                    if(!R365_ORIGINAL_EK_SHA256.equals(hex(digest)))
                        throw new IllegalStateException(
                            "R366 original Mojang 1.8.9 ek.class hash mismatch");
                    final ClassReader packetReader=new ClassReader(bytes);
                    final ClassWriter packetWriter=
                        new ClassWriter(packetReader,ClassWriter.COMPUTE_MAXS);
                    final AtomicInteger packetMethods=new AtomicInteger();
                    final AtomicInteger packetEntryHooks=new AtomicInteger();
                    packetReader.accept(new ClassVisitor(Opcodes.ASM5,packetWriter){
                        @Override public MethodVisitor visitMethod(int access,String method,
                                String descriptor,String signature,String[] exceptions){
                            MethodVisitor downstream=super.visitMethod(access,method,
                                descriptor,signature,exceptions);
                            if(Vanilla189OutboundEntryWeaver.matches(method,descriptor,access)){
                                packetMethods.incrementAndGet();
                                return Vanilla189OutboundEntryWeaver.wrap(
                                    downstream,packetEntryHooks);
                            }
                            return downstream;
                        }
                    },0);
                    if(packetMethods.get()!=1||packetEntryHooks.get()!=1)
                        throw new IllegalStateException(
                            "R366 exact original outbound packet method unavailable");
                    networkOriginalClassFingerprint=digest;
                    modifiedNetwork.incrementAndGet();
                    return packetWriter.toByteArray();
                }
                if (type == null || !recognized(type.getName(), test)) return null;
                seen.incrementAndGet();
                if (!hook) return null;
                if(r358SourceReadOnly &&
                   (!"ave".equals(type.getName())||
                    !R358_ORIGINAL_AVE_SHA256.equals(hex(sha256(bytes)))))
                    throw new IllegalStateException("R358 original Mojang 1.8.9 class hash mismatch");
                originalClassFingerprint = sha256(bytes);
                ClassReader reader = new ClassReader(bytes);
                ClassWriter writer = new ClassWriter(reader, ClassWriter.COMPUTE_MAXS);
                final AtomicInteger exact = new AtomicInteger();
                final AtomicInteger clickMethods = new AtomicInteger();
                final AtomicInteger clickReturns = new AtomicInteger();
                ClassVisitor visitor = new ClassVisitor(Opcodes.ASM5, writer) {
                    @Override public MethodVisitor visitMethod(int access, String method,
                            String descriptor, String signature, String[] exceptions) {
                        MethodVisitor downstream =
                            super.visitMethod(access, method, descriptor, signature, exceptions);
                        if(r362ExactClickMode &&
                           Vanilla189ClickExitWeaver.matches(method,descriptor,access)){
                            clickMethods.incrementAndGet();
                            return Vanilla189ClickExitWeaver.wrap(downstream,clickReturns);
                        }
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
                if (r362ExactClickMode &&
                    (clickMethods.get()!=1||clickReturns.get()<1))
                    throw new IllegalStateException(
                        "R362 exact original ave.aw()V click return mapping unavailable");
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
            r358SourceReadOnly=sourceMode||
                Boolean.getBoolean("slinkywinky.r358OriginalSourceTickReadOnly");
            r362ExactClickMode=clickMode;
            r366PacketMode=packetMode;
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
                if(r358SourceReadOnly)
                    SourceRuntimePreview.admitPreloadedLibrary();
                System.out.println(PREFIX + "NATIVE_CXX_PREFLIGHT_PASS");
                try { ObservedState.admit(target); }
                catch (ReflectiveOperationException mappingFailure) {
                    throw new IllegalStateException("Unknown Minecraft observation mapping", mappingFailure);
                }
                System.out.println(PREFIX + "OBSERVATION_MAPPING_ADMITTED");
                if(r358SourceReadOnly){
                    if(test||!"ave".equals(target.getName()))
                        throw new IllegalStateException("R358 only admits original vanilla Minecraft");
                    ObservedState.enableOriginalSourceReadOnly(true);
                }
                verifyCallbackVisibility(target);
                if(packetMode){
                    // Both classes must be loaded already, modifiable,
                    // and resolved from the SAME original vanilla loader.
                    Class<?> network=null;
                    for(Class<?> c:instrumentation.getAllLoadedClasses()){
                        if("ek".equals(c.getName())&&
                           c.getClassLoader()==target.getClassLoader()){
                            if(network!=null)
                                throw new IllegalStateException("Duplicate original NetworkManager");
                            network=c;
                        }
                    }
                    if(network==null||!instrumentation.isModifiableClass(network))
                        throw new IllegalStateException("Original ek NetworkManager not already loaded");
                    ClassLoader loader=network.getClassLoader();
                    Class<?> packet=Class.forName("ff",false,loader);
                    Class<?> movement=Class.forName("ip",false,loader);
                    // Validate exact original primitive names and field types
                    // before installing EITHER original class transformer.
                    Vanilla189OutboundPacketSampler preflight=
                        new Vanilla189OutboundPacketSampler(packet,movement);
                    preflight.close();
                    installedNetworkClass=network;
                    r366PacketBase=packet;r366MovementBase=movement;
                }
            } catch (RuntimeException | LinkageError | ReflectiveOperationException failure) {
                ObservedState.release();
                r358SourceReadOnly=false;
                r362ExactClickMode=false;
                r366PacketMode=false;
                installedNetworkClass=null;
                r366PacketBase=r366MovementBase=null;
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
            r358Source=null;r360AttackFence=null;r362ClickFence=null;
            r362OwnerThread=null;r362CallbackFault=false;
            r366Outbound=null;r366PacketFault=false;r366WorldGeneration=0;
            r366Packets=r366NativeEvent3=r366HoldProposals=0;
            r358Executed=r358CandidateTargets=r360AttackCallbacks=0;
            r362ClickReturns=r362AcceptedClicks=0;
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
            if(packetMode){
                if(installedNetworkClass==null)
                    throw new IllegalStateException("Original NetworkManager admission missing");
                instrumentation.retransformClasses(target,installedNetworkClass);
            }else{
                instrumentation.retransformClasses(target);
            }
            if (seen.get() != 1 || (hook && modified.get() != 1)||
                (packetMode&&
                 (modifiedNetwork.get()!=1||networkOriginalClassFingerprint==null)))
                throw new IllegalStateException("Expected exact original transformed target(s)");
            System.out.println(PREFIX + (hook ? "GAME_THREAD_HOOK_INSTALLED" :
                "INSTRUMENT_RETRANSFORM_PASS") + " class=" + target.getName());
            if(packetMode)
                System.out.println(PREFIX+
                    "R366_ORIGINAL_EK_PACKET_OBSERVER_INSTALLED "+
                    "sha256="+R365_ORIGINAL_EK_SHA256+
                    " method=a(Lff;)V packetWrites=0");
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
