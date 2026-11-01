package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RenderCallbackProbe;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** Synthetic, isolated JVM-only renderer callback regression. No Minecraft. */
public final class RenderCallbackSmoke implements Opcodes {
    private static void check(boolean value,String problem) {
        if(!value) throw new AssertionError(problem);
    }
    private static byte[] fixture(boolean staticRender,boolean wrongName,
                                  boolean overload) {
        String type=RenderCallbackProbe.FIXTURE;
        ClassWriter w=new ClassWriter(0);
        w.visit(V1_8,ACC_PUBLIC,type,null,"java/lang/Object",null);
        w.visitField(ACC_PUBLIC|ACC_STATIC,"work","I",null,null).visitEnd();
        MethodVisitor ctor=w.visitMethod(ACC_PUBLIC,"<init>","()V",null,null);
        ctor.visitCode();
        ctor.visitVarInsn(ALOAD,0);
        ctor.visitMethodInsn(INVOKESPECIAL,"java/lang/Object","<init>","()V",false);
        ctor.visitInsn(RETURN);
        ctor.visitMaxs(1,1);
        ctor.visitEnd();
        MethodVisitor m=w.visitMethod(ACC_PUBLIC|(staticRender?ACC_STATIC:0),
                                      wrongName?"b":"a","(FJ)V",null,null);
        m.visitCode();
        m.visitFieldInsn(GETSTATIC,type,"work","I");
        m.visitInsn(ICONST_1);m.visitInsn(IADD);
        m.visitFieldInsn(PUTSTATIC,type,"work","I");
        m.visitInsn(RETURN);
        m.visitMaxs(2,staticRender?3:4);
        m.visitEnd();
        if(overload) {
            MethodVisitor extra=w.visitMethod(ACC_PUBLIC,"a","(I)V",null,null);
            extra.visitCode();
            extra.visitInsn(RETURN);
            extra.visitMaxs(0,2);
            extra.visitEnd();
        }
        w.visitEnd();
        return w.toByteArray();
    }
    private static final class Isolated extends ClassLoader {
        Isolated() {super(RenderCallbackSmoke.class.getClassLoader());}
        Class<?> define(byte[] bytes) {
            return defineClass(RenderCallbackProbe.FIXTURE.replace('/','.'),
                               bytes,0,bytes.length);
        }
    }
    private static void denied(byte[] bytes) {
        try {
            RenderCallbackProbe.weaveFixture(bytes);
            throw new AssertionError("Non-exact renderer mapped");
        } catch(RuntimeException expected) {
            // ASM may reject truncated classfiles before policy validation.
        }
    }
    private static int work(Class<?> clazz) throws Exception {
        return clazz.getField("work").getInt(null);
    }
    public static void main(String[] args) throws Exception {
        final byte[] original=fixture(false,false,false);
        final byte[] baseline=MessageDigest.getInstance("SHA-256").digest(original);
        byte[] mapped=RenderCallbackProbe.weaveFixture(original);
        check(!Arrays.equals(mapped,original),"No callback in transformed fixture");
        check(Arrays.equals(MessageDigest.getInstance("SHA-256").digest(original),
                             baseline),"Original fixture bytes altered by preflight");
        denied(fixture(true,false,false));
        denied(fixture(false,true,false));
        denied(fixture(false,false,true));
        denied(new byte[]{1,2,3});
        Class<?> target=new Isolated().define(mapped);
        Object instance=target.newInstance();
        Method call=target.getMethod("a",float.class,long.class);
        call.invoke(instance,0.5f,1L);
        check(work(target)==1,"Unarmed callback changed fixture behavior");
        check(!RenderCallbackProbe.armFixture(0,4),"Zero epoch admitted");
        check(!RenderCallbackProbe.armFixture(1,0),"Zero callback cap admitted");
        check(!RenderCallbackProbe.armFixture(1,65),"Oversized callback cap admitted");
        check(RenderCallbackProbe.armFixture(1,3),"Bounded renderer not armed");
        check(!RenderCallbackProbe.armFixture(2,3),"Double renderer arm accepted");
        for(int i=0;i<5;i++)call.invoke(instance,0.5f,i+1L);
        RenderCallbackProbe.Summary first=RenderCallbackProbe.closeFixture(1);
        check(first.epoch==1 && first.accepted==3 && !first.invalid,
              "Rendered callback did not honor independent three-frame cap");
        check(work(target)==6,"Instrumented normal render body not preserved");
        try {RenderCallbackProbe.closeFixture(1);throw new AssertionError("Double close");}
        catch(IllegalStateException expected){}
        check(!RenderCallbackProbe.armFixture(1,2),"Old epoch replay admitted");
        check(RenderCallbackProbe.armFixture(2,12),"Fresh renderer owner denied");
        AtomicBoolean crossThreadRejected=new AtomicBoolean();
        Thread unexpected=new Thread(new Runnable() {
            @Override public void run() {
                try {
                    call.invoke(instance,0.5f,8L);
                    try {RenderCallbackProbe.closeFixture(2);}
                    catch(IllegalStateException expected) {
                        crossThreadRejected.set(true);
                    }
                } catch(Exception failure) {throw new RuntimeException(failure);}
            }
        },"R65-offthread-control");
        unexpected.start();unexpected.join();
        RenderCallbackProbe.Summary cross=RenderCallbackProbe.closeFixture(2);
        check(crossThreadRejected.get() && cross.invalid && cross.accepted==0,
              "Wrong render thread must fail closed without admitted frames");
        check(RenderCallbackProbe.armFixture(3,8),"New frame generation denied");
        call.invoke(instance,Float.NaN,9L);
        RenderCallbackProbe.Summary bad=RenderCallbackProbe.closeFixture(3);
        check(bad.invalid && bad.accepted==0,
              "Invalid partial ticks silently admitted");
        check(RenderCallbackProbe.armFixture(4,8),"Final frame generation denied");
        call.invoke(instance,0.1f,-1L);
        check(RenderCallbackProbe.closeFixture(4).invalid,
              "Negative frame timestamp admitted");
        final Class<?> restored=new Isolated().define(original);
        Object fresh=restored.newInstance();
        Method unmodified=restored.getMethod("a",float.class,long.class);
        unmodified.invoke(fresh,0.25f,2L);
        check(work(restored)==1,"Original fixture method not intact");
        check(Arrays.equals(MessageDigest.getInstance("SHA-256").digest(original),
                             baseline),"Original bytecode fingerprint changed");
        System.out.println("R65_READ_ONLY_RENDER_FIXTURE_PASS");
        System.out.println("R65_RENDER_THREAD_AND_BOUNDS_NEGATIVE_CONTROLS_PASS");
        System.out.println("R65_NOTE=NEW_CLASSLOADER_ORIGINAL_BYTES_NOT_LIVE_RETRANSFORM");
    }
    private RenderCallbackSmoke() {}
}
