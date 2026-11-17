package dev.slinkywinky.instrument;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.ProtectionDomain;
import java.util.concurrent.atomic.AtomicInteger;
import org.objectweb.asm.*;

/**
 * R240: bounded passive entry trace on ORIGINAL source-pinned ave.s()V.
 * No LWJGL calls, OS hooks, game state, input re-routing or suppression.
 * An unproven bytecode restore permanently holds the agent admission gate.
 */
public final class OriginalInputFrameDiagnostic {
    private static final int LIMIT=64;
    private static final AtomicInteger entries=new AtomicInteger();
    private static final AtomicInteger foreign=new AtomicInteger();
    private static boolean active;
    private static Thread owner;
    private OriginalInputFrameDiagnostic(){}

    public static synchronized void onInputEntry() {
        if(!active)return;
        try {
            Thread current=Thread.currentThread();
            if(owner==null)owner=current;
            if(current!=owner) {foreign.incrementAndGet();return;}
            if(entries.get()<LIMIT)entries.incrementAndGet();
        } catch(Throwable problem) {
            foreign.incrementAndGet(); // Never throw into vanilla.
        }
    }
    private static synchronized void begin() {
        if(active)throw new IllegalStateException("R240 already observing");
        entries.set(0);foreign.set(0);owner=null;active=true;
    }
    private static synchronized int[] stopAndDrain() {
        active=false;
        return new int[]{entries.get(),foreign.get()};
    }
    private static byte[] weave(byte[] raw,final String expected,
                                final String method) {
        if(raw==null || raw.length<32 || raw.length>1000000)
            throw new IllegalArgumentException("R240 invalid class bytes");
        ClassReader reader=new ClassReader(raw);
        if(!expected.equals(reader.getClassName()))
            throw new IllegalArgumentException("R240 incorrect class identity");
        final AtomicInteger matches=new AtomicInteger();
        final AtomicInteger existing=new AtomicInteger();
        ClassWriter writer=new ClassWriter(reader,ClassWriter.COMPUTE_MAXS);
        reader.accept(new ClassVisitor(Opcodes.ASM5,writer){
            @Override public MethodVisitor visitMethod(int flags,String name,
                    String desc,String signature,String[] exceptions){
                MethodVisitor downstream=super.visitMethod(
                    flags,name,desc,signature,exceptions);
                final boolean selected=method.equals(name)&&"()V".equals(desc);
                if(selected) {
                    if((flags&Opcodes.ACC_PUBLIC)==0 ||
                       (flags&(Opcodes.ACC_STATIC|Opcodes.ACC_NATIVE|
                               Opcodes.ACC_ABSTRACT))!=0)
                        throw new IllegalArgumentException(
                            "R240 input method flags changed");
                    matches.incrementAndGet();
                }
                return new MethodVisitor(Opcodes.ASM5,downstream){
                    @Override public void visitMethodInsn(int opcode,String own,
                            String called,String descriptor,boolean itf) {
                        if("dev/slinkywinky/instrument/OriginalInputFrameDiagnostic"
                             .equals(own) && "onInputEntry".equals(called))
                            existing.incrementAndGet();
                        super.visitMethodInsn(opcode,own,called,descriptor,itf);
                    }
                    @Override public void visitCode(){
                        super.visitCode();
                        if(selected)super.visitMethodInsn(Opcodes.INVOKESTATIC,
                            "dev/slinkywinky/instrument/OriginalInputFrameDiagnostic",
                            "onInputEntry","()V",false);
                    }
                };
            }
        },0);
        if(matches.get()!=1 || existing.get()!=0)
            throw new IllegalArgumentException(
                "R240 missing, duplicated or already-weaved input entry");
        return writer.toByteArray();
    }
    private static byte[] weaveOriginal(byte[] raw) {
        return weave(raw,"ave","s");
    }
    // R240: run the *same* production weave against exact immutable
    // original Mojang bytes in the standalone source audit; no injection.
    public static byte[] weavePinnedOriginalForOffline(byte[] raw) {
        if(!OriginalKeyboardBridgeWeaver.exactOriginal(raw))
            throw new IllegalArgumentException(
                "R240 expected exact original 1.8.9 ave.class fingerprint");
        return weaveOriginal(raw);
    }

    // Source-restricted, offline-only test path.
    static byte[] weaveFixture(byte[] raw) {
        return weave(raw,"dev/slinkywinky/instrument/R240InputFixture","tick");
    }
    static void fixtureBegin(){begin();}
    static int[] fixtureEnd(){return stopAndDrain();}

    /** Revert only AFTER confirmed removal of the weaving transformer. */
    private static boolean restore(Instrumentation inst,final Class<?> game) {
        final AtomicInteger observed=new AtomicInteger();
        final AtomicInteger matched=new AtomicInteger();
        ClassFileTransformer reader=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader,String name,
                    Class<?> type,ProtectionDomain domain,byte[] raw) {
                if(type!=game)return null;
                observed.incrementAndGet();
                try {
                    RendererLivePreflight.checkPinnedRuntimeInput(game,raw);
                    matched.incrementAndGet();
                }catch(Exception mismatch) {
                    System.err.println("R240_ORIGINAL_INPUT_RESTORE_MISMATCH");
                }
                return null;
            }
        };
        boolean registered=false,removed=false;
        try {
            inst.addTransformer(reader,true);
            registered=true;
            inst.retransformClasses(game);
        }catch(Throwable error) {
            System.err.println("R240_INPUT_RESTORE_FAILED "+
                error.getClass().getSimpleName());
        }finally {
            if(registered){
                try{removed=inst.removeTransformer(reader);}
                catch(Throwable error){removed=false;}
            }
        }
        return registered && removed &&
               observed.get()==1 && matched.get()==1;
    }

    public static void start(final Instrumentation inst) throws Exception {
        // The same original Jar/loader/SHA + runtime shape source audit
        // already used by R90. Any uncertain inspector keeps gate locked.
        final Class<?> renderer=RendererLivePreflight.admit(inst);
        InstrumentationProbe.verifyCallbackVisibility(renderer);
        Class<?> candidate=null;
        for(Class<?> loaded:inst.getAllLoadedClasses())
            if("ave".equals(loaded.getName())) {
                if(candidate!=null)
                    throw new IllegalStateException("R240 multiple input classes");
                candidate=loaded;
            }
        if(candidate==null || candidate.getClassLoader()!=renderer.getClassLoader()
           || !inst.isModifiableClass(candidate)) {
            InstrumentationProbe.finishUnmodifiedRenderAdmission();
            throw new IllegalStateException("R240 original class not admitted");
        }
        final Class<?> game=candidate;
        final AtomicInteger transformed=new AtomicInteger();
        ClassFileTransformer weaver=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader,String name,
                    Class<?> type,ProtectionDomain domain,byte[] raw) {
                if(type!=game)return null;
                try {
                    RendererLivePreflight.checkPinnedRuntimeInput(game,raw);
                    byte[] output=weaveOriginal(raw);
                    transformed.incrementAndGet();
                    return output;
                }catch(Exception bad) {
                    throw new IllegalStateException(
                        "R240 original input source changed",bad);
                }
            }
        };
        boolean registered=false,removed=false;
        try {
            inst.addTransformer(weaver,true);
            registered=true;
            inst.retransformClasses(game);
        }catch(Throwable error) {
            System.err.println("R240_INPUT_WEAVE_UNCERTAIN "+
                error.getClass().getSimpleName());
        }finally {
            if(registered) {
                try{removed=inst.removeTransformer(weaver);}
                catch(Throwable error){removed=false;}
            }
        }
        if(!removed)
            throw new IllegalStateException(
                "R240 weaver removal uncertain: restoration forbidden");
        if(transformed.get()!=1) {
            final boolean clean=restore(inst,game);
            InstrumentationProbe.finishRenderDiagnostic(clean);
            throw new IllegalStateException("R240 failed source weave count");
        }
        begin();
        Thread supervisor=new Thread(new Runnable(){
            @Override public void run() {
                try {
                    for(int i=0;i<800;i++) {
                        synchronized(OriginalInputFrameDiagnostic.class){
                            if(entries.get()>=LIMIT || foreign.get()>0)break;
                        }
                        Thread.sleep(10L);
                    }
                }catch(InterruptedException ignored){
                    Thread.currentThread().interrupt();
                }finally {
                    final int[] counts=stopAndDrain();
                    final boolean restored=restore(inst,game);
                    InstrumentationProbe.finishRenderDiagnostic(restored);
                    System.out.println(
                        "R240_ORIGINAL_VANILLA_INPUT_ENTRY samples="+
                        counts[0]+" foreign="+counts[1]+
                        " restore="+restored+
                        " input_interception=false click_delivery=false");
                    if(!restored || counts[1]!=0)
                        System.err.println("R240_REATTACH_NOT_CERTIFIED");
                    System.out.flush();
                }
            }
        },"slinkywinky-r240-original-input-restore");
        supervisor.setDaemon(true);
        try {supervisor.start();}
        catch(Throwable failure){
            stopAndDrain();
            final boolean clean=restore(inst,game);
            InstrumentationProbe.finishRenderDiagnostic(clean);
            throw new IllegalStateException("R240 supervisor unavailable",failure);
        }
    }
}
