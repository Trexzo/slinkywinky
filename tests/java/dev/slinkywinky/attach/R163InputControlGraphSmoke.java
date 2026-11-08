package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.OriginalKeyboardBridgeWeaver;
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

/**
 * R163 original-class CFG negative controls, Java 8 and ASM 5.2 only.
 * Everything remains in memory. No JVM retransformation, Java Attach, or
 * original Minecraft input queue is called by this test.
 */
public final class R163InputControlGraphSmoke implements Opcodes {
    private R163InputControlGraphSmoke(){}
    private static void check(boolean ok,String message){
        if(!ok)throw new AssertionError("R163 "+message);
    }
    private static byte[] fixture(int branchOpcode) {
        ClassWriter writer=new ClassWriter(ClassWriter.COMPUTE_MAXS);
        writer.visit(V1_8,ACC_PUBLIC,"ave",null,"java/lang/Object",null);
        MethodVisitor m=writer.visitMethod(ACC_PUBLIC,"s","()V",null,null);
        m.visitCode();
        Label skip=new Label(),done=new Label();
        m.visitInsn(ICONST_0);
        m.visitJumpInsn(branchOpcode,skip);
        m.visitMethodInsn(INVOKESTATIC,"org/lwjgl/input/Mouse",
            "next","()Z",false);m.visitInsn(POP);
        m.visitLabel(skip);
        m.visitMethodInsn(INVOKESTATIC,"org/lwjgl/input/Keyboard",
            "next","()Z",false);m.visitInsn(POP);
        m.visitInsn(ICONST_0);
        m.visitMethodInsn(INVOKESTATIC,"org/lwjgl/input/Keyboard",
            "isKeyDown","(I)Z",false);m.visitInsn(POP);
        m.visitJumpInsn(GOTO,done);
        m.visitLabel(done);
        m.visitInsn(RETURN);
        m.visitMaxs(0,0);
        m.visitEnd();writer.visitEnd();
        return writer.toByteArray();
    }
    // The same call instructions, same order, same branch targets and exact
    // method layout, but inverted first conditional branch operation.
    private static byte[] flipOriginalBranch(byte[] original) {
        final int[] changed={0};
        ClassReader reader=new ClassReader(original);
        ClassWriter writer=new ClassWriter(reader,0);
        reader.accept(new ClassVisitor(ASM5,writer){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] exceptions) {
                MethodVisitor out=super.visitMethod(access,name,desc,sig,exceptions);
                final boolean target="s".equals(name)&&"()V".equals(desc);
                return new MethodVisitor(ASM5,out){
                    @Override public void visitJumpInsn(int opcode,Label label) {
                        if(target && changed[0]==0){
                            if(opcode==IFEQ){opcode=IFNE;changed[0]++;}
                            else if(opcode==IFNE){opcode=IFEQ;changed[0]++;}
                            else if(opcode==IFNULL){opcode=IFNONNULL;changed[0]++;}
                            else if(opcode==IFNONNULL){opcode=IFNULL;changed[0]++;}
                            else if(opcode==IF_ICMPEQ){
                                opcode=IF_ICMPNE;changed[0]++;
                            } else if(opcode==IF_ICMPNE){
                                opcode=IF_ICMPEQ;changed[0]++;
                            }
                        }
                        super.visitJumpInsn(opcode,label);
                    }
                };
            }
        },0);
        check(changed[0]==1,"Could not invert a conditional original branch");
        return writer.toByteArray();
    }
    private static void synthetic(){
        byte[] initial=fixture(IFEQ);
        byte[] inverted=fixture(IFNE);
        R163InputControlGraph.Report baseline=
            R163InputControlGraph.inspect(initial);
        R163InputControlGraph.Report changed=
            R163InputControlGraph.inspect(inverted);
        check(baseline.instructions==changed.instructions &&
              baseline.jumps==changed.jumps &&
              baseline.sourceSites==changed.sourceSites,
              "Generated branch negative control differs in counts");
        check(!baseline.fingerprint().equals(changed.fingerprint()),
              "Equivalent-count branch inversion undetected");
        check(!baseline.canAuthorizeLiveModal() &&
              !changed.canAuthorizeLiveModal(),
              "Offline CFG report authorized live client interception");
        boolean refused=false;
        try{R163InputControlGraph.inspect(new byte[]{1,2,3});}
        catch(IllegalArgumentException expected){refused=true;}
        check(refused,"Malformed CFG owner input accepted");
        refused=false;
        try{R163InputControlGraph.inspect(null);}
        catch(IllegalArgumentException expected){refused=true;}
        check(refused,"Missing CFG owner input accepted");
        System.out.println("R163_SYNTHETIC_BRANCH_OPCODE_NEGATIVE_CONTROL_PASS");
    }
    private static void original(String jar) throws Exception {
        check(Files.isRegularFile(Paths.get(jar)),"Original Mojang 1.8.9 jar unavailable");
        RenderCallsiteAudit.main(new String[]{jar});
        byte[] raw;
        try(ZipFile z=new ZipFile(jar)){
            raw=RenderCallsiteAudit.entry(z,"ave.class");
        }
        final byte[] original=raw.clone();
        R162InputOrderLedger.Report source=R162InputOrderLedger.pinned(raw);
        R163InputControlGraph.Report base=R163InputControlGraph.pinned(raw);
        check(base.sourceSites==source.sites.size() &&
              base.sourceSites==58 && base.jumps==179 &&
              base.handlers==3,"CFG and source ledger disagree");
        check(base.instructions>base.sourceSites &&
              base.exceptionalEdges>0,"CFG too sparse to audit");
        check(base.reachableSources>0 &&
              base.reachableDirectKeys>0 &&
              base.reachableHeldPolls>0 &&
              base.reachablePressPolls>0,
              "No reachable residual vanilla input consumers found");
        check(!base.canAuthorizeLiveModal(),
              "Original read-only source graph claimed live ownership");

        // This existing keyboard-only rewrite remains an offline probe.
        // Canonicalize exactly the substituted queue owner's spelling;
        // other opcode and branch identities must remain unchanged.
        byte[] oneQueue=OriginalKeyboardBridgeWeaver.weavePinnedOriginal(raw);
        R163InputControlGraph.Report bridged=R163InputControlGraph.inspect(oneQueue);
        check(base.fingerprint().equals(bridged.fingerprint()) &&
              base.instructions==bridged.instructions &&
              base.exceptionalEdges==bridged.exceptionalEdges,
              "Queue-only patch unexpectedly affected other control flow");

        byte[] inverted=flipOriginalBranch(raw);
        R162InputOrderLedger.Report originalOrder=R162InputOrderLedger.inspect(raw);
        R162InputOrderLedger.Report mutatedOrder=R162InputOrderLedger.inspect(inverted);
        check(originalOrder.digest().equals(mutatedOrder.digest()) &&
              originalOrder.jumps==mutatedOrder.jumps &&
              originalOrder.handlers==mutatedOrder.handlers &&
              originalOrder.methodCalls==mutatedOrder.methodCalls,
              "Branch mutation does not preserve old source-order evidence");
        R163InputControlGraph.Report mutant=R163InputControlGraph.inspect(inverted);
        check(base.instructions==mutant.instructions &&
              base.jumps==mutant.jumps &&
              base.handlers==mutant.handlers &&
              base.sourceSites==mutant.sourceSites,
              "Branch mutation changed source counts");
        check(!base.fingerprint().equals(mutant.fingerprint()),
              "R163 branch change invisible to CFG fingerprint");
        boolean originalPinRejected=false;
        try{R163InputControlGraph.pinned(inverted);}
        catch(IllegalArgumentException expected){originalPinRejected=true;}
        check(originalPinRejected,"Modified original method passed strict pin");
        check(Arrays.equals(raw,original),
              "Original Mojang 1.8.9 class bytes unexpectedly modified");
        // This restoration is a COPY in test memory, not restoration of a
        // loaded class in a running Minecraft JVM.
        byte[] restored=original.clone();
        check(R162InputOrderLedger.pinned(restored).digest().equals(
                 R162InputOrderLedger.PINNED_ORDER_SHA256) &&
              base.fingerprint().equals(
                 R163InputControlGraph.pinned(restored).fingerprint()),
              "Exact-byte offline copy failed source and CFG restore");
        System.out.println("R163_ORIGINAL_VANILLA_INPUT_CFG_SHA256 "+
                           base.fingerprint());
        System.out.println("R163_ORIGINAL_VANILLA_CFG_COUNTS instructions="+
            base.instructions+" jumps="+base.jumps+
            " handlers="+base.handlers+" exceptionEdges="+base.exceptionalEdges+
            " inputSites="+base.sourceSites+" reachable="+base.reachableSources+
            " directKeys="+base.reachableDirectKeys+
            " heldPolls="+base.reachableHeldPolls+
            " pressPolls="+base.reachablePressPolls);
        System.out.println("R163_ORIGINAL_COUNT_AND_ORDER_PRESERVING_BRANCH_MUTATION_DETECTED");
        System.out.println("R163_QUEUE_ONLY_MEMORY_WEAVE_CFG_IDENTICAL");
        System.out.println("R163_OFFLINE_EXACT_BYTE_RESTORATION_ONLY");
        System.out.println("R163_NO_LIVE_INPUT_HOOK_OR_GAME_ACTION");
    }
    public static void main(String[] args) throws Exception {
        check(args.length<=1,"Expected at most one original JAR");
        synthetic();
        if(args.length==1)original(args[0]);
    }
}
