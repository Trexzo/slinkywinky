package dev.slinkywinky.instrument;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.MessageDigest;
import java.security.ProtectionDomain;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * R241-R243: disposable Java 8 cross-process Attach observer agent.
 * NEVER packaged in the production instrumentation JAR.
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

    /** All R243 simulated exceptions are explicitly limited to a new JVM
     * with *both* opt-in variables. The normal R241/R242 paths return "".
     */
    private static String faultMode(){
        if(!"1".equals(System.getenv("SLINKYWINKY_R243_DISPOSABLE_NEGATIVE")))
            return "";
        final String value=System.getenv("SLINKYWINKY_R243_FAULT_MODE");
        if("install".equals(value)||"remove".equals(value)||
           "restore".equals(value))return value;
        return "";
    }
    /**
     * Safe only after the weaving transformer has been positively removed.
     * A false return is NOT a certified restoration and keeps admission locked.
     */
    private static boolean restoreBaseline(final Instrumentation inst,
            final Class<?> target,final byte[] expected,
            final boolean injectFirstCallFailure) {
        if(inst==null || target==null || expected==null)return false;
        final AtomicInteger observed=new AtomicInteger();
        final AtomicBoolean same=new AtomicBoolean();
        ClassFileTransformer verifier=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader,String name,
                    Class<?> type,ProtectionDomain domain,byte[] raw){
                if(type!=target)return null;
                observed.incrementAndGet();
                same.set(Arrays.equals(expected,digest(raw)));
                return null;
            }
        };
        boolean registered=false,removed=false;
        try {
            inst.addTransformer(verifier,true);
            registered=true;
            try {
                // R243: deterministic exception BEFORE the first API call.
                // Never inject faults into any production process.
                if(injectFirstCallFailure)
                    throw new IllegalStateException(
                        "R243 simulated retransformClasses refusal");
                inst.retransformClasses(target);
            } catch (Throwable failure) {
                if(!injectFirstCallFailure)throw failure;
                System.out.println("R243_FIRST_RESTORE_CALL_REFUSED");
                System.out.flush();
                // A second, real retransform is only allowed because the
                // weaving transformer was independently confirmed removed.
                inst.retransformClasses(target);
            }
        } catch(Throwable failure) {
            System.err.println("R243_RESTORE_UNCERTAIN "+
                failure.getClass().getSimpleName());
        } finally {
            if(registered){
                try {removed=inst.removeTransformer(verifier);}
                catch(Throwable failure){removed=false;}
            }
        }
        return registered && removed && observed.get()==1 && same.get();
    }

    public static void agentmain(String options,final Instrumentation inst)
        throws Exception {
        if(!"R241_DISPOSABLE_INPUT_ONLY".equals(options) ||
           !"1".equals(System.getenv("SLINKYWINKY_R241_DISPOSABLE_FIXTURE"))) {
            if("R242_BAD_OPTION".equals(options) &&
               "1".equals(System.getenv("SLINKYWINKY_R242_DISPOSABLE_NEGATIVE"))) {
                System.out.println("R242_INVALID_AGENT_OPTIONS_REFUSED");
                System.out.flush();
            }
            throw new IllegalArgumentException("R241 disposable JVM opt-in required");
        }
        if(!inst.isRetransformClassesSupported())
            throw new IllegalStateException("R241 retransform unsupported");
        if(!admitted.compareAndSet(false,true)) {
            if("1".equals(System.getenv("SLINKYWINKY_R242_DISPOSABLE_NEGATIVE"))) {
                System.out.println("R242_DUPLICATE_ATTACH_REFUSED");
                System.out.flush();
            }
            if(!faultMode().isEmpty()) {
                System.out.println("R243_REATTACH_AFTER_FAULT_DENIED");
                System.out.flush();
            }
            throw new IllegalStateException("R241 duplicate attach refused");
        }

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
        final String fault=faultMode();

        // R243 injects install failure before the first registration.
        // If a real registration fails unpredictably, retain the closed gate.
        if("install".equals(fault)) {
            System.out.println("R243_TRANSFORMER_INSTALL_REFUSED_BEFORE_MUTATION");
            System.out.flush();
            throw new IllegalStateException("R243 injected install refusal");
        }
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

        boolean removed=false;
        Throwable weaveFailure=null;
        Throwable removeFailure=null;
        // Do not install/activate observation until all exact-source checks
        // and the confirmed transformer-removal boundary succeed.
        inst.addTransformer(weaver,true);
        try {
            inst.retransformClasses(target);
        } catch(Throwable failure) {
            weaveFailure=failure;
        } finally {
            try {
                if("remove".equals(fault))
                    throw new IllegalStateException(
                        "R243 simulated first removeTransformer refusal");
                removed=inst.removeTransformer(weaver);
            } catch(Throwable firstFailure) {
                removeFailure=firstFailure;
                // Retrying an *uncertain* remove is conservative: only
                // a positive second return can authorize source restoration.
                try {removed=inst.removeTransformer(weaver);}
                catch(Throwable secondFailure){removed=false;}
            }
        }
        if(!removed) {
            System.err.println("R243_TRANSFORMER_REMOVAL_UNCERTAIN_GATE_LOCKED");
            throw new IllegalStateException("R241 unverified weaver removal");
        }

        if(weaveFailure!=null || removeFailure!=null ||
           transformed.get()!=1 || baselineHash==null) {
            // The input observer has NOT started; restore only after known
            // removal, and never allow reattach on this discarded fixture.
            final boolean restored=restoreBaseline(inst,target,baselineHash,false);
            if(restored && "remove".equals(fault)) {
                System.out.println("R243_TRANSFORMER_REMOVE_RETRY_AND_RESTORE_SHA256_PASS");
                System.out.flush();
            }
            if(!restored)
                System.err.println("R243_FAILED_WEAVE_RESTORATION_NOT_CERTIFIED");
            throw new IllegalStateException(
                "R241 observer install aborted restored="+restored);
        }

        exactTarget=target;
        installed=inst;
        OriginalInputFrameDiagnostic.fixtureBegin();
        ready=true;
        System.out.println("R241_REAL_JAVA8_INPUT_RETRANSFORM_PASS");
        System.out.flush();
    }

    /** Called by the disposable JVM main thread after eight source ticks. */
    public static synchronized void restoreOnOwner() throws Exception {
        if(!ready || !"1".equals(System.getenv("SLINKYWINKY_R241_DISPOSABLE_FIXTURE")))
            throw new IllegalStateException("R241 no active disposable observer");
        ready=false;
        final int[] counts=OriginalInputFrameDiagnostic.fixtureEnd();
        final boolean countsValid=(counts[0]==8 && counts[1]==0);
        if(countsValid)
            System.out.println("R241_OWNER_INPUT_ENTRY_COUNT_PASS count="+counts[0]);
        final Instrumentation inst=installed;
        final Class<?> target=exactTarget;
        final byte[] expected=baselineHash;
        if(inst==null || target==null || expected==null)
            throw new IllegalStateException("R241 missing baseline receipt");

        final boolean injectFailure="restore".equals(faultMode());
        final boolean restored=restoreBaseline(inst,target,expected,injectFailure);
        if(!restored)
            throw new IllegalStateException(
                "R241 baseline bytecode restoration failed");
        exactTarget=null;installed=null;baselineHash=null;
        if(injectFailure) {
            System.out.println("R243_RESTORE_RETRY_SOURCE_SHA256_PASS");
            System.out.flush();
            // A recovered fault never silently certifies the first attempt.
            throw new IllegalStateException("R243 recovery after restore fault");
        }
        System.out.println("R241_REAL_JAVA8_INPUT_BYTECODE_RESTORE_SHA256_PASS");
        System.out.flush();
        if(!countsValid) {
            System.out.println("R242_RESTORED_BEFORE_INVALID_COUNT_REJECTION entries="+
                counts[0]+" foreign="+counts[1]);
            System.out.flush();
            throw new IllegalStateException("R241 owner entry count mismatch");
        }
    }
    private R241InputAttachAgent(){}
}
