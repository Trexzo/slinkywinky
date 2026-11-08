package dev.slinkywinky.attach;

import java.lang.reflect.Method;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

public final class R164OfflineInputCutSmoke implements Opcodes {
    private static void check(boolean yes,String message){
        if(!yes)throw new AssertionError("R164 "+message);
    }
    private static final String TARGET="dev/slinkywinky/attach/R164TinyInputTarget";
    private R164OfflineInputCutSmoke(){}
    private static byte[] fixture(){
        ClassWriter w=new ClassWriter(ClassWriter.COMPUTE_MAXS);
        w.visit(V1_6,ACC_PUBLIC,TARGET,null,"java/lang/Object",null);
        w.visitField(ACC_PUBLIC|ACC_STATIC,"executions","I",null,null).visitEnd();
        MethodVisitor ctor=w.visitMethod(ACC_PUBLIC,"<init>","()V",null,null);
        ctor.visitCode();
        ctor.visitVarInsn(ALOAD,0);
        ctor.visitMethodInsn(INVOKESPECIAL,"java/lang/Object","<init>","()V",false);
        ctor.visitInsn(RETURN);ctor.visitMaxs(0,0);ctor.visitEnd();
        MethodVisitor run=w.visitMethod(ACC_PUBLIC,"run","()V",null,null);
        run.visitCode();
        run.visitFieldInsn(GETSTATIC,TARGET,"executions","I");
        run.visitInsn(ICONST_1);
        run.visitInsn(IADD);
        run.visitFieldInsn(PUTSTATIC,TARGET,"executions","I");
        run.visitInsn(RETURN);
        run.visitMaxs(0,0);run.visitEnd();w.visitEnd();
        return w.toByteArray();
    }
    private static final class FixtureLoader extends ClassLoader {
        FixtureLoader(){super(R164OfflineInputCutSmoke.class.getClassLoader());}
        Class<?> loadTarget(byte[] bytes) {
            return defineClass("dev.slinkywinky.attach.R164TinyInputTarget",
                               bytes,0,bytes.length);
        }
    }
    private static void synthetic() throws Exception {
        byte[] generated=fixture();
        byte[] patched=R164OfflineInputCut.inMemoryGeneratedFixture(generated);
        check(!Arrays.equals(generated,patched),"Fixture not modified");
        Class<?> target=new FixtureLoader().loadTarget(patched);
        Object instance=target.getConstructor().newInstance();
        Method run=target.getMethod("run");
        R164OfflineModalGate.setForFixture(false);
        run.invoke(instance);
        run.invoke(instance);
        int vanilla=target.getField("executions").getInt(null);
        check(vanilla==2,"Vanilla branch did not execute twice");
        R164OfflineModalGate.setForFixture(true);
        run.invoke(instance);
        run.invoke(instance);
        check(target.getField("executions").getInt(null)==2,
              "Quarantine branch executed original input work");
        R164OfflineModalGate.setForFixture(false);
        run.invoke(instance);
        check(target.getField("executions").getInt(null)==3,
              "Quarantine release did not restore vanilla branch");
        R164OfflineModalGate.setForFixture(false);
        System.out.println("R164_TEST_ONLY_GATE_VANILLA_MODAL_VANILLA_EXECUTION_PASS");
    }
    private static byte[] sabotageFirstConditional(byte[] original) {
        final int[] mutated={0};
        ClassReader reader=new ClassReader(original);
        ClassWriter writer=new ClassWriter(reader,0);
        reader.accept(new ClassVisitor(ASM5,writer){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] ex){
                MethodVisitor out=super.visitMethod(access,name,desc,sig,ex);
                if(!"s".equals(name)||!"()V".equals(desc))return out;
                return new MethodVisitor(ASM5,out){
                    @Override public void visitJumpInsn(int op,Label label) {
                        if(mutated[0]==0 && op==IFEQ){
                            op=IFNE;mutated[0]++;
                        }
                        super.visitJumpInsn(op,label);
                    }
                };
            }
        },0);
        check(mutated[0]==1,"No inserted guard condition to invert");
        return writer.toByteArray();
    }
    private static byte[] sabotageReturn(byte[] original) {
        final int[] mutant={0};
        ClassReader reader=new ClassReader(original);
        ClassWriter writer=new ClassWriter(reader,0);
        reader.accept(new ClassVisitor(ASM5,writer){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] ex){
                MethodVisitor out=super.visitMethod(access,name,desc,sig,ex);
                if(!"s".equals(name)||!"()V".equals(desc))return out;
                return new MethodVisitor(ASM5,out){
                    private boolean afterGate,afterJump;
                    @Override public void visitMethodInsn(int op,String owner,
                            String called,String sign,boolean itf) {
                        if(owner.equals("dev/slinkywinky/attach/R164OfflineModalGate"))
                            afterGate=true;
                        super.visitMethodInsn(op,owner,called,sign,itf);
                    }
                    @Override public void visitJumpInsn(int op,Label label) {
                        if(afterGate && !afterJump)afterJump=true;
                        super.visitJumpInsn(op,label);
                    }
                    @Override public void visitInsn(int op) {
                        if(afterJump && mutant[0]==0 && op==RETURN){
                            op=NOP;mutant[0]++;
                        }
                        super.visitInsn(op);
                    }
                };
            }
        },0);
        check(mutant[0]==1,"No modal return to replace");
        return writer.toByteArray();
    }
    private static boolean refused(byte[] original,byte[] candidate){
        try {R164OfflineInputCut.verifyOriginal(original,candidate);return false;}
        catch(IllegalArgumentException expected){return true;}
    }
    private static void original(String jar)throws Exception {
        check(Files.isRegularFile(Paths.get(jar)),"Original vanilla jar missing");
        RenderCallsiteAudit.main(new String[]{jar});
        byte[] raw;
        try(ZipFile zip=new ZipFile(jar)) {
            raw=RenderCallsiteAudit.entry(zip,"ave.class");
        }
        byte[] baseline=raw.clone();
        R163InputControlGraph.Report graph=R163InputControlGraph.pinned(raw);
        byte[] candidate=R164OfflineInputCut.inMemoryOriginal(raw);
        check(candidate.length>raw.length,"No synthetic modal entry cut");
        R164OfflineInputCut.verifyOriginal(raw,candidate);
        check(refused(raw,sabotageFirstConditional(candidate)),
              "Reversed vanilla/modal routing was accepted");
        check(refused(raw,sabotageReturn(candidate)),
              "Removal of modal early-return was accepted");
        check(refused(raw,raw),"Unmodified class considered quarantined");
        check(refused(raw,null),"Missing candidate accepted");
        boolean doubleRewriteRefused=false;
        try{R164OfflineInputCut.inMemoryOriginal(candidate);}
        catch(IllegalArgumentException expected){doubleRewriteRefused=true;}
        check(doubleRewriteRefused,"Double weaving accepted");
        check(Arrays.equals(raw,baseline),"Original byte array mutated");
        byte[] copied=baseline.clone();
        check(Arrays.equals(raw,copied) &&
              graph.fingerprint().equals(
                R163InputControlGraph.pinned(copied).fingerprint()),
              "Byte-exact offline copy did not restore original fingerprint");
        System.out.println("R164_ORIGINAL_AVE_ENTRY_ONLY_MODAL_CUT_SHAPE_PASS");
        System.out.println("R164_ORIGINAL_58_INPUT_PATHS_UNCHANGED_VANILLA_BRANCH_PASS");
        System.out.println("R164_ORIGINAL_REVERSED_GATE_AND_MISSING_RETURN_REFUSED_PASS");
        System.out.println("R164_SOURCE_IN_MEMORY_COPY_RESTORED_OFFLINE_ONLY_PASS");
        System.out.println("R164_NO_JVM_CLASS_TRANSFORMATION_INSTALLED");
    }
    public static void main(String[] args)throws Exception {
        check(args.length<=1,"Unexpected arguments");
        synthetic();
        if(args.length==1)original(args[0]);
    }
}
