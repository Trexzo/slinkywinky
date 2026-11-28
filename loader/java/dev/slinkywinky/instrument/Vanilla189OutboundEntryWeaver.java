package dev.slinkywinky.instrument;

import java.util.concurrent.atomic.AtomicInteger;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R365 exact original NetworkManager ek.a(Lff;)V ENTRY observer.
 * Before the original body executes, it provides the transient NetworkManager
 * and Packet arguments to a no-throw observational callback. No cancellation.
 */
public final class Vanilla189OutboundEntryWeaver {
    public static final String METHOD="a",DESCRIPTOR="(Lff;)V";
    private Vanilla189OutboundEntryWeaver(){}
    public static boolean matches(String name,String descriptor,int access){
        return METHOD.equals(name)&&DESCRIPTOR.equals(descriptor)&&
            (access&(Opcodes.ACC_STATIC|Opcodes.ACC_NATIVE|Opcodes.ACC_ABSTRACT))==0;
    }
    public static MethodVisitor wrap(MethodVisitor original,AtomicInteger entries){
        if(original==null||entries==null)
            throw new IllegalArgumentException("Invalid NetworkManager ASM weaver");
        return new MethodVisitor(Opcodes.ASM5,original){
            @Override public void visitCode(){
                super.visitCode();
                entries.incrementAndGet();
                super.visitVarInsn(Opcodes.ALOAD,0); // original ek NetworkManager
                super.visitVarInsn(Opcodes.ALOAD,1); // transient original ff packet
                super.visitMethodInsn(Opcodes.INVOKESTATIC,
                    "dev/slinkywinky/instrument/InstrumentationProbe",
                    "onOriginalOutboundPacket",
                    "(Ljava/lang/Object;Ljava/lang/Object;)V",false);
            }
        };
    }
}
