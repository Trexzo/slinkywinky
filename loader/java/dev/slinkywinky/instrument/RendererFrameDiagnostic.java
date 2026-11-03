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
 * R75: opt-in, finite, Java-only callback diagnostic on exact ORIGINAL bfk.
 *
 * No GL, LWJGL, WGL, JNI, UI drawing, input interception or game state access.
 * A daemon retransform restores original bytecode (or locks future admission).
 */
public final class RendererFrameDiagnostic {
    private static final String PINNED_BFK_SHA256 =
        "8a4ce93bbf272398ea298d2ed363f75211dd469fcd3b66682a387e4738a419a2";
    private static volatile RenderFrameGate activeGate;

    private static byte[] hash(byte[] data) {
        try {return MessageDigest.getInstance("SHA-256").digest(data);}
        catch(Exception error){throw new IllegalStateException(error);}
    }
    private static String hex(byte[] data) {
        StringBuilder result=new StringBuilder(64);
        for(byte item:data)result.append(String.format("%02x",item&0xff));
        return result.toString();
    }

    /** Called only by the opt-in classfile prefix, never performs side effects. */
    public static void onRender(float partialTicks,long frameTime) {
        final RenderFrameGate gate=activeGate;
        if(gate==null)return;
        try {
            gate.sample(Thread.currentThread(),partialTicks,frameTime,
                Thread.currentThread().getStackTrace());
        } catch(Throwable unexpected) {
            // Do not throw into Mojang rendering. Stop all further sampling.
            gate.stop();
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
        try {
            instrumentation.addTransformer(passive,true);
            installed=true;
            instrumentation.retransformClasses(target);
            return observed.get()==1 && exact.get();
        } catch(Throwable failure) {
            System.err.println("R75_RENDER_RESTORE_FAIL "+failure.getClass().getSimpleName());
            return false;
        } finally {
            if(installed)instrumentation.removeTransformer(passive);
        }
    }

    /** Existing agent admission guard must already have been acquired. */
    public static void start(final Instrumentation instrumentation) throws Exception {
        final Class<?> target=RendererLivePreflight.admit(instrumentation);
        InstrumentationProbe.verifyCallbackVisibility(target);
        final ClassLoader loader=target.getClassLoader();
        if(Class.forName(RendererFrameDiagnostic.class.getName(),false,loader)!=
                RendererFrameDiagnostic.class)
            throw new IllegalStateException("R75 render callback resolves to wrong loader");

        final RenderFrameGate session=new RenderFrameGate(8);
        final AtomicInteger changed=new AtomicInteger();
        final ClassFileTransformer transformer=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader owner,String name,
                    Class<?> type,ProtectionDomain protection,byte[] raw) {
                if(type!=target)return null;
                changed.incrementAndGet();
                return weavePinnedOriginal(raw);
            }
        };
        activeGate=session;
        boolean added=false;
        Throwable installationFailure=null;
        try {
            instrumentation.addTransformer(transformer,true);
            added=true;
            instrumentation.retransformClasses(target);
        } catch(Throwable problem) {
            installationFailure=problem;
        } finally {
            // Never restore while the weaving transformer remains registered.
            if(added)instrumentation.removeTransformer(transformer);
        }
        if(installationFailure!=null || changed.get()!=1) {
            activeGate=null;
            session.stop();
            boolean recovered=restore(instrumentation,target);
            InstrumentationProbe.finishRenderDiagnostic(recovered);
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
                activeGate=null; // Disable callback before attempting bytecode restoration.
                session.stop();
                try {Thread.sleep(100L);}
                catch(InterruptedException interrupted){Thread.currentThread().interrupt();}
                final boolean restored=restore(instrumentation,target);
                if(restored)
                    System.out.println("R75_ORIGINAL_RENDER_CLASS_RESTORE_SHA256_PASS");
                else
                    System.err.println("R75_RESTORE_UNCERTAIN_NO_REATTACH");
                if(completed && session.accepted()==8 && !session.invalid() && restored)
                    System.out.println("R75_RENDER_CALLBACK_BOUNDED_PASS accepted=8 owner="+
                                       session.ownerThread());
                else
                    System.err.println("R75_RENDER_CALLBACK_UNVERIFIED frames="+
                                       session.accepted()+" invalid="+session.invalid()+
                                       " complete="+completed);
                System.out.println("R75_RENDER_CALLBACK_NO_GRAPHICS_OR_GAMEPLAY_ACTIONS");
                System.out.flush();
                InstrumentationProbe.finishRenderDiagnostic(restored);
            }
        },"SlinkyWinky-R75-Finite-Restore");
        cleanup.setDaemon(true);
        cleanup.start();
        System.out.println("R75_RENDER_CALLBACK_ARMED_LIMIT=8");
        System.out.flush();
    }

    private RendererFrameDiagnostic(){}
}
