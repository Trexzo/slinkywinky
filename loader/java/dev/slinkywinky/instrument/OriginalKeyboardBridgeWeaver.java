package dev.slinkywinky.instrument;

import java.security.MessageDigest;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** R111 original Minecraft 1.8.9 SHA-pinned keyboard bridge.
 * NEVER called by the agent in R111. In particular a Keyboard.next()
 * substitution alone does not prevent pre-loop game keybindings.
 */
public final class OriginalKeyboardBridgeWeaver implements Opcodes {
    public static final String ORIGINAL_SHA256=
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82";
    private static final String FROM="org/lwjgl/input/Keyboard";
    private static final String TO="dev/slinkywinky/instrument/KeyboardQueueOwner";
    private OriginalKeyboardBridgeWeaver(){}
    private static void require(boolean ok,String message) {
        if(!ok)throw new IllegalArgumentException(message);
    }
    private static String digest(byte[] raw) {
        try {
            byte[] hash=MessageDigest.getInstance("SHA-256").digest(raw);
            StringBuilder result=new StringBuilder(64);
            for(byte b:hash)result.append(String.format("%02x",b&255));
            return result.toString();
        }catch(Exception impossible){throw new IllegalStateException(impossible);}
    }
    private static final class Counts {
        int ticks,next,key,state,character,preKey,prePress,postKey,postPress,gui;
    }
    private static void validate(byte[] bytes,final boolean routed) {
        final Counts count=new Counts();
        ClassReader reader=new ClassReader(bytes);
        require("ave".equals(reader.getClassName()),"Original input class not found");
        reader.accept(new ClassVisitor(ASM5) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String descriptor,String signature,String[] exceptions) {
                final boolean tick="s".equals(name)&&"()V".equals(descriptor);
                if(tick) {
                    require((access&ACC_PUBLIC)!=0 &&
                            (access&(ACC_STATIC|ACC_ABSTRACT|ACC_NATIVE))==0,
                            "Unexpected input method flags");
                    count.ticks++;
                }
                return new MethodVisitor(ASM5) {
                    private boolean after;
                    @Override public void visitMethodInsn(int opcode,String owner,
                            String method,String desc,boolean itf) {
                        if(opcode==INVOKESTATIC && "next".equals(method) &&
                           "()Z".equals(desc) && (FROM.equals(owner)||TO.equals(owner))) {
                            require(tick && !after && !itf &&
                                    (routed?TO:FROM).equals(owner),
                                    "Ambiguous or misplaced keyboard event queue");
                            count.next++;
                            after=true;
                        }
                        if(!tick)return;
                        if(opcode==INVOKESTATIC && FROM.equals(owner)) {
                            if("getEventKey".equals(method)&&"()I".equals(desc))
                                count.key++;
                            if("getEventKeyState".equals(method)&&"()Z".equals(desc))
                                count.state++;
                            if("getEventCharacter".equals(method)&&"()C".equals(desc))
                                count.character++;
                        }
                        if(opcode==INVOKESTATIC && "avb".equals(owner) &&
                           "a".equals(method)) {
                            if("(IZ)V".equals(desc)) {
                                if(after)count.postKey++;else count.preKey++;
                            }
                            if("(I)V".equals(desc)) {
                                if(after)count.postPress++;else count.prePress++;
                            }
                        }
                        if(after && "axu".equals(owner))count.gui++;
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        require(count.ticks==1 && count.next==1 &&
                count.key==2 && count.state==3 && count.character==1 &&
                count.preKey==1 && count.prePress==1 &&
                count.postKey==1 && count.postPress==1 && count.gui>=1,
                "Original game input dispatch or GUI shape changed");
    }
    /** Memory-only transformation; DOES NOT install an instrumentation hook. */
    public static byte[] weavePinnedOriginal(byte[] raw) {
        require(raw!=null && raw.length>32 && raw.length<=1000000,
                "Missing original 1.8.9 class");
        require(ORIGINAL_SHA256.equals(digest(raw)),
                "Original keyboard owner SHA256 does not match");
        validate(raw,false);
        final int[] replaced={0};
        ClassReader reader=new ClassReader(raw);
        ClassWriter writer=new ClassWriter(reader,ClassWriter.COMPUTE_MAXS);
        reader.accept(new ClassVisitor(ASM5,writer) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String descriptor,String signature,String[] exceptions) {
                MethodVisitor downstream=super.visitMethod(access,name,descriptor,
                                                            signature,exceptions);
                if(!"s".equals(name)||!"()V".equals(descriptor))return downstream;
                return new MethodVisitor(ASM5,downstream) {
                    @Override public void visitMethodInsn(int op,String owner,
                            String method,String desc,boolean itf) {
                        if(op==INVOKESTATIC && FROM.equals(owner) &&
                           "next".equals(method) && "()Z".equals(desc) && !itf) {
                            replaced[0]++;
                            super.visitMethodInsn(op,TO,method,desc,false);
                        }else super.visitMethodInsn(op,owner,method,desc,itf);
                    }
                };
            }
        },0);
        require(replaced[0]==1,"Not exactly one source-pinned queue call");
        byte[] changed=writer.toByteArray();
        validate(changed,true);
        require(ORIGINAL_SHA256.equals(digest(raw)),"Baseline input changed");
        return changed;
    }
    /** On-disk digest only; not independent live class restore verification. */
    public static boolean exactOriginal(byte[] bytes) {
        return bytes!=null && bytes.length>32 && bytes.length<=1000000 &&
               ORIGINAL_SHA256.equals(digest(bytes));
    }
}
