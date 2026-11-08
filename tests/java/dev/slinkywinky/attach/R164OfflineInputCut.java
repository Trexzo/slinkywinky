package dev.slinkywinky.attach;

import java.util.Arrays;
import java.util.List;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R164 TEST-ONLY in-memory input-method entry cut. This class NEVER appears
 * in the Java agent JAR and is never called from any original game hook.
 * The test-only gate bypasses *all* of this one method's original bytecode,
 * not the game's other input owners, the OS, or the physical device.
 */
public final class R164OfflineInputCut implements Opcodes {
    private static final String GATE=
        "dev/slinkywinky/attach/R164OfflineModalGate";
    private static final String PREDICATE="shouldQuarantineInput";
    private R164OfflineInputCut(){}

    private static void require(boolean good,String msg) {
        if(!good)throw new IllegalArgumentException("R164 "+msg);
    }
    private static byte[] insert(final byte[] input,final String owner,
                                 final String methodName) {
        final ClassReader r=new ClassReader(input);
        require(owner.equals(r.getClassName()),"Unexpected method owner");
        final ClassWriter w=new ClassWriter(r,ClassWriter.COMPUTE_MAXS);
        final int[] exact={0};
        r.accept(new ClassVisitor(ASM5,w){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] ex){
                MethodVisitor dest=super.visitMethod(access,name,desc,sig,ex);
                if(!methodName.equals(name)||!"()V".equals(desc))return dest;
                require((access&ACC_PUBLIC)!=0 &&
                        (access&(ACC_STATIC|ACC_ABSTRACT|ACC_NATIVE))==0,
                        "Unexpected input method flags");
                exact[0]++;
                return new MethodVisitor(ASM5,dest){
                    @Override public void visitCode(){
                        super.visitCode();
                        Label vanilla=new Label();
                        // Gateway is an artificial fixture flag, not an
                        // authenticated game-thread ownership receipt.
                        super.visitMethodInsn(INVOKESTATIC,GATE,PREDICATE,
                                              "()Z",false);
                        super.visitJumpInsn(IFEQ,vanilla);
                        super.visitInsn(RETURN);
                        super.visitLabel(vanilla);
                        super.visitFrame(F_SAME,0,null,0,null);
                    }
                };
            }
        },0);
        require(exact[0]==1,"Missing or duplicated input entry point");
        return w.toByteArray();
    }
    public static byte[] inMemoryOriginal(byte[] raw){
        require(raw!=null,"Missing original Minecraft bytecode");
        R163InputControlGraph.pinned(raw);
        final byte[] unmodified=raw.clone();
        byte[] result=insert(raw,"ave","s");
        verifyOriginal(raw,result);
        require(Arrays.equals(raw,unmodified),"Vanilla input source was modified");
        return result;
    }
    public static byte[] inMemoryGeneratedFixture(byte[] raw) {
        require(raw!=null && raw.length>=32 && raw.length<65536,
                "Unbounded generated fixture");
        return insert(raw,"dev/slinkywinky/attach/R164TinyInputTarget","run");
    }
    /**
     * Verify the entry-cut shape and every source-matched input callsite.
     * Does NOT prove that a real game can load, dispatch, undo or restore it.
     */
    public static void verifyOriginal(byte[] original,byte[] candidate) {
        R163InputControlGraph.Report before=R163InputControlGraph.pinned(original);
        require(candidate!=null && candidate.length>original.length &&
                candidate.length<1000000,"Invalid offline candidate");
        final ClassReader r=new ClassReader(candidate);
        require("ave".equals(r.getClassName()),"Candidate changed class owner");
        final int[] methods={0},step={0},calls={0},jumps={0};
        final boolean[] gateLabelHit={false};
        final Label[] vanillaTarget={null};
        r.accept(new ClassVisitor(ASM5){
            @Override public MethodVisitor visitMethod(int flags,String name,
                    String desc,String sig,String[] exceptions) {
                if(!"s".equals(name)||!"()V".equals(desc))return null;
                methods[0]++;
                return new MethodVisitor(ASM5){
                    private void record(int opcode,String calledOwner,
                                        String called,String descriptor){
                        if(step[0]==0) {
                            require(opcode==INVOKESTATIC && GATE.equals(calledOwner) &&
                                PREDICATE.equals(called) &&
                                "()Z".equals(descriptor),
                                "First method operation is not the test-only gate");
                            calls[0]++;
                            step[0]=1;
                        }else if(step[0]==2) {
                            require(opcode==RETURN,
                                    "Modal path does not immediately exit method");
                            step[0]=3;
                        }else {
                            require(step[0]>=4,
                                    "Instructions before quarantine jump target");
                        }
                    }
                    @Override public void visitMethodInsn(int op,String owner,
                            String name,String desc,boolean itf){
                        record(op,owner,name,desc);
                        if(step[0]>=4)calls[0]++;
                    }
                    @Override public void visitJumpInsn(int op,Label dest) {
                        if(step[0]==1) {
                            require(op==IFEQ,
                                    "Gate must preserve vanilla on false");
                            vanillaTarget[0]=dest;step[0]=2;jumps[0]++;
                        }else {
                            require(step[0]>=4,
                                    "Jump before quarantine return");
                            jumps[0]++;
                        }
                    }
                    @Override public void visitInsn(int op) {
                        record(op,"","","");
                    }
                    @Override public void visitLabel(Label label){
                        if(step[0]==3 && label==vanillaTarget[0]) {
                            gateLabelHit[0]=true;step[0]=4;
                        }
                    }
                    @Override public void visitVarInsn(int op,int n){
                        require(step[0]>=4,"Unexpected pre-gate var");
                    }
                    @Override public void visitFieldInsn(int op,String owner,
                            String name,String desc) {
                        require(step[0]>=4,"Unexpected pre-gate field");
                    }
                    @Override public void visitTypeInsn(int op,String type){
                        require(step[0]>=4,"Unexpected pre-gate type");
                    }
                    @Override public void visitIntInsn(int op,int value){
                        require(step[0]>=4,"Unexpected pre-gate int");
                    }
                    @Override public void visitLdcInsn(Object value){
                        require(step[0]>=4,"Unexpected pre-gate constant");
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        require(methods[0]==1 && step[0]==4 &&
                gateLabelHit[0],"Invalid modal entry-cut prefix");
        R162InputOrderLedger.Report expected=R162InputOrderLedger.pinned(original);
        R162InputOrderLedger.Report actual=R162InputOrderLedger.inspect(candidate);
        require(actual.methodCalls==expected.methodCalls+1 &&
                actual.jumps==expected.jumps+1 &&
                actual.handlers==expected.handlers &&
                actual.sites.size()==expected.sites.size() &&
                actual.sites.size()==58,
                "Offline cut lost or added original input consumer");
        for(int i=0;i<expected.sites.size();i++){
            R162InputOrderLedger.Site x=expected.sites.get(i);
            R162InputOrderLedger.Site y=actual.sites.get(i);
            require(y.callOrdinal==x.callOrdinal+1 &&
                    x.family.equals(y.family) &&
                    x.phase.equals(y.phase) &&
                    x.signature.equals(y.signature),
                    "Input path changed after modal entry cut: "+i);
        }
        R163InputControlGraph.Report graph=R163InputControlGraph.inspect(candidate);
        require(graph.instructions==before.instructions+3 &&
                graph.jumps==before.jumps+1 &&
                graph.handlers==before.handlers &&
                graph.exceptionalEdges==before.exceptionalEdges &&
                graph.sourceSites==before.sourceSites &&
                graph.reachableSources==before.reachableSources,
                "Unexpected modal entry-cut control flow shape");
    }
}
