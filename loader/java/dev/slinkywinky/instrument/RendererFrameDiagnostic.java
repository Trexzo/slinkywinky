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
 * R78 optionally observes current WGL/viewport through read-only JNI after
 * strict R79 render-thread admission. Neither path draws UI or changes GL.
 * A daemon retransform restores original bytecode (or locks future admission).
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
                    @Override public void visitCode() {
                        super.visitCode();
                        super.visitVarInsn(Opcodes.FLOAD,1);
                        super.visitVarInsn(Opcodes.LLOAD,2);
                        super.visitMethodInsn(Opcodes.INVOKESTATIC,
                            "dev/slinkywinky/instrument/RendererFrameDiagnostic",
                            "onRender","(FJ)V",false);
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
                    exact.set(raw!=null && PINNED_BFK_SHA256.equals(hex(hash(raw))));
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

    /** Existing agent admission guard must already have been acquired. */
    public static void start(final Instrumentation instrumentation) throws Exception {
        start(instrumentation,null);
    }

    /** Non-null DLL path enables only R78 read-only current-WGL sampling. */
    public static void start(final Instrumentation instrumentation,
                             final String contextDllPath) throws Exception {
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
                RendererWglEvidence.open(contextDllPath);
        } catch(Exception preWeaveFailure) {
            InstrumentationProbe.finishUnmodifiedRenderAdmission();
            throw preWeaveFailure;
        } catch(LinkageError preWeaveFailure) {
            InstrumentationProbe.finishUnmodifiedRenderAdmission();
            throw preWeaveFailure;
        }
        final RenderFrameGate session=new RenderFrameGate(8);
        final RenderCallbackFence fence=new RenderCallbackFence(session,
            evidence==null ? null : new Runnable() {
                @Override public void run() {evidence.sample();}
            });
        final AtomicInteger changed=new AtomicInteger();
        final ClassFileTransformer transformer=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader owner,String name,
                    Class<?> type,ProtectionDomain protection,byte[] raw) {
                if(type!=target)return null;
                changed.incrementAndGet();
                return weavePinnedOriginal(raw);
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
            boolean recovered=restore(instrumentation,target);
            // Uncertain removal means the weaving transformer may remain active.
            InstrumentationProbe.finishRenderDiagnostic(recovered && weaveRemoved);
            if(installationFailure!=null)
                throw new IllegalStateException(
                    "R75 renderer callback installation refused",installationFailure);
            throw new IllegalStateException("R75 renderer not transformed exactly once");
        }

        Thread cleanup=new Thread(new Runnable() {
            @Override public void run() {
                boolean completed=false;
                try {completed=session.await(12,TimeUnit.SECONDS);}
                catch(InterruptedException interrupted){Thread.currentThread().interrupt();}
                activeFence=null; // Future callbacks immediately become no-ops.
                fence.stopAndDrain(); // Also joins an in-flight native WGL read.
                final boolean restored=restore(instrumentation,target);
                if(restored)
                    System.out.println("R75_ORIGINAL_RENDER_CLASS_RESTORE_SHA256_PASS");
                else
                    System.err.println("R75_RESTORE_UNCERTAIN_NO_REATTACH");
                if(completed && session.accepted()==8 && !session.invalid() &&
                   !fence.failed() && restored)
                    System.out.println("R75_RENDER_CALLBACK_BOUNDED_PASS accepted=8 owner="+
                                       session.ownerThread());
                else
                    System.err.println("R75_RENDER_CALLBACK_UNVERIFIED frames="+
                                       session.accepted()+" invalid="+session.invalid()+
                                       " complete="+completed);
                if(evidence!=null) {
                    final boolean stable=completed && !session.invalid() &&
                                         !fence.failed() && session.accepted()==8 &&
                                         restored && evidence.fullyStable();
                    System.out.println((stable?
                        "R78_READ_ONLY_WGL_CONTEXT_STABLE_PASS ":
                        "R78_READ_ONLY_WGL_CONTEXT_UNVERIFIED ")+
                        evidence.result());
                }
                System.out.println(evidence==null?
                    "R75_RENDER_CALLBACK_NO_GRAPHICS_OR_GAMEPLAY_ACTIONS":
                    "R78_READ_ONLY_WGL_NO_RENDER_OR_GAMEPLAY_ACTIONS");
                System.out.flush();
                InstrumentationProbe.finishRenderDiagnostic(restored);
            }
        },"SlinkyWinky-R75-Finite-Restore");
        cleanup.setDaemon(true);
        try {
            cleanup.start();
        } catch(Throwable workerFailure) {
            activeFence=null;
            fence.stopAndDrain();
            final boolean restored=restore(instrumentation,target);
            InstrumentationProbe.finishRenderDiagnostic(restored);
            throw new IllegalStateException(
                "R75 cleanup worker unavailable; original renderer restored="+restored,
                workerFailure);
        }
        System.out.println("R75_RENDER_CALLBACK_ARMED_LIMIT=8");
        System.out.flush();
    }

    private RendererFrameDiagnostic(){}
}
