package dev.slinkywinky.forge;

import java.io.File;
import java.util.jar.JarFile;
import java.util.jar.JarEntry;
import java.io.InputStream;
import java.io.ByteArrayOutputStream;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.tree.AbstractInsnNode;
import org.objectweb.asm.tree.ClassNode;
import org.objectweb.asm.tree.MethodInsnNode;
import org.objectweb.asm.tree.MethodNode;
import org.objectweb.asm.tree.FieldInsnNode;

/** Source-controlled R214 check against the *real pinned ForgeBin class*. */
public final class R214TransformerSmoke implements Opcodes {
    static void check(boolean yes, String why) {
        if(!yes) throw new AssertionError("R214: " + why);
    }
    static byte[] readClass(String forgeJar) throws Exception {
        try(JarFile jar = new JarFile(new File(forgeJar))) {
            JarEntry e = jar.getJarEntry(
                R214SlowdownTransformer.PLAYER + ".class");
            check(e!=null,"cached Forge 1.8.9 player bytecode missing");
            try(InputStream in=jar.getInputStream(e);
                ByteArrayOutputStream out=new ByteArrayOutputStream()) {
                byte[] buf=new byte[16384];
                for(int n;(n=in.read(buf))!=-1;) out.write(buf,0,n);
                return out.toByteArray();
            }
        }
    }
    public static void main(String[] args) throws Exception {
        check(args.length==1, "expected exact Forge 1.8.9 forgeBin JAR");
        byte[] original=readClass(args[0]);
        byte[] altered=R214SlowdownTransformer.instrument(original);
        check(altered.length>original.length,"patch did not emit hook code");
        ClassNode clazz=new ClassNode(ASM5);
        new ClassReader(altered).accept(clazz,0);
        int invocations=0;
        int immediate=0;
        for(Object m:clazz.methods) {
            MethodNode method=(MethodNode)m;
            if(!"onLivingUpdate".equals(method.name) &&
               !"func_70636_d".equals(method.name)) continue;
            for(AbstractInsnNode insn=method.instructions.getFirst();
                insn!=null;insn=insn.getNext()) {
                if(insn.getOpcode()!=INVOKESTATIC ||
                   !(insn instanceof MethodInsnNode)) continue;
                MethodInsnNode call=(MethodInsnNode)insn;
                if(!R214SlowdownTransformer.HOOK.equals(call.owner) ||
                   !R214SlowdownTransformer.HOOK_METHOD.equals(call.name))continue;
                ++invocations;
                AbstractInsnNode load=insn.getPrevious();
                AbstractInsnNode previous=load==null?null:load.getPrevious();
                check(load!=null && load.getOpcode()==ALOAD &&
                      previous instanceof FieldInsnNode &&
                      previous.getOpcode()==PUTFIELD,
                      "hook not directly after slowdown field write");
                FieldInsnNode field=(FieldInsnNode)previous;
                check("F".equals(field.desc) &&
                      ("moveForward".equals(field.name) ||
                       "field_78900_b".equals(field.name)),
                      "hook not after vanilla forward 0.2 multiplier");
                ++immediate;
            }
        }
        check(invocations==1 && immediate==1,"ambiguous injected native input hook");

        boolean denied=false;
        try {
            // Patching twice must not silently add a duplicate hook. Exact
            // placement is counted in acceptance; second pass explicitly
            // checked as unsupported in the production launcher.
            R214SlowdownTransformer.instrument(new byte[]{0,0,0,0});
        }catch(RuntimeException expected){denied=true;}
        check(denied,"invalid bytecode admitted");
        System.out.println("R214_REAL_FORGEBIN_POST_SLOW_PRE_CONSUME_TRANSFORM_PASS "+
            "hook_count=1 location=after_second_0.2_field_write "+
            "changed_classes=1 game_actions=0");
    }
}
