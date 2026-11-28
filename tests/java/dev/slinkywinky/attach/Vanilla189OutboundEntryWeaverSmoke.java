package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.Vanilla189OutboundEntryWeaver;
import java.util.concurrent.atomic.AtomicInteger;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** Synthetic ASM5 original ek.a(Lff;)V entry, no Mojang or packet IO. */
public final class Vanilla189OutboundEntryWeaverSmoke {
    static void yes(boolean v,String why){if(!v)throw new AssertionError(why);}
    static byte[] synthetic(){
        ClassWriter w=new ClassWriter(0);
        w.visit(Opcodes.V1_8,Opcodes.ACC_PUBLIC,"ek",null,"java/lang/Object",null);
        for(String sig:new String[]{"(Lff;)V","(Ljava/lang/Object;)V"}){
            MethodVisitor m=w.visitMethod(Opcodes.ACC_PUBLIC,"a",sig,null,null);
            m.visitCode();m.visitInsn(Opcodes.RETURN);
            m.visitMaxs(0,2);m.visitEnd();
        }
        MethodVisitor another=w.visitMethod(Opcodes.ACC_PUBLIC,"b","(Lff;)V",null,null);
        another.visitCode();another.visitInsn(Opcodes.RETURN);
        another.visitMaxs(0,2);another.visitEnd();
        w.visitEnd();return w.toByteArray();
    }
    public static void main(String[] args){
        ClassReader original=new ClassReader(synthetic());
        ClassWriter output=new ClassWriter(original,ClassWriter.COMPUTE_MAXS);
        AtomicInteger found=new AtomicInteger(),entries=new AtomicInteger();
        original.accept(new ClassVisitor(Opcodes.ASM5,output){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String descriptor,String signature,String[] exceptions){
                MethodVisitor down=super.visitMethod(access,name,descriptor,
                    signature,exceptions);
                if(Vanilla189OutboundEntryWeaver.matches(name,descriptor,access)){
                    found.incrementAndGet();
                    return Vanilla189OutboundEntryWeaver.wrap(down,entries);
                }
                return down;
            }
        },0);
        yes(found.get()==1&&entries.get()==1,
            "Original exact NetworkManager method matched once");
        AtomicInteger invokes=new AtomicInteger(),other=new AtomicInteger(),
                      loads=new AtomicInteger(),returns=new AtomicInteger();
        new ClassReader(output.toByteArray()).accept(new ClassVisitor(Opcodes.ASM5){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String signature,String[] exceptions){
                final boolean originalSend=name.equals("a")&&desc.equals("(Lff;)V");
                return new MethodVisitor(Opcodes.ASM5){
                    private int preceding;
                    @Override public void visitVarInsn(int code,int var){
                        if(originalSend&&code==Opcodes.ALOAD&&var==preceding){
                            preceding++;loads.incrementAndGet();
                        }
                    }
                    @Override public void visitMethodInsn(int code,String owner,
                             String method,String descriptor,boolean itf){
                        if(owner.equals("dev/slinkywinky/instrument/InstrumentationProbe")&&
                           method.equals("onOriginalOutboundPacket")){
                            if(originalSend&&code==Opcodes.INVOKESTATIC&&
                               descriptor.equals("(Ljava/lang/Object;Ljava/lang/Object;)V")&&
                               preceding==2)invokes.incrementAndGet();
                            else other.incrementAndGet();
                        }
                    }
                    @Override public void visitInsn(int code){
                        if(originalSend&&code==Opcodes.RETURN)returns.incrementAndGet();
                    }
                };
            }
        },0);
        yes(invokes.get()==1&&other.get()==0&&loads.get()==2&&returns.get()==1,
            "One no-result observer before original network send, one original RETURN");
        yes(!Vanilla189OutboundEntryWeaver.matches("a","(Lff;)V",
            Opcodes.ACC_PUBLIC|Opcodes.ACC_STATIC),"Static send rejected");
        yes(!Vanilla189OutboundEntryWeaver.matches("a","(Ljava/lang/Object;)V",
            Opcodes.ACC_PUBLIC),"Alternate original send overload untouched");
        yes(!Vanilla189OutboundEntryWeaver.matches("b","(Lff;)V",
            Opcodes.ACC_PUBLIC),"Unrelated packet method untouched");
        System.out.println("R365_EXACT_ORIGINAL_EK_PACKET_ENTRY_ONE_OBSERVER_PASS");
    }
}
