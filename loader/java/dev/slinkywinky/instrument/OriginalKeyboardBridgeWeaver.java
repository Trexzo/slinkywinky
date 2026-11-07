package dev.slinkywinky.instrument;

import java.security.MessageDigest;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R111: inert, SHA-pinned ORIGINAL vanilla 1.8.9 keyboard callsite weaver.
 *
 * No agent mode invokes this class. No retransform, hooks, input acquisition,
 * or game process changes occur in R111. In particular, substitution of
 * Keyboard.next alone does NOT suppress vanilla pre-queue keybind polling.
 */
public final class OriginalKeyboardBridgeWeaver implements Opcodes {
    public static final String ORIGINAL_SHA256 =
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82";
    public static final String OWNER="dev/slinkywinky/instrument/KeyboardQueueOwner";
    private static final String KEYBOARD="org/lwjgl/input/Keyboard";
    private OriginalKeyboardBridgeWeaver(){}

    private static void require(boolean valid,String message) {
        if(!valid)throw new IllegalArgumentException(message);
    }
    private static String digest(byte[] raw) {
        try {
            byte[] hash=MessageDigest.getInstance("SHA-256").digest(raw);
            StringBuilder value=new StringBuilder(64);
            for(byte b:hash)value.append(String.format("%02x",b&0xff));
            return value.toString();
        }catch(Exception problem){throw new IllegalStateException(problem);}
    }
    private static final class Shape {
        int exactMethods,queueCalls,getKeys,getStates,getChars;
        int beforeKeys,beforePresses,afterKeys,afterPresses,guiCalls;
    }

    private static Shape inspect(byte[] bytes,final boolean bridged) {
        final Shape found=new Shape();
        ClassReader reader=new ClassReader(bytes);
        require("ave".equals(reader.getClassName()),"Only original ave accepted");
        reader.accept(new ClassVisitor(ASM5) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String signature,String[] exceptions) {
                final boolean target="s".equals(name) && "()V".equals(desc);
                if(target) {
                    require((access&ACC_PUBLIC)!=0 &&
                            (access&(ACC_STATIC|ACC_ABSTRACT|ACC_NATIVE))==0,
                            "Unexpected original input method access flags");
                    found.exactMethods++;
                }
                return new MethodVisitor(ASM5) {
                    private boolean insideQueue;
                    @Override public void visitMethodInsn(int op,String owner,
                            String method,String descriptor,boolean iface) {
                        if(op==INVOKESTATIC && "next".equals(method) &&
                           "()Z".equals(descriptor) &&
                           (KEYBOARD.equals(owner)||OWNER.equals(owner))) {
                            found.queueCalls++;
                            require(target,"Keyboard loop moved to another method");
                            require(!insideQueue,"Multiple keyboard queues");
                            require((bridged?OWNER:KEYBOARD).equals(owner) && !iface,
                                    "Unexpected keyboard owner");
                            insideQueue=true;
                        }
                        if(!target)return;
                        if(op==INVOKESTATIC && KEYBOARD.equals(owner)) {
                            if("getEventKey".equals(method) && "()I".equals(descriptor))
                                found.getKeys++;
                            if("getEventKeyState".equals(method) && "()Z".equals(descriptor))
                                found.getStates++;
                            if("getEventCharacter".equals(method) && "()C".equals(descriptor))
                                found.getChars++;
                        }
                        if(op==INVOKESTATIC && "avb".equals(owner)) {
                            if("a".equals(method)&&"(IZ)V".equals(descriptor)) {
                                if(insideQueue)found.afterKeys++;else found.beforeKeys++;
                            }
                            if("a".equals(method)&&"(I)V".equals(descriptor)) {
                                if(insideQueue)found.afterPresses++;else found.beforePresses++;
                            }
                        }
                        if(insideQueue && "axu".equals(owner))found.guiCalls++;
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        require(found.exactMethods==1 && found.queueCalls==1 &&
                found.getKeys==2 && found.getStates==3 &&
                found.getChars==1 && found.beforeKeys==1 &&
                found.beforePresses==1 && found.afterKeys==1 &&
                found.afterPresses==1 && found.guiCalls>=1,
                "Vanilla pre/post keybinding and GUI shape changed");
        return found;
    }

    /**
     * Transforms only the exact source SHA. Returned class bytes remain in
     * memory under caller control and must not be installed by this method.
     */
    public static byte[] weavePinnedOriginal(byte[] original) {
        require(original!=null && original.length>32 && original.length<=1000000,
                "Missing or unexpected original keyboard class");
        require(ORIGINAL_SHA256.equals(digest(original)),
                "Original Minecraft 1.8.9 ave.class SHA-256 mismatch");
        inspect(original,false);
        byte[] updated=rewriteOneCall(original,"s","()V");
        inspect(updated,true);
        require(ORIGINAL_SHA256.equals(digest(original)),
                "R111 transformed immutable baseline input");
        return updated;
    }

    /**
     * Tiny, generated test class only; exercises the SAME rewrite function
     * in a disposable JVM, not the source-pinned original Minecraft class.
     */
    public static byte[] weaveGeneratedFixtureForTest(byte[] input) {
        require(input!=null && input.length>32 && input.length<65536,
                "Invalid generated R111 fixture");
        final ClassReader reader=new ClassReader(input);
        require("dev/slinkywinky/attach/R112KeyboardTarget".equals(
                reader.getClassName()),"Non-test class denied");
        return rewriteOneCall(input,"run","()V");
    }

    private static byte[] rewriteOneCall(byte[] input,
            final String methodName,final String descriptor) {
        final int[] seen={0};
        final int[] replaced={0};
        final int[] methods={0};
        ClassReader reader=new ClassReader(input);
        ClassWriter writer=new ClassWriter(reader,ClassWriter.COMPUTE_MAXS);
        reader.accept(new ClassVisitor(ASM5,writer) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] exceptions) {
                MethodVisitor delegate=super.visitMethod(access,name,desc,sig,exceptions);
                final boolean target=methodName.equals(name) && descriptor.equals(desc);
                if(target) {
                    require((access&ACC_PUBLIC)!=0 &&
                            (access&(ACC_ABSTRACT|ACC_NATIVE))==0,
                            "Unsupported source input-owner method");
                    methods[0]++;
                }
                return new MethodVisitor(ASM5,delegate) {
                    @Override public void visitMethodInsn(int op,String source,
                            String called,String sign,boolean itf) {
                        if(op==INVOKESTATIC && KEYBOARD.equals(source) &&
                           "next".equals(called) && "()Z".equals(sign) && !itf) {
                            seen[0]++;
                            require(target,"Unexpected additional LWJGL queue path");
                            replaced[0]++;
                            super.visitMethodInsn(op,OWNER,called,sign,false);
                        } else super.visitMethodInsn(op,source,called,sign,itf);
                    }
                };
            }
        },0);
        require(methods[0]==1 && seen[0]==1 && replaced[0]==1,
                "R111 refused ambiguous keyboard owner callsite");
        return writer.toByteArray();
    }

    /** Proof of exact original bytes only; not a live restoration receipt. */
    public static boolean exactOriginal(byte[] raw) {
        return raw!=null && raw.length>32 && raw.length<=1000000 &&
               ORIGINAL_SHA256.equals(digest(raw));
    }
}
