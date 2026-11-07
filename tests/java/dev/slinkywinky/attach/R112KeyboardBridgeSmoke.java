package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.KeyboardQueueOwner;
import dev.slinkywinky.instrument.OriginalKeyboardBridgeWeaver;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import org.lwjgl.input.Keyboard;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R111: execute the production ASM rewrite's TEST-ONLY path in an isolated
 * Java 8 classloader. Fake LWJGL2; no Minecraft, OS input, or Attach.
 */
public final class R112KeyboardBridgeSmoke implements Opcodes {
    private static final String TARGET="dev.slinkywinky.attach.R112KeyboardTarget";
    private static final String ORIGINAL_CALL=
        "M:184:org/lwjgl/input/Keyboard:next:()Z:false";
    private static final String BRIDGED_CALL=
        "M:184:dev/slinkywinky/instrument/KeyboardQueueOwner:next:()Z:false";
    private static void check(boolean yes,String message) {
        if(!yes)throw new AssertionError(message);
    }
    private static byte[] fixture() throws Exception {
        String resource=TARGET.replace('.','/')+".class";
        InputStream input=R112KeyboardBridgeSmoke.class.getClassLoader()
            .getResourceAsStream(resource);
        check(input!=null,"Generated fixture class missing");
        try {
            ByteArrayOutputStream output=new ByteArrayOutputStream();
            byte[] buffer=new byte[4096];int n;
            while((n=input.read(buffer))!=-1)output.write(buffer,0,n);
            return output.toByteArray();
        }finally {input.close();}
    }
    private static final class Isolated extends ClassLoader {
        Isolated() {super(R112KeyboardBridgeSmoke.class.getClassLoader());}
        Class<?> define(byte[] bytes) {
            return defineClass(TARGET,bytes,0,bytes.length);
        }
    }
    private static boolean refused(byte[] raw) {
        try{
            OriginalKeyboardBridgeWeaver.weaveGeneratedFixtureForTest(raw);
            return false;
        }catch(RuntimeException expected){return true;}
    }
    private static byte[] damaged(byte[] input,final boolean duplicate) {
        ClassReader reader=new ClassReader(input);
        ClassWriter writer=new ClassWriter(reader,ClassWriter.COMPUTE_MAXS);
        reader.accept(new ClassVisitor(ASM5,writer) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String signature,String[] exceptions) {
                MethodVisitor delegate=super.visitMethod(access,name,desc,
                                                          signature,exceptions);
                if(!"run".equals(name)||!"()V".equals(desc))return delegate;
                return new MethodVisitor(ASM5,delegate) {
                    @Override public void visitMethodInsn(int opcode,String owner,
                            String method,String descriptor,boolean iface) {
                        if(opcode==INVOKESTATIC &&
                           "org/lwjgl/input/Keyboard".equals(owner) &&
                           "next".equals(method) && "()Z".equals(descriptor)) {
                            if(duplicate) {
                                super.visitMethodInsn(opcode,owner,method,descriptor,iface);
                                super.visitInsn(POP);
                            }else method="replacedNext";
                        }
                        super.visitMethodInsn(opcode,owner,method,descriptor,iface);
                    }
                };
            }
        },0);
        return writer.toByteArray();
    }
    private static int stat(Class<?> type,String name)throws Exception {
        return type.getField(name).getInt(null);
    }
    public static void main(String[] args)throws Exception {
        final byte[] original=fixture();
        final byte[] saved=original.clone();
        check(!OriginalKeyboardBridgeWeaver.exactOriginal(original),
              "Synthetic Java target treated as original Mojang ave");
        boolean sourceRefused=false;
        try {OriginalKeyboardBridgeWeaver.weavePinnedOriginal(original);}
        catch(IllegalArgumentException expected){sourceRefused=true;}
        check(sourceRefused,"Original-only transformer accepted synthetic bytes");
        check(refused(null),"Null fixture admitted");
        check(refused(new byte[]{1,2,3}),"Truncated fixture admitted");
        check(refused(damaged(original,true)),"Duplicate LWJGL next accepted");
        check(refused(damaged(original,false)),"Missing LWJGL next accepted");

        byte[] changed=OriginalKeyboardBridgeWeaver.weaveGeneratedFixtureForTest(original);
        check(Arrays.equals(saved,original),"Original generated fixture modified");
        check(refused(changed),"Already transformed fixture accepted");
        List<String> before=OriginalKeyboardLoopWeaveAudit.trace(original);
        List<String> expected=new ArrayList<String>(before);
        check(java.util.Collections.frequency(expected,ORIGINAL_CALL)==1,
              "Expected one raw keyboard call");
        expected.set(expected.indexOf(ORIGINAL_CALL),BRIDGED_CALL);
        check(expected.equals(OriginalKeyboardLoopWeaveAudit.trace(changed)),
              "Bytecode bridge altered more than original Keyboard.next()");

        // Original and modified fixture classes share only the fake public
        // Keyboard and the first-party R110 gate through their parent loader.
        Keyboard.reset();
        Class<?> unmodified=new Isolated().define(original);
        Method vanilla=unmodified.getMethod("run");
        Keyboard.enqueue(17,true);Keyboard.enqueue(17,false);
        vanilla.invoke(null);
        check(stat(unmodified,"delivered")==2 &&
              stat(unmodified,"released")==1 && stat(unmodified,"keySum")==34,
              "Unmodified fixture's normal key delivery not established");

        Keyboard.reset();
        Class<?> rewritten=new Isolated().define(changed);
        Method call=rewritten.getMethod("run");
        check(KeyboardQueueOwner.bind(Keyboard.class,
              Keyboard.class.getClassLoader(),true),"R111 fixture gateway not bound");
        Keyboard.enqueue(18,true);
        call.invoke(null);
        check(stat(rewritten,"delivered")==1 && stat(rewritten,"keySum")==18,
              "Unarmed transformed code changed normal input");

        final List<String> nav=new ArrayList<String>();
        check(KeyboardQueueOwner.observeForeground(true),"Owner focus refused");
        check(KeyboardQueueOwner.arm(1,true,true,new KeyboardQueueOwner.KeySink() {
            @Override public void onEdge(int key,boolean down) {
                nav.add(key+":"+(down?"D":"U"));
            }
        }),"Owner modal refused after consent");
        Keyboard.enqueue(30,true);Keyboard.enqueue(30,true);
        Keyboard.enqueue(30,false);Keyboard.enqueue(31,true);
        Keyboard.enqueue(31,false);
        call.invoke(null);
        check(stat(rewritten,"delivered")==1 &&
              nav.equals(Arrays.asList("30:D","30:U","31:D","31:U")),
              "R111 synthetic live callsite leaked events or repeated a key");
        check(KeyboardQueueOwner.close(true),"Modal close refused");
        call.invoke(null);
        check("VANILLA".equals(KeyboardQueueOwner.state()),
              "Modal did not release after observed empty queue");
        Keyboard.enqueue(32,true);Keyboard.enqueue(32,false);
        call.invoke(null);
        check(stat(rewritten,"delivered")==3 &&
              stat(rewritten,"released")==1 &&
              stat(rewritten,"keySum")==82,
              "Original post-modal event delivery failed");
        check(KeyboardQueueOwner.unbind(true,true,true),"R111 fixture cleanup failed");
        Keyboard.reset();

        System.out.println("R112_SYNTHETIC_TRANSFORMED_CLASS_EXECUTION_AND_KEY_OWNER_PASS");
        System.out.println("R112_EXACT_ONE_CALLSITE_MODAL_NO_QUEUE_PASSTHROUGH_FIXTURE_PASS");
        System.out.println("R112_NO_LIVE_MINECRAFT_OR_ANTICHEAT_OR_GAMEPLAY_HOOK");
    }
    private R112KeyboardBridgeSmoke(){}
}
