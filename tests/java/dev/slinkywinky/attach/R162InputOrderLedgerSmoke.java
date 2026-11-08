package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.OriginalKeyboardBridgeWeaver;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.Label;

/** Java 8 offline source-only original vanilla method audit. */
public final class R162InputOrderLedgerSmoke implements Opcodes {
    private static void check(boolean ok,String msg) {
        if(!ok)throw new AssertionError("R162 "+msg);
    }
    private static boolean denied(Runnable r){
        try{r.run();return false;}
        catch(IllegalArgumentException expected){return true;}
        catch(IllegalStateException expected){return true;}
    }
    private static void method(MethodVisitor mv,String owner,String name,
                               String desc,boolean virtual) {
        if(virtual)mv.visitInsn(ACONST_NULL);
        else if(desc.equals("(I)Z"))mv.visitInsn(ICONST_0);
        mv.visitMethodInsn(virtual?INVOKEVIRTUAL:INVOKESTATIC,
                owner,name,desc,false);
        if(desc.endsWith("Z"))mv.visitInsn(POP);
    }
    private static byte[] orderFixture(boolean swap) {
        ClassWriter writer=new ClassWriter(ClassWriter.COMPUTE_MAXS);
        writer.visit(V1_8,ACC_PUBLIC,"ave",null,"java/lang/Object",null);
        MethodVisitor mv=writer.visitMethod(ACC_PUBLIC,"s","()V",null,null);
        mv.visitCode();
        method(mv,"org/lwjgl/input/Mouse","next","()Z",false);
        method(mv,"org/lwjgl/input/Mouse","getEventButtonState","()Z",false);
        method(mv,"org/lwjgl/input/Keyboard","next","()Z",false);
        method(mv,"org/lwjgl/input/Keyboard","isKeyDown","(I)Z",false);
        if(swap){
            method(mv,"avb","d","()Z",true);
            method(mv,"avb","f","()Z",true);
        }else{
            method(mv,"avb","f","()Z",true);
            method(mv,"avb","d","()Z",true);
        }
        Label end=new Label();
        mv.visitJumpInsn(GOTO,end);
        mv.visitLabel(end);
        mv.visitInsn(RETURN);
        mv.visitMaxs(0,0);
        mv.visitEnd();writer.visitEnd();
        return writer.toByteArray();
    }
    // The old R119 count comparison cannot distinguish this same-count
    // original-class mutation. We modify one f() and one d(), preserving
    // method signatures, instruction count, and branch count.
    private static byte[] reorderOriginal(byte[] original) {
        final int[] f={0},d={0};
        final ClassReader reader=new ClassReader(original);
        final ClassWriter writer=new ClassWriter(reader,0);
        reader.accept(new ClassVisitor(ASM5,writer){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] except) {
                MethodVisitor out=super.visitMethod(access,name,desc,sig,except);
                final boolean target="s".equals(name)&&"()V".equals(desc);
                return new MethodVisitor(ASM5,out){
                    @Override public void visitMethodInsn(int op,String owner,
                            String method,String signature,boolean itf) {
                        if(target && op==INVOKEVIRTUAL && "avb".equals(owner) &&
                           "()Z".equals(signature)){
                            if("f".equals(method)&&f[0]++==0)method="d";
                            else if("d".equals(method)&&d[0]++==0)method="f";
                        }
                        super.visitMethodInsn(op,owner,method,signature,itf);
                    }
                };
            }
        },0);
        check(f[0]>0 && d[0]>0,"Original mutation control unavailable");
        return writer.toByteArray();
    }
    private static void synthetic(){
        R162InputOrderLedger.Report before=
            R162InputOrderLedger.inspect(orderFixture(false));
        R162InputOrderLedger.Report reordered=
            R162InputOrderLedger.inspect(orderFixture(true));
        check(before.count("BIND_HELD_POLL")==reordered.count("BIND_HELD_POLL") &&
              before.count("BIND_PRESS_POLL")==reordered.count("BIND_PRESS_POLL"),
              "Same-count control not actually equal");
        check(before.methodCalls==reordered.methodCalls &&
              before.jumps==reordered.jumps &&
              before.sites.size()==reordered.sites.size(),
              "Same-size control not comparable");
        check(!before.digest().equals(reordered.digest()),
              "Reordered f/d input paths share one fingerprint");
        check(!before.canAuthorizeLiveInput() &&
              !reordered.canAuthorizeLiveInput(),"Offline ledger authorized hooks");
        check(denied(new Runnable(){@Override public void run(){
            R162InputOrderLedger.pinned(orderFixture(false));
        }}),"Generated class admitted original SHA pin");
        check(denied(new Runnable(){@Override public void run(){
            R162InputOrderLedger.inspect(new byte[]{1,2,3});
        }}),"Malformed class admitted");
        check(denied(new Runnable(){@Override public void run(){
            R162InputOrderLedger.inspect(null);
        }}),"Missing class admitted");
        System.out.println("R162_SAME_COUNT_REORDERED_SOURCE_CALLSITE_REFUSED_PASS");
        System.out.println("R162_TEST_ONLY_ORDER_LEDGER_NEVER_ADMITS_LIVE_INPUT_PASS");
    }
    private static void original(String jarPath)throws Exception {
        check(Files.isRegularFile(Paths.get(jarPath)),"Original jar missing");
        // Jar size/SHA-1 AND entry identity are required.
        RenderCallsiteAudit.main(new String[]{jarPath});
        byte[] original;
        try(ZipFile zip=new ZipFile(jarPath)) {
            original=RenderCallsiteAudit.entry(zip,"ave.class");
        }
        final byte[] untouched=original.clone();
        R162InputOrderLedger.Report report=R162InputOrderLedger.pinned(original);
        check(report.digest().equals(
                 R162InputOrderLedger.PINNED_ORDER_SHA256),
              "Original source instruction-order fingerprint changed");
        check(report.count("DIRECT_KEY_DOWN")==17 &&
              report.count("BIND_HELD_POLL")==13 &&
              report.count("BIND_PRESS_POLL")==3 &&
              report.count("GUI_DISPATCH")>=1,
              "Some original input category unaccounted");
        check(report.sites.size()>35 && report.jumps>0 &&
              report.methodCalls>report.sites.size(),
              "Lexical source ledger unexpectedly sparse");
        final byte[] inMemoryWeave=
            OriginalKeyboardBridgeWeaver.weavePinnedOriginal(original);
        R162InputOrderLedger.Report normalized=
            R162InputOrderLedger.inspect(inMemoryWeave);
        check(normalized.digest().equals(report.digest()) &&
              normalized.jumps==report.jumps &&
              normalized.switches==report.switches &&
              normalized.handlers==report.handlers,
              "Queue-only rewritten source shifted other input instructions");
        byte[] mutated=reorderOriginal(original);
        R162InputOrderLedger.Report wrong=
            R162InputOrderLedger.inspect(mutated);
        OriginalInputOwnershipAudit.Report oldCounts=
            OriginalInputOwnershipAudit.inspectCandidate(mutated);
        check(oldCounts.rawKeyDown==17 && oldCounts.heldBindingPolls==13 &&
              oldCounts.pressBindingPolls==3 &&
              oldCounts.mouseNext==1,
              "Tamper control did not preserve original R119 counts");
        check(wrong.methodCalls==report.methodCalls &&
              wrong.jumps==report.jumps &&
              wrong.switches==report.switches &&
              wrong.handlers==report.handlers,
              "Tamper unexpectedly changed instruction/branch totals");
        check(!wrong.digest().equals(report.digest()),
              "Same-count unsafe source mutation undetected");
        check(denied(new Runnable(){@Override public void run(){
            R162InputOrderLedger.pinned(mutated);
        }}),"Same-count mutated original accepted SHA pin");
        check(Arrays.equals(original,untouched),"Original bytes modified");
        System.out.println("R162_ORIGINAL_AVE_INPUT_CALLSITE_SEQUENCE_SHA256 "+
                           report.digest());
        System.out.println("R162_ORIGINAL_AVE_INPUT_BRANCH_SHAPE "+
                           "calls="+report.methodCalls+" jumps="+report.jumps+
                           " switches="+report.switches+
                           " handlers="+report.handlers+
                           " recordedSites="+report.sites.size());
        System.out.println("R162_ORIGINAL_INPUT_SOURCE_PHASES_PASS");
        System.out.println("R162_QUEUE_ONLY_IN_MEMORY_WEAVE_ORDER_UNCHANGED_PASS");
        System.out.println("R162_ORIGINAL_COUNT_PRESERVING_MUTATION_DETECTED_PASS");
        System.out.println("R162_NO_VANILLA_INPUT_INTERCEPTION");
    }
    public static void main(String[] args)throws Exception {
        check(args.length<=1,"Unexpected extra source argument");
        synthetic();
        if(args.length==1)original(args[0]);
    }
}
