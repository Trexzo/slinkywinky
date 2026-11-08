package dev.slinkywinky.attach;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R171 independent Java 8 negative controls: source-wide input consumers.
 * Original game is never loaded, modified, attached to or invoked.
 */
public final class R171VanillaJarInputSurfaceSmoke implements Opcodes {
    private static void check(boolean yes,String why){
        if(!yes)throw new AssertionError("R171 "+why);
    }
    private static byte[] fake(String name,boolean inputMethod) {
        ClassWriter writer=new ClassWriter(ClassWriter.COMPUTE_MAXS);
        writer.visit(V1_8,ACC_PUBLIC,name,null,"java/lang/Object",null);
        MethodVisitor m=writer.visitMethod(ACC_PUBLIC,
            inputMethod?"s":"a",inputMethod?"()V":"(FJ)V",null,null);
        m.visitCode();
        if(inputMethod) {
            m.visitMethodInsn(INVOKESTATIC,"org/lwjgl/input/Mouse",
                "next","()Z",false);
            m.visitInsn(POP);
        }else{
            m.visitInsn(ICONST_0);
            m.visitMethodInsn(INVOKESTATIC,"org/lwjgl/input/Keyboard",
                "isKeyDown","(I)Z",false);
            m.visitInsn(POP);
            m.visitInsn(ICONST_0);
            m.visitMethodInsn(INVOKESTATIC,"org/lwjgl/input/Mouse",
                "isButtonDown","(I)Z",false);
            m.visitInsn(POP);
        }
        m.visitInsn(RETURN);m.visitMaxs(0,0);m.visitEnd();
        writer.visitEnd();
        return writer.toByteArray();
    }
    /** Change one *outside* caller only, never original Minecraft ave. */
    private static byte[] sabotageMouseSource(byte[] raw) {
        ClassReader reader=new ClassReader(raw);
        ClassWriter writer=new ClassWriter(reader,0);
        final int[] touched={0};
        reader.accept(new ClassVisitor(ASM5,writer) {
            @Override public MethodVisitor visitMethod(int flags,String name,
                    String desc,String sig,String[] exceptions){
                MethodVisitor out=super.visitMethod(flags,name,desc,sig,exceptions);
                final boolean target="a".equals(name) && "(FJ)V".equals(desc);
                return new MethodVisitor(ASM5,out) {
                    @Override public void visitMethodInsn(int opcode,String owner,
                            String method,String desc,boolean iface){
                        if(target && touched[0]==0 && opcode==INVOKESTATIC &&
                           owner.equals("org/lwjgl/input/Mouse") &&
                           method.equals("isButtonDown") &&
                           desc.equals("(I)Z")) {
                            owner="dev/slinkywinky/attach/R171UnresolvedMouse";
                            touched[0]++;
                        }
                        super.visitMethodInsn(opcode,owner,method,desc,iface);
                    }
                };
            }
        },0);
        check(touched[0]==1,"Outside renderer mouse callsite not unique");
        return writer.toByteArray();
    }
    private static void synthetic() {
        final byte[] ave=fake("ave",true);
        final byte[] bfk=fake("bfk",false);
        R171VanillaJarInputSurface.Report full=
            R171VanillaJarInputSurface.inspect(Arrays.asList(ave,bfk));
        check(full.classesInspected==2 && full.sites.size()==3 &&
              full.outsideMainInput()==2 &&
              full.directOutsideMainInput()==2,
              "Whole-method input gate forgot external render calls");
        R171VanillaJarInputSurface.Report swapped=
            R171VanillaJarInputSurface.inspect(Arrays.asList(bfk,ave));
        check(full.fingerprint.equals(swapped.fingerprint),
              "Scan digest depends on ZIP order");
        check(!full.wholeMethodCutIsSufficient(),
              "Unverified modal hook authorized by static scan");
        byte[] tampered=sabotageMouseSource(bfk);
        R171VanillaJarInputSurface.Report missing=
            R171VanillaJarInputSurface.inspect(Arrays.asList(ave,tampered));
        check(missing.outsideMainInput()==1 &&
              missing.sites.size()==2 &&
              !missing.fingerprint.equals(full.fingerprint),
              "Outside-method source removal not detected");
        check(R171VanillaJarInputSurface.scanClass(ave).size()==1,
              "Standalone original method changed");
        check(R171VanillaJarInputSurface.scanClass(bfk).size()==2,
              "Original render mock source mutated");
        check(Arrays.equals(bfk,fake("bfk",false)),
              "Mutation modified underlying fixture bytes");
        boolean refused=false;
        try{
            R171VanillaJarInputSurface.inspectPinnedJar("nonexistent.jar");
        }catch(Exception expected){refused=true;}
        check(refused,"Non-original client JAR accepted");
        System.out.println("R171_EXTERNAL_RENDER_CONSUMER_NEGATIVE_CONTROL_PASS");
        System.out.println("R171_CROSS_CLASS_SCAN_ORDER_AND_UNCHANGED_AVE_PASS");
        System.out.println("R171_JAVA8_TEST_ONLY_NO_GAME_INPUT_HOOK");
    }

    private static void original(String jar)throws Exception {
        check(Files.isRegularFile(Paths.get(jar)),"Original Mojang JAR missing");
        final R171VanillaJarInputSurface.Report global=
            R171VanillaJarInputSurface.inspectPinnedJar(jar);
        check(global.classesInspected==2507 && global.sites.size()==146 &&
              global.outsideMainInput()==96 &&
              global.directOutsideMainInput()==86 &&
              global.count("MOUSE")==42 &&
              global.count("KEYBOARD")==74 &&
              global.count("KEYBIND_POLL")==26 &&
              global.count("KEYBIND_MUTATION")==4 &&
              global.fingerprint.equals(
                "b029538b90e0e23947c9224779028679f0faced032f5a469b07f69b8859dd11e"),
                "Exact vanilla game-wide input surface drifted");
        boolean renderMouse=false,otherKeyboard=false,guiMouse=false;
        for(R171VanillaJarInputSurface.Site s:global.sites){
            if(s.caller().equals("bfk.a(FJ)V") &&
               s.targetMethod.equals("isButtonDown") &&
               s.family.equals("MOUSE"))renderMouse=true;
            if(s.caller().equals("avh.a(Lavb;)Z") &&
               s.targetMethod.equals("isKeyDown") &&
               s.family.equals("KEYBOARD"))otherKeyboard=true;
            if(s.caller().equals("awi.p()V") &&
               s.targetMethod.equals("getEventButton") &&
               s.family.equals("MOUSE"))guiMouse=true;
        }
        check(renderMouse && otherKeyboard && guiMouse,
              "Renderer, keybind and UI external input sites not found");
        byte[] ave,render;
        try(ZipFile z=new ZipFile(jar)){
            ave=RenderCallsiteAudit.entry(z,"ave.class");
            render=RenderCallsiteAudit.entry(z,"bfk.class");
        }
        final byte[] unchanged=render.clone();
        final String originalSha=R171VanillaJarInputSurface.sha("SHA-256",render);
        List<byte[]> sources=Arrays.asList(ave,render);
        R171VanillaJarInputSurface.Report before=
            R171VanillaJarInputSurface.inspect(sources);
        byte[] mutated=sabotageMouseSource(render);
        R171VanillaJarInputSurface.Report after=
            R171VanillaJarInputSurface.inspect(Arrays.asList(ave,mutated));
        check(after.sites.size()==before.sites.size()-1 &&
              after.outsideMainInput()==before.outsideMainInput()-1 &&
              !after.fingerprint.equals(before.fingerprint),
              "Render method mutation escaped whole-client source audit");
        check(Arrays.equals(render,unchanged) &&
              originalSha.equals(R171VanillaJarInputSurface.sha("SHA-256",render)),
              "Real Mojang renderer mutated in memory");
        R163InputControlGraph.pinned(ave);
        check(!global.wholeMethodCutIsSufficient(),
              "Global input audit improperly certified entry cut");
        System.out.println("R171_PINNED_VANILLA_189_GLOBAL_2507_CLASSES_PASS");
        System.out.println("R171_PINNED_VANILLA_189_GLOBAL_SURFACE_SHA256 "+
            global.fingerprint);
        System.out.println("R171_ORIGINAL_RENDERER_EXTERNAL_INPUT_MUTATION_DETECTED");
        System.out.println("R171_AVE_METHOD_STATIC_AUDIT_UNCHANGED_BUT_UNSAFE");
        System.out.println("R171_NO_LIVE_HOOK_OR_JVM_REDEFINITION");
    }
    public static void main(String[] args)throws Exception {
        check(args.length<=1,"Unexpected original-JAR arguments");
        synthetic();
        if(args.length==1)original(args[0]);
    }
}
