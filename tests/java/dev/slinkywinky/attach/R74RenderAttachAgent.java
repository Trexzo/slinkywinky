package dev.slinkywinky.attach;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.MessageDigest;
import java.security.ProtectionDomain;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import dev.slinkywinky.instrument.RendererLivePreflight;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** R74 test-only agent targeting generated `ave` and `bfk` classes.
 * The commercial/Mojang client is NEVER targeted by this agent.
 */
public final class R74RenderAttachAgent {
    private static final AtomicBoolean admitted=new AtomicBoolean();
    private static volatile boolean ready;
    private static volatile Class<?> targetClass;
    private static volatile Instrumentation attached;
    private static volatile byte[] baselineHash;

    private static byte[] hash(byte[] bytes) {
        try {return MessageDigest.getInstance("SHA-256").digest(bytes);}
        catch(Exception error){throw new IllegalStateException(error);}
    }
    private static String hex(byte[] bytes) {
        StringBuilder s=new StringBuilder();
        for(byte b:bytes)s.append(String.format("%02x",b&0xff));
        return s.toString();
    }
    public static boolean ready(){return ready;}

    private static byte[] weave(byte[] raw,final boolean r77) {
        if(raw==null || raw.length==0 || raw.length>65536)
            throw new IllegalArgumentException("Invalid generated renderer length");
        ClassReader reader=new ClassReader(raw);
        if(!"bfk".equals(reader.getClassName()))
            throw new IllegalArgumentException("R74 fixture must be exactly bfk");
        ClassWriter writer=new ClassWriter(reader,ClassWriter.COMPUTE_MAXS);
        final AtomicInteger matches=new AtomicInteger(), competing=new AtomicInteger();
        reader.accept(new ClassVisitor(Opcodes.ASM5,writer) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String signature,String[] exceptions) {
                MethodVisitor child=super.visitMethod(access,name,desc,signature,exceptions);
                if(!"a".equals(name))return child;
                competing.incrementAndGet();
                if(!"(FJ)V".equals(desc) || (access&Opcodes.ACC_PUBLIC)==0 ||
                   (access&(Opcodes.ACC_STATIC|Opcodes.ACC_ABSTRACT|Opcodes.ACC_NATIVE))!=0)
                    return child;
                matches.incrementAndGet();
                return new MethodVisitor(Opcodes.ASM5,child) {
                    @Override public void visitCode() {
                        super.visitCode();
                        super.visitVarInsn(Opcodes.FLOAD,1);
                        super.visitVarInsn(Opcodes.LLOAD,2);
                        super.visitMethodInsn(Opcodes.INVOKESTATIC,
                            r77 ? "dev/slinkywinky/attach/R77WglRenderReceiver" :
                                  "dev/slinkywinky/instrument/RenderCallbackProbe",
                            "onRender","(FJ)V",false);
                    }
                };
            }
        },0);
        if(matches.get()!=1 || competing.get()!=1)
            throw new IllegalArgumentException("R74 generated renderer method ambiguous");
        return writer.toByteArray();
    }

    public static void agentmain(String option,Instrumentation inst) throws Exception {
        final boolean r77="R77_DISPOSABLE_WGL_ONLY".equals(option);
        if((!r77 && !"R74_DISPOSABLE_OBFUSCATED_ONLY".equals(option)) ||
           !"1".equals(System.getenv("SLINKYWINKY_R74_DISPOSABLE_FIXTURE")) ||
           (r77 && (!"1".equals(System.getenv("SLINKYWINKY_R76_WGL_FIXTURE")) ||
                     !"1".equals(System.getenv("SLINKYWINKY_R77_DISPOSABLE_FIXTURE")) ||
                     !"dev.slinkywinky.attach.R77WglRenderTarget".equals(
                         System.getProperty("sun.java.command")))))
            throw new IllegalArgumentException("R74/R77 exact disposable opt-in required");
        if(inst==null || !inst.isRetransformClassesSupported())
            throw new IllegalStateException("R74 Java 8 retransformation unavailable");
        if(!admitted.compareAndSet(false,true))
            throw new IllegalStateException("R74 duplicate attach refused");
        Class<?> renderer=null,game=null;
        for(Class<?> loaded:inst.getAllLoadedClasses()) {
            if("bfk".equals(loaded.getName())) {
                if(renderer!=null)throw new IllegalStateException("Duplicate bfk classes");
                renderer=loaded;
            }
            if("ave".equals(loaded.getName())) {
                if(game!=null)throw new IllegalStateException("Duplicate ave classes");
                game=loaded;
            }
        }
        if(renderer==null || game==null ||
           renderer.getClassLoader()!=ClassLoader.getSystemClassLoader() ||
           game.getClassLoader()!=renderer.getClassLoader() ||
           !inst.isModifiableClass(renderer) || !inst.isModifiableClass(game))
            throw new IllegalStateException("Only system-loaded generated ave/bfk accepted");
        final Class<?> selected=renderer, gameTarget=game;
        // A passive 2-class snapshot first checks the R73 structural contract,
        // with synthetic-only hashes; production original hashes are immutable.
        final byte[][] copies=new byte[2][];
        final AtomicInteger[] observed={new AtomicInteger(),new AtomicInteger()};
        ClassFileTransformer passive=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader,String name,
                    Class<?> current,ProtectionDomain domain,byte[] bytes) {
                if(current==gameTarget) {copies[0]=bytes.clone();observed[0].incrementAndGet();}
                if(current==selected) {copies[1]=bytes.clone();observed[1].incrementAndGet();}
                return null;
            }
        };
        inst.addTransformer(passive,true);
        try {inst.retransformClasses(gameTarget,selected);}
        finally {inst.removeTransformer(passive);}
        if(observed[0].get()!=1 || observed[1].get()!=1)
            throw new IllegalStateException("Synthetic renderer snapshot incomplete");
        RendererLivePreflight.checkExactClasses(copies[0],copies[1],
            hex(hash(copies[0])),hex(hash(copies[1])));
        final AtomicInteger modified=new AtomicInteger();
        ClassFileTransformer worker=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader,String name,
                    Class<?> current,ProtectionDomain domain,byte[] bytes) {
                if(current!=selected)return null;
                modified.incrementAndGet();
                baselineHash=hash(bytes);
                return weave(bytes,r77);
            }
        };
        inst.addTransformer(worker,true);
        try {inst.retransformClasses(selected);}
        finally {inst.removeTransformer(worker);}
        if(modified.get()!=1 || baselineHash==null)
            throw new IllegalStateException("Synthetic bfk not woven exactly once");
        attached=inst;
        targetClass=selected;
        ready=true;
        System.out.println(r77 ? "R77_EXACT_GENERATED_WGL_RENDER_HOOK_PASS" :
                                 "R74_EXACT_OBFUSCATED_FIXTURE_HOOK_PASS");
        System.out.flush();
    }

    /** Called by the same synthetic render-owner thread, outside a frame. */
    public static void restoreOnOwner() throws Exception {
        if(!ready || !admitted.get() ||
           !"1".equals(System.getenv("SLINKYWINKY_R74_DISPOSABLE_FIXTURE")))
            throw new IllegalStateException("R74 fixture not active");
        ready=false;
        final Class<?> selected=targetClass;
        final Instrumentation inst=attached;
        final byte[] expected=baselineHash;
        if(inst==null || selected==null || expected==null)
            throw new IllegalStateException("R74 missing baseline receipt");
        final AtomicInteger observed=new AtomicInteger();
        final AtomicBoolean same=new AtomicBoolean();
        ClassFileTransformer inspector=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader,String name,
                    Class<?> current,ProtectionDomain domain,byte[] bytes) {
                if(current==selected) {
                    observed.incrementAndGet();
                    same.set(Arrays.equals(expected,hash(bytes)));
                }
                return null;
            }
        };
        inst.addTransformer(inspector,true);
        try {inst.retransformClasses(selected);}
        finally {inst.removeTransformer(inspector);}
        if(observed.get()!=1 || !same.get())
            throw new IllegalStateException("R74 exact original fixture restoration failed");
        attached=null;targetClass=null;baselineHash=null;
        System.out.println("R74_OBFUSCATED_CLASS_RESTORE_SHA256_PASS");
        System.out.flush();
    }
    private R74RenderAttachAgent(){}
}
