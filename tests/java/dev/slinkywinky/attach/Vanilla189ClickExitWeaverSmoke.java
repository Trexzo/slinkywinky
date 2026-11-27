package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.Vanilla189ClickExitWeaver;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.util.concurrent.atomic.AtomicInteger;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** Source-free ASM5 bytecode-test for exactly two original click exits. */
public final class Vanilla189ClickExitWeaverSmoke {
    static void yes(boolean value,String what){
        if(!value)throw new AssertionError(what);
    }
    static byte[] read(String path)throws Exception{
        InputStream resource=Vanilla189ClickExitWeaverSmoke.class.getClassLoader()
            .getResourceAsStream(path);
        if(resource==null)throw new AssertionError("Synthetic class resource missing");
        try(InputStream in=resource;ByteArrayOutputStream out=new ByteArrayOutputStream()){
            byte[] buf=new byte[4096];int n;
            while((n=in.read(buf))>=0)out.write(buf,0,n);
            return out.toByteArray();
        }
    }
    public static void main(String[] args)throws Exception{
        final String path="dev/slinkywinky/attach/R362SyntheticClickTarget.class";
        ClassReader original=new ClassReader(read(path));
        ClassWriter writer=new ClassWriter(original,ClassWriter.COMPUTE_MAXS);
        AtomicInteger matched=new AtomicInteger(),returns=new AtomicInteger();
        original.accept(new ClassVisitor(Opcodes.ASM5,writer){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String descriptor,String signature,String[] exceptions){
                MethodVisitor down=super.visitMethod(access,name,descriptor,
                    signature,exceptions);
                if(Vanilla189ClickExitWeaver.matches(name,descriptor,access)){
                    matched.incrementAndGet();
                    return Vanilla189ClickExitWeaver.wrap(down,returns);
                }
                return down;
            }
        },0);
        yes(matched.get()==1&&returns.get()==2,
            "Two return sites, exactly one instance clickMouse method");
        AtomicInteger callback= new AtomicInteger(),other= new AtomicInteger();
        new ClassReader(writer.toByteArray()).accept(new ClassVisitor(Opcodes.ASM5){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String descriptor,String signature,String[] exceptions){
                return new MethodVisitor(Opcodes.ASM5){
                    @Override public void visitMethodInsn(int opcode,String owner,
                            String method,String desc,boolean itf){
                        if(owner.equals("dev/slinkywinky/instrument/InstrumentationProbe") &&
                           method.equals("onOriginalClickReturn")){
                            if(name.equals("aw")&&desc.equals("()V")&&
                               opcode==Opcodes.INVOKESTATIC)callback.incrementAndGet();
                            else other.incrementAndGet();
                        }
                    }
                };
            }
        },0);
        yes(callback.get()==2&&other.get()==0,
            "Inserted callback ONLY before each normal click return");
        yes(!Vanilla189ClickExitWeaver.matches("ax","()V",Opcodes.ACC_PUBLIC),
            "Wrong original method rejected");
        yes(!Vanilla189ClickExitWeaver.matches("aw","(I)V",Opcodes.ACC_PUBLIC),
            "Wrong original signature rejected");
        yes(!Vanilla189ClickExitWeaver.matches("aw","()V",
            Opcodes.ACC_PUBLIC|Opcodes.ACC_STATIC),
            "Static click method rejected");
        System.out.println("R362_EXACT_VANILLA189_CLICK_EXIT_ASM_CALLSITES_PASS");
    }
}
