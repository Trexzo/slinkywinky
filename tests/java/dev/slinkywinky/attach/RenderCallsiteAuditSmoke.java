package dev.slinkywinky.attach;

import java.util.HashMap;
import java.util.Map;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** R64 offline test uses generated classfile fixtures; no commercial game JAR. */
public final class RenderCallsiteAuditSmoke implements Opcodes {
    private static void check(boolean ok,String why) {
        if(!ok)throw new AssertionError(why);
    }
    static byte[] renderer(String name,boolean publicMethod) {
        ClassWriter w=new ClassWriter(0);
        w.visit(V1_8,ACC_PUBLIC,name,null,"java/lang/Object",null);
        MethodVisitor method=w.visitMethod(publicMethod?ACC_PUBLIC:ACC_PRIVATE,
            "a","(FJ)V",null,null);
        method.visitCode();method.visitInsn(RETURN);method.visitMaxs(0,4);method.visitEnd();
        w.visitEnd();
        return w.toByteArray();
    }
    static byte[] game(boolean second,boolean hasField,boolean staticField) {
        ClassWriter w=new ClassWriter(0);
        w.visit(V1_8,ACC_PUBLIC,"ave",null,"java/lang/Object",null);
        if(hasField)w.visitField(ACC_PUBLIC|(staticField?ACC_STATIC:0),
            "renderer","Labc;",null,null).visitEnd();
        if(second)w.visitField(ACC_PUBLIC,"other","Ldef;",null,null).visitEnd();
        MethodVisitor m=w.visitMethod(ACC_PUBLIC,"q","()V",null,null);
        m.visitCode();
        m.visitVarInsn(ALOAD,0);
        m.visitFieldInsn(GETFIELD,"ave","renderer","Labc;");
        m.visitInsn(FCONST_0);m.visitInsn(LCONST_0);
        m.visitMethodInsn(INVOKEVIRTUAL,"abc","a","(FJ)V",false);
        if(second){
            m.visitVarInsn(ALOAD,0);m.visitFieldInsn(GETFIELD,"ave","other","Ldef;");
            m.visitInsn(FCONST_0);m.visitInsn(LCONST_0);
            m.visitMethodInsn(INVOKEVIRTUAL,"def","a","(FJ)V",false);
        }
        m.visitInsn(RETURN);m.visitMaxs(4,1);m.visitEnd();w.visitEnd();
        return w.toByteArray();
    }
    static RenderCallsiteAudit.ClassBytes provider(final Map<String,byte[]> map) {
        return new RenderCallsiteAudit.ClassBytes() {
            @Override public byte[] get(String owner) {return map.get(owner);}
        };
    }
    static void denied(byte[] source,Map<String,byte[]> classes) throws Exception {
        try {
            RenderCallsiteAudit.inspect(source,provider(classes));
            throw new AssertionError("Ambiguous/unverified renderer admitted");
        } catch(IllegalStateException expected) {
            check(expected.getMessage().contains("not unique"),"Unexpected denial");
        }
    }
    public static void main(String[] args) throws Exception {
        Map<String,byte[]> classes=new HashMap<String,byte[]>();
        classes.put("abc",renderer("abc",true));
        RenderCallsiteAudit.Candidate result=RenderCallsiteAudit.inspect(
            game(false,true,false),provider(classes));
        check(result.renderer.equals("abc"),"Wrong renderer class");
        check(result.caller.equals("q"),"Wrong Minecraft method");
        check(result.method.equals("a"),"Wrong render method");
        check(result.toString().contains("descriptor=(FJ)V"),"Render descriptor missing");
        denied(game(false,false,false),classes);
        denied(game(false,true,true),classes);
        Map<String,byte[]> missing=new HashMap<String,byte[]>();
        denied(game(false,true,false),missing);
        Map<String,byte[]> privateTarget=new HashMap<String,byte[]>();
        privateTarget.put("abc",renderer("abc",false));
        denied(game(false,true,false),privateTarget);
        classes.put("def",renderer("def",true));
        denied(game(true,true,false),classes);
        boolean invalidClassDenied=false;
        try {RenderCallsiteAudit.inspect(renderer("abc",true),provider(classes));}
        catch(IllegalArgumentException expected){invalidClassDenied=true;}
        check(invalidClassDenied,"Wrong root class accepted");
        check(RenderCallsiteAudit.decodeHex("3870888a").length==4,"SHA1 parser");
        boolean invalidHexDenied=false;
        try {RenderCallsiteAudit.decodeHex("xxx");}
        catch(IllegalArgumentException expected){invalidHexDenied=true;}
        check(invalidHexDenied,"Bad hash accepted");
        System.out.println("R64_RENDER_CALLSITE_NEGATIVE_CONTROLS_PASS");
    }
    private RenderCallsiteAuditSmoke(){}
}
