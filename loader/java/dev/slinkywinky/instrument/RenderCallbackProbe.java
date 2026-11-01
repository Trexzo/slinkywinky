package dev.slinkywinky.instrument;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R65 renderer callback proof restricted to generated, disposable test bytecode.
 *
 * NOT a Minecraft render hook: no window/GL/JNI operations, no game actions and
 * no class retransform. The production agent never invokes weaveFixture().
 */
public final class RenderCallbackProbe {
    public static final String FIXTURE = "dev/slinkywinky/attach/R65Fixture";
    private static final String TARGET = "a";
    private static final String DESCRIPTOR = "(FJ)V";
    private static final AtomicInteger samples = new AtomicInteger();
    private static final AtomicBoolean armed = new AtomicBoolean();
    private static volatile Thread owner;
    private static volatile boolean violation;
    private static volatile long currentEpoch;
    private static volatile long lastEpoch;
    private static volatile int maxSamples;

    /** No Minecraft classes or reflection; caller must explicitly own fixture thread. */
    public static synchronized boolean armFixture(long epoch, int limit) {
        if (armed.get() || currentEpoch != 0 || epoch <= lastEpoch ||
            epoch <= 0 || limit < 1 || limit > 64) return false;
        owner = Thread.currentThread();
        samples.set(0);
        violation = false;
        currentEpoch = epoch;
        maxSamples = limit;
        armed.set(true);
        return true;
    }

    /** Signature and dispatch shape intended for a *future* verified renderer. */
    public static void onRender(float partialTicks, long frameNanos) {
        if (!armed.get()) return;
        if (Thread.currentThread() != owner ||
            Float.isNaN(partialTicks) || Float.isInfinite(partialTicks) ||
            partialTicks < 0f || partialTicks > 1f || frameNanos < 0L) {
            violation = true;
            armed.set(false); // fail closed without ever touching game/GL state
            return;
        }
        int observed = samples.incrementAndGet(); // only the owner thread writes
        if (observed >= maxSamples) armed.set(false);
    }

    public static final class Summary {
        public final long epoch;
        public final int accepted;
        public final boolean invalid;
        private Summary(long e, int n, boolean bad) {
            epoch=e; accepted=n; invalid=bad;
        }
    }

    /** Requires original owner thread, even after a cap or validation failure. */
    public static synchronized Summary closeFixture(long epoch) {
        if (currentEpoch == 0 || currentEpoch != epoch ||
            Thread.currentThread() != owner)
            throw new IllegalStateException("Renderer callback session owner mismatch");
        Summary summary = new Summary(currentEpoch, samples.get(), violation);
        armed.set(false);
        owner=null;
        currentEpoch=0;
        lastEpoch=epoch;
        maxSamples=0;
        return summary;
    }

    /**
     * Pure bytecode transformation, fail-closed and usable on ONE test class
     * only. No Instrumentation.retransformClasses and no production class name.
     * The caller retains raw unmodified bytes for its independent test loader.
     */
    public static byte[] weaveFixture(byte[] original) {
        if (original == null || original.length == 0 || original.length > 65536)
            throw new IllegalArgumentException("Unexpected fixture class length");
        final ClassReader reader = new ClassReader(original);
        if (!FIXTURE.equals(reader.getClassName()))
            throw new IllegalArgumentException("Only generated R65 fixture accepted");
        final ClassWriter writer = new ClassWriter(reader, ClassWriter.COMPUTE_MAXS);
        final AtomicInteger accepted = new AtomicInteger();
        final AtomicInteger competing = new AtomicInteger();
        ClassVisitor visitor = new ClassVisitor(Opcodes.ASM5, writer) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String descriptor,String signature,String[] exceptions) {
                MethodVisitor delegate=super.visitMethod(access,name,
                    descriptor,signature,exceptions);
                if (!TARGET.equals(name)) return delegate;
                competing.incrementAndGet();
                if (!DESCRIPTOR.equals(descriptor) ||
                    (access & Opcodes.ACC_PUBLIC)==0 ||
                    (access & (Opcodes.ACC_STATIC|Opcodes.ACC_ABSTRACT|
                               Opcodes.ACC_NATIVE))!=0)
                    return delegate;
                accepted.incrementAndGet();
                return new MethodVisitor(Opcodes.ASM5,delegate) {
                    @Override public void visitCode() {
                        super.visitCode();
                        super.visitVarInsn(Opcodes.FLOAD,1);
                        super.visitVarInsn(Opcodes.LLOAD,2);
                        super.visitMethodInsn(Opcodes.INVOKESTATIC,
                            "dev/slinkywinky/instrument/RenderCallbackProbe",
                            "onRender","(FJ)V",false);
                    }
                };
            }
        };
        reader.accept(visitor,0);
        if (accepted.get()!=1 || competing.get()!=1)
            throw new IllegalArgumentException("Fixture render mapping not unique");
        return writer.toByteArray();
    }

    private RenderCallbackProbe() {}
}
