package dev.slinkywinky.attach;

import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** Synthetic-only R107 controls; NEVER starts or instruments Minecraft. */
public final class KeyboardInputCallsiteAuditSmoke implements Opcodes {
    private static void require(boolean test,String message) {
        if(!test)throw new AssertionError(message);
    }
    private static void call(MethodVisitor mv,String owner,String method,String desc) {
        if("avb".equals(owner)) {
            mv.visitInsn(ICONST_0);
            if("(IZ)V".equals(desc))mv.visitInsn(ICONST_0);
        } else if("axu".equals(owner))mv.visitInsn(ACONST_NULL);
        mv.visitMethodInsn("axu".equals(owner)?INVOKEVIRTUAL:INVOKESTATIC,
            owner,method,desc,false);
        if(desc.endsWith("Z") || desc.endsWith("I") || desc.endsWith("C"))
            mv.visitInsn(POP);
    }
    // variant: 0 valid, 1 no next, 2 duplicate next, 3 no post-dispatch,
    // 4 wrong method, 5 wrong class, 6 static, 7 missing event state,
    // 8 missing GUI, 9 duplicate before-dispatch.
    static byte[] fixture(int variant) {
        ClassWriter writer=new ClassWriter(0);
        writer.visit(V1_8,ACC_PUBLIC,""+(variant==5?"fake":"ave"),
            null,"java/lang/Object",null);
        MethodVisitor mv=writer.visitMethod(
            variant==6?ACC_PUBLIC|ACC_STATIC:ACC_PUBLIC,
            variant==4?"x":"s","()V",null,null);
        mv.visitCode();
        call(mv,"avb","a","(IZ)V");
        call(mv,"avb","a","(I)V");
        if(variant==9)call(mv,"avb","a","(IZ)V");
        if(variant!=1)call(mv,"org/lwjgl/input/Keyboard","next","()Z");
        if(variant==2)call(mv,"org/lwjgl/input/Keyboard","next","()Z");
        call(mv,"org/lwjgl/input/Keyboard","getEventKey","()I");
        call(mv,"org/lwjgl/input/Keyboard","getEventKey","()I");
        call(mv,"org/lwjgl/input/Keyboard","getEventCharacter","()C");
        call(mv,"org/lwjgl/input/Keyboard","getEventKeyState","()Z");
        call(mv,"org/lwjgl/input/Keyboard","getEventKeyState","()Z");
        if(variant!=7)
            call(mv,"org/lwjgl/input/Keyboard","getEventKeyState","()Z");
        if(variant!=3) {
            call(mv,"avb","a","(IZ)V");
            call(mv,"avb","a","(I)V");
        }
        if(variant!=8)call(mv,"axu","l","()V");
        mv.visitInsn(RETURN);
        mv.visitMaxs(4,variant==6?0:1);
        mv.visitEnd();
        writer.visitEnd();
        return writer.toByteArray();
    }
    public static void main(String[] args) {
        require(KeyboardInputCallsiteAudit.inspect(fixture(0)).contains(
            "gameBindingsAfter=2"),"Synthetic expected shape rejected");
        for(int variant=1;variant<=9;variant++) {
            boolean refused=false;
            try { KeyboardInputCallsiteAudit.inspect(fixture(variant)); }
            catch(RuntimeException expected) {refused=true;}
            require(refused,"Unsafe synthetic input owner admitted: "+variant);
        }
        boolean badBytesRefused=false;
        try {KeyboardInputCallsiteAudit.inspect(new byte[]{1,2,3});}
        catch(RuntimeException expected){badBytesRefused=true;}
        require(badBytesRefused,"Non-class bytes admitted");
        System.out.println("R107_KEYBOARD_EVENT_PATH_SYNTHETIC_NEGATIVE_CONTROLS_PASS");
        System.out.println("R107_NO_INJECTED_KEYBOARD_OR_MODAL_GAMEPLAY_INPUT");
    }
}
