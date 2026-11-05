package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RendererLivePreflight;
import java.lang.instrument.Instrumentation;
import java.lang.reflect.InvocationHandler;
import java.lang.reflect.Method;
import java.lang.reflect.Proxy;
import java.security.MessageDigest;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** Generated-classes-only R73 preflight admission and denial regression. */
public final class RendererLivePreflightSmoke implements Opcodes {
    private static void check(boolean valid, String explanation) {
        if(!valid) throw new AssertionError(explanation);
    }

    private static String sha(byte[] value) throws Exception {
        byte[] digest=MessageDigest.getInstance("SHA-256").digest(value);
        StringBuilder out=new StringBuilder();
        for(byte b:digest)out.append(String.format("%02x",b&0xff));
        return out.toString();
    }

    private static byte[] game(boolean ownsRenderer, boolean secondCall) {
        ClassWriter w=new ClassWriter(0);
        w.visit(V1_8,ACC_PUBLIC,"ave",null,"java/lang/Object",null);
        w.visitField(ownsRenderer?ACC_PUBLIC:(ACC_PUBLIC|ACC_STATIC),
            "renderer","Lbfk;",null,null).visitEnd();
        MethodVisitor m=w.visitMethod(ACC_PUBLIC,"av","()V",null,null);
        m.visitCode();
        m.visitVarInsn(ALOAD,0);
        m.visitFieldInsn(GETFIELD,"ave","renderer","Lbfk;");
        m.visitInsn(FCONST_0);
        m.visitInsn(LCONST_0);
        m.visitMethodInsn(INVOKEVIRTUAL,"bfk","a","(FJ)V",false);
        if(secondCall){
            m.visitVarInsn(ALOAD,0);
            m.visitFieldInsn(GETFIELD,"ave","renderer","Lbfk;");
            m.visitInsn(FCONST_0);
            m.visitInsn(LCONST_0);
            m.visitMethodInsn(INVOKEVIRTUAL,"bfk","a","(FJ)V",false);
        }
        m.visitInsn(RETURN);
        m.visitMaxs(4,1);
        m.visitEnd();w.visitEnd();
        return w.toByteArray();
    }

    private static byte[] renderer(String cls, boolean staticMethod) {
        ClassWriter w=new ClassWriter(0);
        w.visit(V1_8,ACC_PUBLIC,cls,null,"java/lang/Object",null);
        MethodVisitor m=w.visitMethod(
            ACC_PUBLIC|(staticMethod?ACC_STATIC:0),"a","(FJ)V",null,null);
        m.visitCode();m.visitInsn(RETURN);
        m.visitMaxs(0,staticMethod?3:4);
        m.visitEnd();w.visitEnd();
        return w.toByteArray();
    }

    private static void denied(byte[] ave, byte[] bfk) throws Exception {
        try {
            RendererLivePreflight.checkExactClasses(ave,bfk,sha(ave),sha(bfk));
            throw new AssertionError("Unsupported renderer shape admitted");
        } catch(IllegalStateException expected) {
            // Correct fail-closed behavior.
        }
    }

    private static Instrumentation unrelatedJVM() {
        return (Instrumentation)Proxy.newProxyInstance(
            Instrumentation.class.getClassLoader(),new Class<?>[]{Instrumentation.class},
            new InvocationHandler() {
                @Override public Object invoke(Object proxy, Method method, Object[] args) {
                    if("isRetransformClassesSupported".equals(method.getName()))return true;
                    if("getAllLoadedClasses".equals(method.getName()))
                        return new Class<?>[]{RendererLivePreflightSmoke.class};
                    throw new AssertionError("No mutation allowed for unrelated JVM");
                }
            });
    }

    public static void main(String[] args) throws Exception {
        byte[] game=game(true,false);
        byte[] target=renderer("bfk",false);
        RendererLivePreflight.checkExactClasses(game,target,sha(game),sha(target));
        boolean refused=false;
        try {
            RendererLivePreflight.checkExactClasses(
                game,target,"000000",sha(target));
        } catch(IllegalStateException expected){refused=true;}
        check(refused,"Incorrect SHA admitted");
        denied(game(false,false),target);
        denied(game(true,true),target);
        denied(game,renderer("other",false));
        denied(game,renderer("bfk",true));
        denied(new byte[]{1,2,3},target);
        try {
            RendererLivePreflight.run(unrelatedJVM());
            throw new AssertionError("Unrelated JVM admitted");
        } catch(IllegalStateException expected) {
            check(expected.getMessage().contains("not loaded"),
                "Unexpected unrelated JVM denial");
        }
        System.out.println("R73_RENDERER_PREFLIGHT_SYNTHETIC_GUARDS_PASS");
        System.out.println("R73_UNRELATED_JVM_REJECTED_BEFORE_TRANSFORM_PASS");
    }

    private RendererLivePreflightSmoke() {}
}
