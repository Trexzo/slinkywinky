package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RenderCallbackProbe;
import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.MessageDigest;
import java.security.ProtectionDomain;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

/**
 * R66 test-only Java 8 Attach API agent. It cannot target any Minecraft class,
 * does not use JNI or graphics APIs and is never packaged with the user injector.
 */
public final class RenderAttachAgent {
    private static final AtomicBoolean admitted=new AtomicBoolean();
    private static volatile Instrumentation instrument;
    private static volatile boolean ready;
    private static volatile byte[] baseline;
    private static volatile Class<?> exactTarget;
    private static final String TARGET="dev.slinkywinky.attach.R65Fixture";

    private static byte[] hash(byte[] value) {
        try {return MessageDigest.getInstance("SHA-256").digest(value);}
        catch(Exception e){throw new IllegalStateException(e);}
    }
    public static boolean hookReady() {return ready;}

    public static void agentmain(String options,final Instrumentation inst) throws Exception {
        if(!"R66_FIXTURE_ONLY".equals(options) ||
           !"1".equals(System.getenv("SLINKYWINKY_R66_DISPOSABLE_FIXTURE")))
            throw new IllegalArgumentException("R66 fixture requires exact opt-in");
        if(!inst.isRetransformClassesSupported())
            throw new IllegalStateException("Class retransformation unsupported");
        if(!admitted.compareAndSet(false,true))
            throw new IllegalStateException("Duplicate fixture attach refused");
        boolean success=false;
        try {
            Class<?> target=null;
            for(Class<?> type:inst.getAllLoadedClasses()) {
                if(TARGET.equals(type.getName())) {
                    if(target!=null)throw new IllegalStateException("Two fixture classes loaded");
                    target=type;
                }
            }
            if(target==null || !inst.isModifiableClass(target) ||
               target.getClassLoader()!=ClassLoader.getSystemClassLoader())
                throw new IllegalStateException("Exact synthetic classloader admission failed");
            final Class<?> selected=target;
            final AtomicInteger transformed=new AtomicInteger();
            ClassFileTransformer worker=new ClassFileTransformer() {
                @Override public byte[] transform(ClassLoader loader,String name,
                    Class<?> type,ProtectionDomain protection,byte[] bytes) {
                    if(type!=selected)return null;
                    transformed.incrementAndGet();
                    baseline=hash(bytes);
                    return RenderCallbackProbe.weaveFixture(bytes);
                }
            };
            inst.addTransformer(worker,true);
            try {inst.retransformClasses(target);}
            finally {inst.removeTransformer(worker);}
            if(transformed.get()!=1 || baseline==null)
                throw new IllegalStateException("Expected exactly one mapped class");
            exactTarget=target;
            instrument=inst;
            ready=true;
            success=true;
            System.out.println("R66_EXACT_FIXTURE_INSTRUMENTATION_PASS");
            System.out.flush();
        } finally {
            if(!success) {
                ready=false;
                // Fail-closed: an uncertain transformation is not retried.
            }
        }
    }

    /** Called outside a render frame by the same disposable target main thread. */
    public static void restoreOnOwner() throws Exception {
        if(!ready || !admitted.get() ||
           !"1".equals(System.getenv("SLINKYWINKY_R66_DISPOSABLE_FIXTURE")))
            throw new IllegalStateException("No admitted disposable renderer to close");
        ready=false;
        final Class<?> target=exactTarget;
        final Instrumentation inst=instrument;
        final byte[] originalHash=baseline;
        if(target==null || inst==null || originalHash==null)
            throw new IllegalStateException("Missing original class receipt");
        final AtomicInteger observed=new AtomicInteger();
        final AtomicBoolean same=new AtomicBoolean();
        ClassFileTransformer verifier=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader loader,String name,
                Class<?> type,ProtectionDomain protection,byte[] bytes) {
                if(type==target) {
                    observed.incrementAndGet();
                    same.set(Arrays.equals(originalHash,hash(bytes)));
                }
                return null;
            }
        };
        inst.addTransformer(verifier,true);
        try {inst.retransformClasses(target);}
        finally {inst.removeTransformer(verifier);}
        if(observed.get()!=1 || !same.get())
            throw new IllegalStateException("Original class SHA256 restoration failed");
        instrument=null;
        exactTarget=null;
        baseline=null;
        System.out.println("R66_BYTECODE_RESTORE_SHA256_PASS");
        System.out.println("R66_RENDER_OWNER_CLEANUP_PASS");
        System.out.flush();
    }
    private RenderAttachAgent(){}
}
