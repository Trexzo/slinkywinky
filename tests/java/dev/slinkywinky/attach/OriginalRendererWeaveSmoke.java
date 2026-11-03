package dev.slinkywinky.attach;

import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** R72 generated-bytecode controls; never uses or packages Mojang classes. */
public final class OriginalRendererWeaveSmoke implements Opcodes {
    private static void require(boolean yes, String why) {
        if(!yes) throw new AssertionError(why);
    }
    private static byte[] fixture(String owner, boolean staticMethod,
                                  String descriptor) {
        ClassWriter writer=new ClassWriter(0);
        writer.visit(V1_8,ACC_PUBLIC,owner,null,"java/lang/Object",null);
        MethodVisitor method=writer.visitMethod(
            ACC_PUBLIC|(staticMethod?ACC_STATIC:0),"a",descriptor,null,null);
        method.visitCode();
        method.visitInsn(RETURN);
        method.visitMaxs(0,staticMethod?3:4);
        method.visitEnd();
        writer.visitEnd();
        return writer.toByteArray();
    }
    private static void reject(byte[] bytes, String sha) throws Exception {
        try {
            OriginalRendererWeaveAudit.checkWeave(bytes,sha);
            throw new AssertionError("Unverified renderer admitted");
        } catch(IllegalArgumentException expected) {
            // The offline ASM admission is intentionally fail-closed.
        }
    }
    public static void main(String[] args) throws Exception {
        byte[] correct=fixture("bfk",false,"(FJ)V");
        String sha=RenderCallsiteAudit.sha256Hex(correct);
        OriginalRendererWeaveAudit.checkWeave(correct,sha);
        require(sha.equals(RenderCallsiteAudit.sha256Hex(correct)),
                "Original array mutated during memory-only preflight");
        reject(correct,"00");
        reject(fixture("wrong",false,"(FJ)V"),
            RenderCallsiteAudit.sha256Hex(fixture("wrong",false,"(FJ)V")));
        byte[] staticClass=fixture("bfk",true,"(FJ)V");
        reject(staticClass,RenderCallsiteAudit.sha256Hex(staticClass));
        byte[] wrongDescriptor=fixture("bfk",false,"(II)V");
        reject(wrongDescriptor,RenderCallsiteAudit.sha256Hex(wrongDescriptor));
        reject(new byte[]{1,2,3},"00");
        System.out.println("R72_OFFLINE_WEAVE_SYNTHETIC_NEGATIVE_CONTROLS_PASS");
        System.out.println("R72_NO_MINECRAFT_CLASS_DEFINITION_OR_ATTACHMENT");
    }
    private OriginalRendererWeaveSmoke() {}
}
