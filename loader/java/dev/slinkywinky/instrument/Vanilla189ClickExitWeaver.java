package dev.slinkywinky.instrument;

import java.util.concurrent.atomic.AtomicInteger;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R362: exact original vanilla 1.8.9 clickMouse()V NORMAL RETURN observer.
 * Only caller-provided SHA-pinned ave.class can be transformed in production.
 * No interception, replacement, cancellation or packet change.
 */
public final class Vanilla189ClickExitWeaver {
    public static final String ORIGINAL_NAME="aw",ORIGINAL_DESCRIPTOR="()V";
    private static final String CALLBACK_OWNER=
        "dev/slinkywinky/instrument/InstrumentationProbe";
    private static final String CALLBACK_NAME="onOriginalClickReturn";

    private Vanilla189ClickExitWeaver(){}
    public static boolean matches(String name,String descriptor,int access){
        return ORIGINAL_NAME.equals(name)&&
            ORIGINAL_DESCRIPTOR.equals(descriptor)&&
            (access&(Opcodes.ACC_STATIC|Opcodes.ACC_ABSTRACT|Opcodes.ACC_NATIVE))==0;
    }
    public static MethodVisitor wrap(MethodVisitor original,AtomicInteger returns){
        if(original==null||returns==null)
            throw new IllegalArgumentException("Invalid R362 click weaver");
        return new MethodVisitor(Opcodes.ASM5,original){
            @Override public void visitInsn(int opcode){
                if(opcode==Opcodes.RETURN){
                    returns.incrementAndGet();
                    super.visitMethodInsn(Opcodes.INVOKESTATIC,
                        CALLBACK_OWNER,CALLBACK_NAME,"()V",false);
                }
                super.visitInsn(opcode);
            }
        };
    }
}
