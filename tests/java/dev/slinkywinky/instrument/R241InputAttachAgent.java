package dev.slinkywinky.instrument;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.MessageDigest;
import java.security.ProtectionDomain;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * R241: test-only cross-process Attach agent for R240's actual ASM weaver.
 * A disposable fixture JVM and explicit environment opt-in are mandatory.
 * This class is NOT part of the production instrumentation JAR.
 */
public final class R241InputAttachAgent {
    private static final String TARGET =
        "dev.slinkywinky.instrument.R240InputFixture";
    private static final AtomicBoolean admitted=new AtomicBoolean();
    private static volatile boolean ready;
    private static volatile Instrumentation installed;
    private static volatile Class<?> exactTarget;
    private static volatile byte[] baselineHash;

    private static byte[] digest(byte[] data) {
        try {return MessageDigest.getInstance("SHA-256").digest(data);}
        catch(Exception problem){throw new IllegalStateException(problem);}
    }
    public static boolean isReady(){return ready;}

    public static void agentmain(String options,final Instrumentation inst)
        throws Exception {
        if(!"R241_DISPOSABLE_INPUT_ONLY".equals(options) ||
           !"1".equals(System.getenv("SLINKYWINKY_R241_DISPOSABLE_FIXTURE")))
            throw new IllegalArgumentException("R241 disposable JVM opt-in required");
        if(!inst.isRetransformClassesSupported())
            throw new IllegalStateException("R241 retransform unsupported");
        if(!admitted.compareAndSet(false,true))
            throw new IllegalStateException("R241 duplicate attach refused");
        Class<?> found=null;
        for(Class<?> type:inst.getAllLoadedClasses()) {
            if(TARGET.equals(type.getName())){
                if(found!=null)throw new IllegalStateException("R241 duplicate class");
                found=type;
            }
        }
        if(found==null || !inst.isModifiableClass(found) ||
           found.getClassLoader()!=ClassLoader.getSystemClassLoader())
            throw new IllegalStateException("R241 exact disposable target absent");
        final Class<?> target=found;
        final AtomicInteger transformed=new AtomicInteger();
        ClassFileTransformer weaver=new ClassFileTransformer(){
            @Override public byte[] transform(ClassLoader loader,String name,
                Class<?> type,ProtectionDomain domain,byte[] raw){
                if(type!=target)return null;
                baselineHash=digest(raw);
                transformed.incrementAndGet();
                return OriginalInputFrameDiagnostic.weaveFixture(raw);
            }
        };
        inst.addTransformer(weaver,true);
        boolean removed=false;
        try {inst.retransformClasses(target);}
        finally {removed=inst.removeTransformer(weaver);}
        if(!removed || transformed.get()!=1 || baselineHash==null)
            throw new IllegalStateException("R241 unverified input weave");
        exactTarget=target;
        installed=inst;
        OriginalInputFrameDiagnostic.fixtureBegin();
        ready=true;
        System.out.println("R241_REAL_JAVA8_INPUT_RETRANSFORM_PASS");
        System.out.flush();
    }

    /** Called by the original disposable main thread after eight source ticks. */
    public static synchronized void restoreOnOwner() throws Exception {
        if(!ready || !"1".equals(System.getenv("SLINKYWINKY_R241_DISPOSABLE_FIXTURE")))
            throw new IllegalStateException("R241 no active disposable observer");
        ready=false;
        final int[] counts=OriginalInputFrameDiagnostic.fixtureEnd();
        if(counts[0]!=8 || counts[1]!=0)
            throw new IllegalStateException("R241 owner entry count mismatch");
        System.out.println("R241_OWNER_INPUT_ENTRY_COUNT_PASS count="+counts[0]);
        final Instrumentation inst=installed;
        final Class<?> target=exactTarget;
        final byte[] expected=baselineHash;
        if(inst==null || target==null || expected==null)
            throw new IllegalStateException("R241 missing baseline receipt");
        final AtomicInteger observed=new AtomicInteger();
        final AtomicBoolean same=new AtomicBoolean();
        ClassFileTransformer verifier=new ClassFileTransformer(){
            @Override public byte[] transform(ClassLoader loader,String name,
                Class<?> type,ProtectionDomain domain,byte[] raw){
                if(type!=target)return null;
                observed.incrementAndGet();
                same.set(Arrays.equals(expected,digest(raw)));
                return null;
            }
        };
        inst.addTransformer(verifier,true);
        boolean removed=false;
        try {inst.retransformClasses(target);}
        finally {removed=inst.removeTransformer(verifier);}
        if(!removed || observed.get()!=1 || !same.get())
            throw new IllegalStateException("R241 baseline bytecode restoration failed");
        exactTarget=null;installed=null;baselineHash=null;
        System.out.println("R241_REAL_JAVA8_INPUT_BYTECODE_RESTORE_SHA256_PASS");
        System.out.flush();
    }
    private R241InputAttachAgent(){}
}
