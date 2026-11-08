package dev.slinkywinky.instrument;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.MessageDigest;
import java.security.ProtectionDomain;
import java.util.Arrays;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R75: opt-in, finite Java-only callback diagnostic on exact ORIGINAL bfk.
 * R78 observes WGL/viewport without rendering. R91, R92 and R93 are
 * separate finite native GL experiments with explicit admission. R93
 * exercises an input-free show/hide/show schedule only. A daemon retransform
 * restores source-pinned original bytecode (or locks future admission).
 */
public final class RendererFrameDiagnostic {
    private static final String PINNED_BFK_SHA256 =
        "8a4ce93bbf272398ea298d2ed363f75211dd469fcd3b66682a387e4738a419a2";
    private static volatile RenderCallbackFence activeFence;

    private static byte[] hash(byte[] data) {
        try {return MessageDigest.getInstance("SHA-256").digest(data);}
        catch(Exception error){throw new IllegalStateException(error);}
    }
    private static String hex(byte[] data) {
        StringBuilder result=new StringBuilder(64);
        for(byte item:data)result.append(String.format("%02x",item&0xff));
        return result.toString();
    }

    /** Only a validated renderer entry can issue the optional WGL read. */
    public static void onRender(float partialTicks,long frameTime) {
        final RenderCallbackFence fence=activeFence;
        if(fence==null)return;
        try {
            final Thread thread=Thread.currentThread();
            fence.sample(thread,partialTicks,frameTime,thread.getStackTrace());
        } catch(Throwable unexpected) {
            // A callback exception must never escape into Mojang code.
            fence.abort();
        }
    }

    /**
     * Pure, pinned original-bytecode transformation, also audited offline.
     * Not available for generated fixture hashes or modded clients.
     */
    public static byte[] weavePinnedOriginal(byte[] raw) {
        if(raw==null || raw.length==0 || raw.length>1000000 ||
           !PINNED_BFK_SHA256.equals(hex(hash(raw))))
            throw new IllegalArgumentException("R75 original bfk fingerprint mismatch");
        return weaveValidatedRuntime(raw,false);
    }

    /** R92 offline audit only: require pinned Mojang bfk source bytes. */
    public static byte[] weavePinnedOriginalAtExit(byte[] raw) {
        if(raw==null || raw.length==0 || raw.length>1000000 ||
           !PINNED_BFK_SHA256.equals(hex(hash(raw))))
            throw new IllegalArgumentException("R92 original bfk fingerprint mismatch");
        return weaveValidatedRuntime(raw,true);
    }

    /** Called ONLY after original-source and loaded-code shape admission. */
    private static byte[] weaveValidatedRuntime(byte[] raw,final boolean atExit) {
        final ClassReader reader=new ClassReader(raw);
        if(!"bfk".equals(reader.getClassName()))
            throw new IllegalArgumentException("R75 unexpected renderer class");
        final ClassWriter writer=new ClassWriter(reader,ClassWriter.COMPUTE_MAXS);
        final AtomicInteger matched=new AtomicInteger();
        reader.accept(new ClassVisitor(Opcodes.ASM5,writer) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String descriptor,String signature,String[] exceptions) {
                MethodVisitor downstream=super.visitMethod(access,name,
                    descriptor,signature,exceptions);
                if(!"a".equals(name) || !"(FJ)V".equals(descriptor))return downstream;
                if((access&Opcodes.ACC_PUBLIC)==0 ||
                   (access&(Opcodes.ACC_STATIC|Opcodes.ACC_ABSTRACT|Opcodes.ACC_NATIVE))!=0)
                    throw new IllegalArgumentException("R75 unsupported renderer access");
                matched.incrementAndGet();
                return new MethodVisitor(Opcodes.ASM5,downstream) {
                    private void callback() {
                        super.visitVarInsn(Opcodes.FLOAD,1);
                        super.visitVarInsn(Opcodes.LLOAD,2);
                        super.visitMethodInsn(Opcodes.INVOKESTATIC,
                            "dev/slinkywinky/instrument/RendererFrameDiagnostic",
                            "onRender","(FJ)V",false);
                    }
                    @Override public void visitCode() {
                        super.visitCode();
                        if(!atExit)callback(); // R75-R91 original entry semantics.
                    }
                    @Override public void visitInsn(int opcode) {
                        // R92 only: draw after the scene is rendered, but
                        // before the game's caller swaps its back buffer.
                        if(atExit && opcode==Opcodes.RETURN)callback();
                        super.visitInsn(opcode);
                    }
                };
            }
        },0);
        if(matched.get()!=1)
            throw new IllegalArgumentException("R75 original renderer method not unique");
        return writer.toByteArray();
    }

    /**
     * Always retransform without the hook transformer and inspect original SHA.
     * Only the caller can decide whether another attach is safe.
     */
    private static boolean restore(final Instrumentation instrumentation,
                                   final Class<?> target) {
        final AtomicInteger observed=new AtomicInteger();
        final java.util.concurrent.atomic.AtomicBoolean exact=
            new java.util.concurrent.atomic.AtomicBoolean();
        ClassFileTransformer passive=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader,String name,
                    Class<?> type,ProtectionDomain protection,byte[] raw) {
                if(type==target) {
                    observed.incrementAndGet();
                    // HotSpot often reorders constant-pool and method entries
                    // on retransformation. Require the source-pinned original
                    // runtime execution shape, not unstable VM raw SHA bytes.
                    try {
                        RendererLivePreflight.checkPinnedRuntimeRenderer(target,raw);
                        exact.set(true);
                    } catch(Exception mismatch) {
                        exact.set(false);
                    }
                }
                return null;
            }
        };
        boolean installed=false;
        boolean verified=false;
        boolean removed=false;
        try {
            instrumentation.addTransformer(passive,true);
            installed=true;
            instrumentation.retransformClasses(target);
            verified=observed.get()==1 && exact.get();
        } catch(Throwable failure) {
            System.err.println("R75_RENDER_RESTORE_FAIL "+failure.getClass().getSimpleName());
        } finally {
            if(installed) {
                try {removed=instrumentation.removeTransformer(passive);}
                catch(Throwable failure) {
                    System.err.println("R75_RESTORE_INSPECTOR_REMOVE_FAIL "+
                                       failure.getClass().getSimpleName());
                }
            }
        }
        // Never release the shared hook gate if our passive inspector leaked.
        return verified && removed;
    }

    /**
     * R85: Never trigger another class retransformation when we cannot prove
     * the bytecode-weaving transformer was removed. A leaked transformer
     * could reweave the original method while "restoring" it. In that case
     * the shared admission lock must remain held for this JVM.
     */
    private static boolean restoreIfWeaverRemoved(boolean removalConfirmed,
                                                   Instrumentation instrumentation,
                                                   Class<?> target) {
        if(!removalConfirmed) {
            System.err.println("R85_UNCERTAIN_WEAVE_REMOVAL_SKIP_RETRANSFORM");
            return false;
        }
        return restore(instrumentation,target);
    }

    /** Existing agent admission guard must already have been acquired. */
    public static void start(final Instrumentation instrumentation) throws Exception {
        start(instrumentation,null,false,false,false);
    }

    /** Non-null DLL path enables only R78 read-only current-WGL sampling. */
    public static void start(final Instrumentation instrumentation,
                             final String contextDllPath) throws Exception {
        start(instrumentation,contextDllPath,false,false,false);
    }

    /** R91 is a distinct explicit, finite, first-party GL11 visual probe. */
    public static void start(final Instrumentation instrumentation,
                             final String contextDllPath,
                             final boolean renderOverlay) throws Exception {
        start(instrumentation,contextDllPath,renderOverlay,false,false);
    }

    public static void start(final Instrumentation instrumentation,
                             final String contextDllPath,
                             final boolean renderOverlay,
                             final boolean multiFrame) throws Exception {
        start(instrumentation,contextDllPath,renderOverlay,multiFrame,false);
    }

    public static void start(final Instrumentation instrumentation,
                             final String contextDllPath,
                             final boolean renderOverlay,
                             final boolean multiFrame,
                             final boolean scriptedVisibility) throws Exception {
        start(instrumentation,contextDllPath,renderOverlay,multiFrame,
              scriptedVisibility,false);
    }

    public static void start(final Instrumentation instrumentation,
                             final String contextDllPath,
                             final boolean renderOverlay,
                             final boolean multiFrame,
                             final boolean scriptedVisibility,
                             final boolean manualVisibility) throws Exception {
        start(instrumentation,contextDllPath,renderOverlay,multiFrame,
              scriptedVisibility,manualVisibility,false);
    }

    public static void start(final Instrumentation instrumentation,
                             final String contextDllPath,
                             final boolean renderOverlay,
                             final boolean multiFrame,
                             final boolean scriptedVisibility,
                             final boolean manualVisibility,
                             final boolean retainedPreview) throws Exception {
        start(instrumentation,contextDllPath,renderOverlay,multiFrame,
              scriptedVisibility,manualVisibility,retainedPreview,false);
    }

    public static void start(final Instrumentation instrumentation,
                             final String contextDllPath,
                             final boolean renderOverlay,
                             final boolean multiFrame,
                             final boolean scriptedVisibility,
                             final boolean manualVisibility,
                             final boolean retainedPreview,
                             final boolean operatorMenu) throws Exception {
        start(instrumentation,contextDllPath,renderOverlay,multiFrame,
              scriptedVisibility,manualVisibility,retainedPreview,operatorMenu,false);
    }
    public static void start(final Instrumentation instrumentation,
                             final String contextDllPath,
                             final boolean renderOverlay,
                             final boolean multiFrame,
                             final boolean scriptedVisibility,
                             final boolean manualVisibility,
                             final boolean retainedPreview,
                             final boolean operatorMenu,
                             final boolean windowAudit) throws Exception {
        if(windowAudit && (renderOverlay || multiFrame || scriptedVisibility ||
                           manualVisibility || retainedPreview || operatorMenu))
            throw new IllegalArgumentException("R101 window audit is exclusive read-only admission");
        if(operatorMenu && (!renderOverlay || multiFrame ||
                            scriptedVisibility || manualVisibility || retainedPreview))
            throw new IllegalArgumentException("R100 finite operator menu is exclusive");
        if(retainedPreview && (!renderOverlay || multiFrame ||
                               scriptedVisibility || manualVisibility))
            throw new IllegalArgumentException("R99 finite retained menu requires exclusive original mode");
        if(manualVisibility && (multiFrame || scriptedVisibility || !renderOverlay))
            throw new IllegalArgumentException("R94 manual mode requires independent consent");
        if(scriptedVisibility && (multiFrame || !renderOverlay))
            throw new IllegalArgumentException("R93 show/hide is a separate finite overlay mode");
        if(multiFrame && !renderOverlay)
            throw new IllegalArgumentException("R92 multi-frame requires overlay");
        if(renderOverlay && contextDllPath==null)
            throw new IllegalArgumentException("R91 native WGL DLL required");
        // R80: before the weaving transformer is registered, no renderer
        // bytes have been altered by this diagnostic. An original-class SHA,
        // callback visibility, or native-library admission failure must NOT
        // strand the process-wide admission gate forever. Installation and
        // restoration failures below remain fail-closed independently.
        final Class<?> target;
        final RendererWglEvidence evidence;
        try {
            target=RendererLivePreflight.admit(instrumentation);
            InstrumentationProbe.verifyCallbackVisibility(target);
            final ClassLoader loader=target.getClassLoader();
            if(Class.forName(RendererFrameDiagnostic.class.getName(),false,loader)!=
                    RendererFrameDiagnostic.class)
                throw new IllegalStateException("R75 render callback resolves to wrong loader");

            // Load AFTER original Mojang SHA and classloader admission, BEFORE
            // registering a bytecode-changing transformer. Never draw.
            evidence=contextDllPath==null?null:
                RendererWglEvidence.open(contextDllPath,renderOverlay,multiFrame,scriptedVisibility,manualVisibility,retainedPreview,operatorMenu,windowAudit);
            if(windowAudit) {
                // R173: resolve the unique ALREADY LOADED original ave
                // class after R73 has pinned the full ave/bfk bytecode.
                // No class is redefined, no screen is opened or closed.
                Class<?> game=null;
                for(Class<?> loaded:instrumentation.getAllLoadedClasses()) {
                    if("ave".equals(loaded.getName())) {
                        if(game!=null)
                            throw new IllegalStateException(
                                "R173 multiple original GUI game owners");
                        game=loaded;
                    }
                }
                if(game==null || game.getClassLoader()!=loader ||
                   !evidence.bindOriginalGuiObserver(
                       OriginalGuiScreenProbe.bind(game,loader)))
                    throw new IllegalStateException(
                        "R173 original GUI screen read-only binding refused");
            }
        } catch(RendererLivePreflight.InspectorRemovalUncertain uncertain) {
            // The read-only class inspector may still be installed. Do not
            // allow another attach while its removal is unproven.
            throw uncertain;
        } catch(Exception preWeaveFailure) {
            // Original SHA/classloader/JNI admission did not weave game bytes.
            InstrumentationProbe.finishUnmodifiedRenderAdmission();
            throw preWeaveFailure;
        }
        // An unexpected VM LinkageError is not known to be recoverable:
        // leave admission locked instead of guessing about transformer state.
        final int maxFrames=(manualVisibility || operatorMenu)?2048:
            ((multiFrame || scriptedVisibility || retainedPreview)?72:8);
        final RenderFrameGate session=new RenderFrameGate(
            maxFrames,multiFrame || scriptedVisibility || retainedPreview,
            manualVisibility || operatorMenu);
        final RenderCallbackFence fence=new RenderCallbackFence(session,
            evidence==null ? null : new Runnable() {
                @Override public void run() {
                    evidence.sample();
                    if((manualVisibility && (evidence.manualStopped() ||
                                             evidence.manualFailed())) ||
                       (operatorMenu && (evidence.operatorStopped() ||
                                         evidence.operatorFailed())))
                        session.stop();
                }
            });
        final AtomicInteger changed=new AtomicInteger();
        final ClassFileTransformer transformer=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader owner,String name,
                    Class<?> type,ProtectionDomain protection,byte[] raw) {
                if(type!=target)return null;
                changed.incrementAndGet();
                try {
                    RendererLivePreflight.checkPinnedRuntimeRenderer(target,raw);
                } catch(Exception mismatch) {
                    throw new IllegalStateException(
                        "R90 original-source/loaded-code admission failed before weave",
                        mismatch);
                }
                return weaveValidatedRuntime(raw,multiFrame || scriptedVisibility || manualVisibility || retainedPreview || operatorMenu);
            }
        };
        activeFence=fence;
        boolean added=false;
        Throwable installationFailure=null;
        boolean weaveRemoved=false;
        try {
            instrumentation.addTransformer(transformer,true);
            added=true;
            instrumentation.retransformClasses(target);
        } catch(Throwable problem) {
            installationFailure=problem;
        } finally {
            // Never restore while the weaving transformer remains registered.
            if(added) {
                try {
                    weaveRemoved=instrumentation.removeTransformer(transformer);
                    if(!weaveRemoved && installationFailure==null)
                        installationFailure=new IllegalStateException(
                            "R75 weaving transformer removal unconfirmed");
                } catch(Throwable removalError) {
                    installationFailure=removalError;
                }
            }
        }
        if(installationFailure!=null || changed.get()!=1) {
            activeFence=null;
            fence.stopAndDrain(); // No in-flight observer at restore entry.
            final boolean manualClosed=evidence==null || evidence.closeManual();
            // A leaking transformer MUST NOT see a new retransformation.
            // In particular addTransformer() may have registered it before
            // throwing, or removeTransformer() may have returned false/thrown.
            boolean recovered=restoreIfWeaverRemoved(
                weaveRemoved,instrumentation,target);
            InstrumentationProbe.finishRenderDiagnostic(recovered && manualClosed);
            if(installationFailure!=null)
                throw new IllegalStateException(
                    "R75 renderer callback installation refused",installationFailure);
            throw new IllegalStateException("R75 renderer not transformed exactly once");
        }

        Thread cleanup=new Thread(new Runnable() {
            @Override public void run() {
                boolean completed=false;
                try {completed=session.await((manualVisibility || operatorMenu)?20:12,TimeUnit.SECONDS);}
                catch(InterruptedException interrupted){Thread.currentThread().interrupt();}
                activeFence=null; // Future callbacks immediately become no-ops.
                fence.stopAndDrain(); // Also joins an in-flight native WGL read.
                final boolean manualClosed=evidence==null || evidence.closeManual();
                final boolean restored=restore(instrumentation,target);
                if(restored)
                    System.out.println("R90_RESTORED_SOURCE_PINNED_EXECUTION_PASS");
                else
                    System.err.println("R75_RESTORE_UNCERTAIN_NO_REATTACH");
                if(completed && (session.accepted()==maxFrames ||
                                 ((manualVisibility && evidence!=null && evidence.manualStopped()) ||
                                  (operatorMenu && evidence!=null && evidence.operatorStopped()))) &&
                   !session.invalid() && !fence.failed() && restored && manualClosed)
                    System.out.println((windowAudit?
                        "R101_RENDER_CALLBACK_BOUNDED_PASS accepted=8 owner=":
                        (operatorMenu?
                        "R100_RENDER_CALLBACK_BOUNDED_PASS accepted="+session.accepted()+" owner=":
                        (retainedPreview?
                        "R99_RENDER_CALLBACK_BOUNDED_PASS accepted=72 owner=":
                        (manualVisibility?
                         "R94_RENDER_CALLBACK_BOUNDED_PASS accepted="+session.accepted()+" owner=":
                         (scriptedVisibility?
                          "R93_RENDER_CALLBACK_BOUNDED_PASS accepted=72 owner=":
                          (multiFrame?"R92_RENDER_CALLBACK_BOUNDED_PASS accepted=72 owner=":
                           "R75_RENDER_CALLBACK_BOUNDED_PASS accepted=8 owner="))))))+
                        session.ownerThread());
                else
                    System.err.println("R75_RENDER_CALLBACK_UNVERIFIED frames="+
                                       session.accepted()+" invalid="+session.invalid()+
                                       " complete="+completed);
                if(evidence!=null) {
                    final boolean stable=completed && !session.invalid() &&
                                         !fence.failed() &&
                                         (session.accepted()==maxFrames ||
                                          (manualVisibility && evidence.manualStopped()) ||
                                          (operatorMenu && evidence.operatorStopped())) &&
                                         restored && manualClosed && evidence.fullyStable() &&
                                         (!windowAudit || evidence.windowIdentityVerified());
                    System.out.println((stable?
                        (windowAudit?"R101_WGL_CONTEXT_STABLE_PASS ":
                         (operatorMenu?"R100_WGL_CONTEXT_STABLE_PASS ":
                         (retainedPreview?"R99_WGL_CONTEXT_STABLE_PASS ":
                         (manualVisibility?"R94_WGL_CONTEXT_STABLE_PASS ":
                          (scriptedVisibility?"R93_WGL_CONTEXT_STABLE_PASS ":
                           (multiFrame?"R92_WGL_CONTEXT_STABLE_PASS ":
                                       "R78_READ_ONLY_WGL_CONTEXT_STABLE_PASS ")))))):
                        (windowAudit?"R101_WGL_CONTEXT_UNVERIFIED ":
                         (operatorMenu?"R100_WGL_CONTEXT_UNVERIFIED ":
                         (retainedPreview?"R99_WGL_CONTEXT_UNVERIFIED ":
                         (manualVisibility?"R94_WGL_CONTEXT_UNVERIFIED ":
                          (scriptedVisibility?"R93_WGL_CONTEXT_UNVERIFIED ":
                           (multiFrame?"R92_WGL_CONTEXT_UNVERIFIED ":
                                       "R78_READ_ONLY_WGL_CONTEXT_UNVERIFIED ")))))))+
                        evidence.result());
                    if(windowAudit) {
                        if(stable) {
                            System.out.println("R101_ORIGINAL_GAME_WINDOW_IDENTIFIED_PASS");
                            if(evidence.originalGuiReadOnlyVerified())
                                System.out.println(
                                    "R173_ORIGINAL_GUI_SCREEN_STATE_READ_ONLY_PASS open="+
                                    evidence.observedOriginalGuiPresent()+
                                    " closed="+evidence.observedOriginalGuiAbsent()+
                                    " refused="+evidence.refusedOriginalGuiObservations());
                            if(evidence.inputRiskAuditVerified())
                                System.out.println(
                                    "R174_ORIGINAL_INPUT_OVERLAP_RISK_AUDIT_PASS candidates="+
                                    evidence.possibleVanillaDoubleDispatchPresses()+
                                    " cancelled="+evidence.cancelledInputRiskCandidates()+
                                    " native_click_delivery=disabled");
                        }
                        else
                            System.err.println("R101_GAME_WINDOW_IDENTITY_UNVERIFIED");
                    }
                    if(manualVisibility && stable && evidence.overlayDrawn()) {
                        // Distinct audit (not a standalone success marker):
                        // original SHA, WGL, restoration and release remain
                        // mandatory. Never log raw keys or WGL handles.
                        System.out.println("R95_INPUT_SOURCE_AUDIT keys="+
                                           evidence.keyToggles()+" events="+
                                           evidence.eventToggles());
                    }
                    if(retainedPreview && stable &&
                       evidence.passiveHoverVerified()) {
                        final int[] hover=evidence.passiveHoverCounters();
                        if(hover!=null && hover.length==5)
                            System.out.println(
                                "R176_ORIGINAL_WINDOW_PASSIVE_HOVER_RECEIPT_PASS "+
                                "frames="+hover[0]+" samples="+hover[1]+
                                " foreground="+hover[2]+
                                " numeric="+hover[3]+" bind="+hover[4]+
                                " native_click_delivery=disabled");
                        // R178 is a source-validated opportunity, not an
                        // assertion that a physical cursor actually hovered.
                        // Exact R176 counters retain that separate meaning.
                        if(hover!=null && hover.length==5)
                            System.out.println(
                                "R178_ORIGINAL_SOURCE_SLIDER_HOVER_WINDOW_PASS "+
                                "eligible_draws=60 frames=64 actions=4 "+
                                "foreground="+hover[2]+
                                " native_click_delivery=disabled");
                    }
                    if(retainedPreview && stable &&
                       evidence.passiveHoverVerified()) {
                        final int[] target=evidence.originalSourceTargetCounters();
                        if(target!=null && target.length==3)
                            System.out.println(
                                "R179_ORIGINAL_NATIVE_SOURCE_TARGET_PASS "+
                                "frames="+target[0]+" source_draws="+target[1]+
                                " exact_hits="+target[2]+
                                " native_click_delivery=disabled");
                    }
                    if(evidence.overlayRequested()) {
                        if(stable && evidence.overlayDrawn())
                            System.out.println(operatorMenu?
                                "R100_OPERATOR_MENU_CONTROLLED_PASS actions=4 tab=1 focus=17 preview=1 stop=operator":
                                (retainedPreview?
                                "R99_RETAINED_MENU_DRAW_PASS frames=64 actions=4 tab=1 focus=17":
                                (manualVisibility?
                                 "R94_MENU_OPERATOR_CONTROLLED_PASS frames="+session.accepted()+
                                 " toggles="+evidence.toggles()+" shown="+evidence.overlayFrames()+
                                 " hidden="+evidence.hiddenFrames()+" stop=operator":
                                 (scriptedVisibility?
                                  "R93_MENU_SHOW_HIDE_SHOW_PASS shown=48 hidden=16":
                                  (multiFrame?"R92_MULTIFRAME_OVERLAY_DRAW_PASS frames=64":
                                              "R91_ORIGINAL_RENDER_OVERLAY_DRAW_PASS frames=1")))));
                        else
                            System.err.println(operatorMenu?
                                "R100_OPERATOR_MENU_UNVERIFIED":
                                (retainedPreview?
                                "R99_RETAINED_MENU_UNVERIFIED":
                                (manualVisibility?
                                 "R94_OPERATOR_VISIBILITY_UNVERIFIED":
                                 (scriptedVisibility?
                                  "R93_MENU_VISIBILITY_UNVERIFIED":
                                  (multiFrame?"R92_MULTIFRAME_OVERLAY_UNVERIFIED":
                                              "R91_RENDER_OVERLAY_UNVERIFIED")))));
                    }
                }
                System.out.println(evidence==null?
                    "R75_RENDER_CALLBACK_NO_GRAPHICS_OR_GAMEPLAY_ACTIONS":
                    (windowAudit?"R101_READ_ONLY_GAME_WINDOW_NO_INPUT_OR_GAMEPLAY_ACTIONS":
                     (operatorMenu?"R100_EXTERNAL_EVENT_PREVIEW_NO_GAME_INPUT_ACTIONS":
                     (retainedPreview?"R99_RETAINED_LOCAL_PREVIEW_NO_GAME_INPUT_ACTIONS":
                     (manualVisibility?"R94_FOREGROUND_INSERT_OR_EVENT_NO_GAMEPLAY_ACTIONS":
                      (scriptedVisibility?"R93_SCRIPTED_SHOW_HIDE_NO_GAMEPLAY_OR_INPUT_ACTIONS":
                       (multiFrame?"R92_FINITE_GL_OVERLAY_NO_GAMEPLAY_OR_INPUT_ACTIONS":
                        (renderOverlay?"R91_ONE_FRAME_GL_OVERLAY_NO_GAMEPLAY_OR_INPUT_ACTIONS":
                         "R78_READ_ONLY_WGL_NO_RENDER_OR_GAMEPLAY_ACTIONS"))))))));
                System.out.flush();
                InstrumentationProbe.finishRenderDiagnostic(restored && manualClosed);
                // R81: consumer must not start the next Attach merely from a
                // restoration receipt printed before the admission release.
                // Emit this marker ONLY after the verified gate is released.
                if(restored && manualClosed) {
                    System.out.println("R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS");
                    System.out.flush();
                }
            }
        },"SlinkyWinky-R75-Finite-Restore");
        cleanup.setDaemon(true);
        try {
            cleanup.start();
        } catch(Throwable workerFailure) {
            activeFence=null;
            fence.stopAndDrain();
            final boolean manualClosed=evidence==null || evidence.closeManual();
            final boolean restored=restoreIfWeaverRemoved(
                weaveRemoved,instrumentation,target);
            InstrumentationProbe.finishRenderDiagnostic(restored && manualClosed);
            throw new IllegalStateException(
                "R75 cleanup worker unavailable; original renderer restored="+restored,
                workerFailure);
        }
        System.out.println(windowAudit?
            "R101_RENDER_CALLBACK_ARMED_LIMIT=8":
            (operatorMenu?
            "R100_RENDER_CALLBACK_ARMED_LIMIT=2048":
            (retainedPreview?
            "R99_RENDER_CALLBACK_ARMED_LIMIT=72":
            (manualVisibility?"R94_RENDER_CALLBACK_ARMED_LIMIT=2048":
             (scriptedVisibility?"R93_RENDER_CALLBACK_ARMED_LIMIT=72":
              (multiFrame?"R92_RENDER_CALLBACK_ARMED_LIMIT=72":
                          "R75_RENDER_CALLBACK_ARMED_LIMIT=8"))))));
        System.out.flush();
    }

    private RendererFrameDiagnostic(){}
}
