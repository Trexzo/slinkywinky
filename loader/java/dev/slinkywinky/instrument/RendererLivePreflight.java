package dev.slinkywinky.instrument;

import java.lang.instrument.ClassFileTransformer;
import java.lang.instrument.Instrumentation;
import java.security.MessageDigest;
import java.security.ProtectionDomain;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicInteger;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R73 opt-in, passive loaded-class admission for ORIGINAL vanilla 1.8.9.
 * No callback weaving, native calls, OpenGL, or game state changes.
 *
 * Transformations return null. Only a fingerprint of loaded classfile bytes
 * is used to decide whether a *future* render diagnostic may be considered.
 */
public final class RendererLivePreflight {
    private static final String AVE_SHA =
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82";
    private static final String BFK_SHA =
        "8a4ce93bbf272398ea298d2ed363f75211dd469fcd3b66682a387e4738a419a2";

    /**
     * An inspector removal failure is different from a clean pre-weave SHA
     * refusal: the passive transformer might still be registered on the JVM.
     * Never release the global hook admission gate in this uncertain state.
     */
    public static final class InspectorRemovalUncertain extends IllegalStateException {
        public InspectorRemovalUncertain(String message) {super(message);}
        public InspectorRemovalUncertain(String message,Throwable why) {
            super(message,why);
        }
    }

    private static void require(boolean ok, String message) {
        if(!ok) throw new IllegalStateException(message);
    }

    private static String sha256(byte[] bytes) throws Exception {
        byte[] digest=MessageDigest.getInstance("SHA-256").digest(bytes);
        StringBuilder out=new StringBuilder(64);
        for(byte b:digest) {
            out.append(Character.forDigit((b>>>4)&15,16));
            out.append(Character.forDigit(b&15,16));
        }
        return out.toString();
    }

    /**
     * Structural checks run on generated fixtures in CI and on SHA-pinned
     * actual classes in production preflight. Policy hashes passed here do
     * not affect the entrypoint's fixed original-client hashes.
     */
    public static void checkExactClasses(byte[] ave, byte[] bfk,
                                          String aveExpected, String bfkExpected)
        throws Exception {
        require(ave!=null && bfk!=null && ave.length>0 && bfk.length>0 &&
                ave.length<=1000000 && bfk.length<=1000000,"Invalid classfile lengths");
        final String aveActual=sha256(ave);
        final String bfkActual=sha256(bfk);
        if(!aveActual.equals(aveExpected) || !bfkActual.equals(bfkExpected)) {
            // R90: trace only public bytecode fingerprints and lengths in the
            // failure path; never persist, print or relax modified class bytes.
            throw new IllegalStateException(
                "Loaded original-class SHA-256 mismatch"+
                " ave="+aveActual+" length="+ave.length+
                " bfk="+bfkActual+" length="+bfk.length);
        }
        final ClassReader game,renderer;
        try {
            game=new ClassReader(ave);
            renderer=new ClassReader(bfk);
        } catch(RuntimeException malformed) {
            throw new IllegalStateException("Malformed original renderer classfile",malformed);
        }
        require("ave".equals(game.getClassName()) &&
                "bfk".equals(renderer.getClassName()),"Wrong original obfuscated owners");
        final int[] invocations={0};
        final int[] ownedRendererField={0};
        game.accept(new ClassVisitor(Opcodes.ASM5) {
            @Override public org.objectweb.asm.FieldVisitor visitField(int access,
                    String name,String desc,String sig,Object value) {
                if((access&Opcodes.ACC_STATIC)==0 && "Lbfk;".equals(desc))
                    ownedRendererField[0]++;
                return null;
            }
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] exceptions) {
                if(!"av".equals(name) || (access&Opcodes.ACC_STATIC)!=0)
                    return null;
                return new MethodVisitor(Opcodes.ASM5) {
                    @Override public void visitMethodInsn(int op,String owner,
                            String method,String desc,boolean itf) {
                        if(op==Opcodes.INVOKEVIRTUAL && !itf && "bfk".equals(owner) &&
                           "a".equals(method) && "(FJ)V".equals(desc))
                            invocations[0]++;
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        final int[] methods={0};
        renderer.accept(new ClassVisitor(Opcodes.ASM5) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] exceptions) {
                if("a".equals(name) && "(FJ)V".equals(desc) &&
                   (access&Opcodes.ACC_PUBLIC)!=0 &&
                   (access&(Opcodes.ACC_STATIC|Opcodes.ACC_ABSTRACT|Opcodes.ACC_NATIVE))==0)
                    methods[0]++;
                return null;
            }
        },ClassReader.SKIP_CODE|ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        require(ownedRendererField[0]==1 && invocations[0]==1 && methods[0]==1,
            "Original renderer shape mismatch");
    }

    /** Only this pinned admission is used on a live target. */
    public static void checkPinnedOriginal(byte[] ave, byte[] bfk) throws Exception {
        checkExactClasses(ave,bfk,AVE_SHA,BFK_SHA);
    }

    /** R75: admission returns the exact loaded class only after all R73 checks. */
    public static Class<?> admit(final Instrumentation instrumentation) throws Exception {
        require(instrumentation!=null && instrumentation.isRetransformClassesSupported(),
                "Read-only retransformation unsupported");
        Class<?> game=null,renderer=null;
        for(Class<?> loaded:instrumentation.getAllLoadedClasses()) {
            if("ave".equals(loaded.getName())) {
                require(game==null,"Multiple Minecraft ave classes");
                game=loaded;
            } else if("bfk".equals(loaded.getName())) {
                require(renderer==null,"Multiple Minecraft bfk classes");
                renderer=loaded;
            }
        }
        require(game!=null && renderer!=null &&
                instrumentation.isModifiableClass(game) &&
                instrumentation.isModifiableClass(renderer),
                "Exact original Minecraft renderer classes not loaded/modifiable");
        final ClassLoader loader=game.getClassLoader();
        require(loader!=null && loader==renderer.getClassLoader(),
                "Original renderer and Minecraft must share one non-bootstrap loader");
        require(loader==ClassLoader.getSystemClassLoader() ||
                "net.minecraft.launchwrapper.LaunchClassLoader".equals(
                    loader.getClass().getName()),
                "Unrecognized Minecraft renderer class loader");

        final Class<?> gameClass=game,renderClass=renderer;
        final AtomicInteger gameSeen=new AtomicInteger(),renderSeen=new AtomicInteger();
        final byte[][] captured=new byte[2][];
        final ClassFileTransformer reader=new ClassFileTransformer() {
            @Override public byte[] transform(ClassLoader owner,String name,
                    Class<?> current,ProtectionDomain protection,byte[] raw) {
                if(current==gameClass) {
                    captured[0]=raw==null?null:raw.clone();
                    gameSeen.incrementAndGet();
                } else if(current==renderClass) {
                    captured[1]=raw==null?null:raw.clone();
                    renderSeen.incrementAndGet();
                }
                return null; // Passive observation only. No transformed bytes.
            }
        };
        try {
            instrumentation.addTransformer(reader,true);
        } catch(Throwable installFailure) {
            // The API threw before confirming installation. Do not assume a
            // failed registration left no passive inspector behind.
            throw new InspectorRemovalUncertain(
                "Passive original-renderer inspector registration uncertain",
                installFailure);
        }
        try {
            instrumentation.retransformClasses(gameClass,renderClass);
        } finally {
            // R80: even a read-only transformer must not leak onto the game.
            // A false return or throwable means admission remains locked.
            final boolean removed;
            try {
                removed=instrumentation.removeTransformer(reader);
            } catch(Throwable removalFailure) {
                throw new InspectorRemovalUncertain(
                    "Passive original-renderer inspector removal failed",removalFailure);
            }
            if(!removed)
                throw new InspectorRemovalUncertain(
                    "Passive original-renderer inspector removal unconfirmed");
        }
        require(gameSeen.get()==1 && renderSeen.get()==1,
                "Original classes not observed exactly once");
        checkPinnedOriginal(captured[0],captured[1]);
        return renderClass;
    }

    public static void run(final Instrumentation instrumentation) throws Exception {
        admit(instrumentation);
        // Does not prove a live game render-frame callback or GL context.
        System.out.println("R73_LOADED_ORIGINAL_VANILLA_189_RENDERER_PREFLIGHT_PASS");
        System.out.println("R73_NO_RENDER_HOOK_OR_GRAPHICS_SIDE_EFFECTS");
        System.out.flush();
    }

    private RendererLivePreflight() {}
}
