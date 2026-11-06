package dev.slinkywinky.attach;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.Handle;
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R109: TEST-ONLY, source-pinned, memory-only keyboard loop callsite rewrite.
 * The deliberately unresolved marker is never defined/loaded in Minecraft,
 * is not part of the injectable agent, and the rewritten bytes are not saved.
 */
public final class OriginalKeyboardLoopWeaveAudit implements Opcodes {
    private static final String GAME_CLASS_SHA256=
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82";
    private static final String KEYBOARD="org/lwjgl/input/Keyboard";
    private static final String MARKER="dev/slinkywinky/r109/OfflineOnlyKeyboardOwner";
    private static final String NEXT="next";
    private static final String SIGNATURE="()Z";
    private static final String ORIGINAL_CALL="M:184:"+KEYBOARD+":"+NEXT+":"+SIGNATURE+":false";
    private static final String REPLACED_CALL="M:184:"+MARKER+":"+NEXT+":"+SIGNATURE+":false";

    private static void require(boolean ok,String reason) {
        if(!ok)throw new IllegalArgumentException(reason);
    }
    // Method-scoped opcode trace covers changes beyond the named invocation:
    // the rewrite may only substitute exactly one same-signature callsite.
    static List<String> trace(byte[] bytes) {
        final List<String> tokens=new ArrayList<String>();
        new ClassReader(bytes).accept(new ClassVisitor(ASM5) {
            @Override public MethodVisitor visitMethod(int access,String name,
                   String descriptor,String sig,String[] exceptions) {
                tokens.add("METHOD:"+access+":"+name+":"+descriptor);
                return new MethodVisitor(ASM5) {
                    @Override public void visitInsn(int op){tokens.add("I:"+op);}
                    @Override public void visitIntInsn(int op,int n){
                        tokens.add("N:"+op+":"+n);
                    }
                    @Override public void visitVarInsn(int op,int n){
                        tokens.add("V:"+op+":"+n);
                    }
                    @Override public void visitTypeInsn(int op,String name){
                        tokens.add("T:"+op+":"+name);
                    }
                    @Override public void visitFieldInsn(int op,String o,String n,String d){
                        tokens.add("F:"+op+":"+o+":"+n+":"+d);
                    }
                    @Override public void visitMethodInsn(int op,String o,String n,
                            String d,boolean iface) {
                        tokens.add("M:"+op+":"+o+":"+n+":"+d+":"+iface);
                    }
                    @Override public void visitInvokeDynamicInsn(String name,
                            String descriptor,Handle bootstrap,Object... args) {
                        tokens.add("D:"+name+":"+descriptor+":"+bootstrap+":"+
                                   java.util.Arrays.deepToString(args));
                    }
                    @Override public void visitJumpInsn(int op,Label label){
                        tokens.add("J:"+op);
                    }
                    @Override public void visitLdcInsn(Object value){
                        tokens.add("L:"+String.valueOf(value));
                    }
                    @Override public void visitIincInsn(int var,int increment){
                        tokens.add("C:"+var+":"+increment);
                    }
                    @Override public void visitMultiANewArrayInsn(String desc,int dims){
                        tokens.add("A:"+desc+":"+dims);
                    }
                    @Override public void visitTableSwitchInsn(int low,int high,
                            Label dflt,Label... labels) {
                        tokens.add("S:"+low+":"+high+":"+labels.length);
                    }
                    @Override public void visitLookupSwitchInsn(Label dflt,
                            int[] keys,Label[] labels) {
                        tokens.add("K:"+java.util.Arrays.toString(keys));
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        return tokens;
    }
    static byte[] weave(byte[] original,String expectedSha256) throws Exception {
        require(original!=null && original.length>32 && original.length<=1000000,
                "Unrecognized class bytes");
        require(RenderCallsiteAudit.sha256Hex(original).equals(expectedSha256),
                "Expected exact SHA-256 for this bytecode version");
        // Fail closed unless R107 independently admits one vanilla input owner
        // with the exact key event and normal gameplay dispatch shape.
        require(KeyboardInputCallsiteAudit.inspect(original).contains(
                "owner=ave.s()V next=1 eventKey=2 eventState=3"),
                "R107 source-owner contract refused");
        final List<String> before=trace(original);
        require(java.util.Collections.frequency(before,ORIGINAL_CALL)==1,
                "Expected one original keyboard queue call");
        require(!before.contains(REPLACED_CALL),"Already instrumented");
        final int[] changedCount={0};
        ClassReader reader=new ClassReader(original);
        ClassWriter writer=new ClassWriter(reader,ClassWriter.COMPUTE_MAXS);
        reader.accept(new ClassVisitor(ASM5,writer) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] exceptions) {
                MethodVisitor target=super.visitMethod(access,name,desc,sig,exceptions);
                if(!"s".equals(name)||!"()V".equals(desc))return target;
                return new MethodVisitor(ASM5,target) {
                    @Override public void visitMethodInsn(int op,String owner,
                            String method,String descriptor,boolean itf) {
                        if(op==INVOKESTATIC && KEYBOARD.equals(owner) &&
                           NEXT.equals(method) && SIGNATURE.equals(descriptor) && !itf) {
                            changedCount[0]++;
                            super.visitMethodInsn(op,MARKER,method,descriptor,false);
                        } else super.visitMethodInsn(op,owner,method,descriptor,itf);
                    }
                };
            }
        },0);
        require(changedCount[0]==1,"Original input callsite was not unique");
        byte[] amended=writer.toByteArray();
        List<String> expectedTrace=new ArrayList<String>(before);
        int callsite=expectedTrace.indexOf(ORIGINAL_CALL);
        require(callsite>=0,"Expected Keyboard.next() in original trace");
        expectedTrace.set(callsite,REPLACED_CALL);
        require(expectedTrace.equals(trace(amended)),
                "Rewrite changed instructions outside exact keyboard callsite");
        require(RenderCallsiteAudit.sha256Hex(original).equals(expectedSha256),
                "Original class mutated by test");
        require(java.util.Collections.frequency(trace(amended),REPLACED_CALL)==1,
                "Missing or duplicated test-only marker");
        return amended; // only in-memory, never executed or saved
    }
    public static void main(String[] args)throws Exception {
        require(args.length==1,"Expected exact original 1.8.9 JAR path");
        Path jar=Paths.get(args[0]);
        require(jar.isAbsolute() && Files.isRegularFile(jar),
                "Missing absolute original client path");
        // Validates original Mojang JAR SHA-1 and size before any transformation.
        RenderCallsiteAudit.main(args);
        byte[] original;
        try(ZipFile zip=new ZipFile(jar.toFile())) {
            original=RenderCallsiteAudit.entry(zip,"ave.class");
        }
        byte[] changed=weave(original,GAME_CLASS_SHA256);
        require(changed.length>original.length,"Test marker did not change class");
        System.out.println("R109_ORIGINAL_189_KEYBOARD_NEXT_EXACT_ONE_CALLSITE_MEMORY_ONLY_PASS");
        System.out.println("R109_ALL_OTHER_METHOD_AND_INSTRUCTION_TRACES_UNCHANGED_PASS");
        System.out.println("R109_NO_GAME_PROCESS_NO_CLASS_REDEFINITION_NO_INPUT_CAPTURE");
    }
    private OriginalKeyboardLoopWeaveAudit(){}
}
