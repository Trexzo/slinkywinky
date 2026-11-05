package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RendererFrameDiagnostic;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.Handle;
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** R75 offline-only exact original 1.8.9 renderer prefix integrity check. */
public final class OriginalRendererFrameDiagnosticAudit implements Opcodes {
    private static final String PREFIX_OWNER=
        "dev/slinkywinky/instrument/RendererFrameDiagnostic";
    private static final class Trace {
        String cls;
        int targets;
        List<String> methods=new ArrayList<String>();
        List<String> instructions=new ArrayList<String>();
    }
    private static Trace trace(byte[] raw) {
        final Trace out=new Trace();
        ClassReader reader=new ClassReader(raw);
        out.cls=reader.getClassName();
        reader.accept(new ClassVisitor(ASM5) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String signature,String[] exceptions) {
                out.methods.add(access+":"+name+desc);
                if(!"a".equals(name) || !"(FJ)V".equals(desc))return null;
                out.targets++;
                return new MethodVisitor(ASM5) {
                    private void add(String s){out.instructions.add(s);}
                    @Override public void visitInsn(int op){add("I:"+op);}
                    @Override public void visitIntInsn(int op,int x){add("N:"+op+":"+x);}
                    @Override public void visitVarInsn(int op,int x){add("V:"+op+":"+x);}
                    @Override public void visitTypeInsn(int op,String x){add("T:"+op+":"+x);}
                    @Override public void visitFieldInsn(int op,String o,String n,String d){
                        add("F:"+op+":"+o+":"+n+":"+d);
                    }
                    @Override public void visitMethodInsn(int op,String o,String n,
                            String d,boolean itf){
                        add("M:"+op+":"+o+":"+n+":"+d+":"+itf);
                    }
                    @Override public void visitInvokeDynamicInsn(String n,String d,
                            Handle b,Object... a){
                        add("D:"+n+":"+d+":"+b+":"+Arrays.deepToString(a));
                    }
                    @Override public void visitJumpInsn(int op,Label l){add("J:"+op);}
                    @Override public void visitLdcInsn(Object x){add("L:"+String.valueOf(x));}
                    @Override public void visitIincInsn(int v,int n){add("C:"+v+":"+n);}
                    @Override public void visitTableSwitchInsn(int a,int b,
                            Label def,Label... targets){
                        add("S:"+a+":"+b+":"+targets.length);
                    }
                    @Override public void visitLookupSwitchInsn(Label def,
                            int[] keys,Label[] targets){
                        add("K:"+Arrays.toString(keys));
                    }
                    @Override public void visitMultiANewArrayInsn(String d,int n){
                        add("A:"+d+":"+n);
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        return out;
    }
    public static void check(byte[] original) {
        Trace before=trace(original);
        byte[] changed=RendererFrameDiagnostic.weavePinnedOriginal(original);
        Trace after=trace(changed);
        List<String> expected=new ArrayList<String>(before.instructions);
        expected.add(0,"M:184:"+PREFIX_OWNER+":onRender:(FJ)V:false");
        expected.add(0,"V:22:2");
        expected.add(0,"V:23:1");
        if(!"bfk".equals(before.cls) || !"bfk".equals(after.cls) ||
            before.targets!=1 || after.targets!=1 ||
            !before.methods.equals(after.methods) ||
            !expected.equals(after.instructions))
            throw new IllegalStateException(
                "R75 bytecode prefix altered original renderer behavior");
    }
    /** Independent exact-original audit for R92's AFTER-scene callback. */
    public static void checkExit(byte[] original) {
        Trace before=trace(original);
        Trace after=trace(RendererFrameDiagnostic.weavePinnedOriginalAtExit(original));
        List<String> expected=new ArrayList<String>();
        int returns=0;
        for(String instruction:before.instructions) {
            if(("I:"+Opcodes.RETURN).equals(instruction)) {
                expected.add("V:"+Opcodes.FLOAD+":1");
                expected.add("V:"+Opcodes.LLOAD+":2");
                expected.add("M:"+Opcodes.INVOKESTATIC+":"+PREFIX_OWNER+
                             ":onRender:(FJ)V:false");
                returns++;
            }
            expected.add(instruction);
        }
        if(!"bfk".equals(before.cls) || !"bfk".equals(after.cls) ||
           before.targets!=1 || after.targets!=1 || returns<1 ||
           !before.methods.equals(after.methods) ||
           !expected.equals(after.instructions))
            throw new IllegalStateException(
                "R92 render-exit callback changed original renderer instructions");
    }

    public static void main(String[] args) throws Exception {
        if(args.length!=1)throw new IllegalArgumentException(
            "Expected original Mojang 1.8.9 JAR path");
        Path path=Paths.get(args[0]);
        if(!path.isAbsolute() || !Files.isRegularFile(path))
            throw new IllegalArgumentException("Missing exact original client");
        RenderCallsiteAudit.main(args);
        try(ZipFile zip=new ZipFile(path.toFile())){
            final byte[] original=RenderCallsiteAudit.entry(zip,"bfk.class");
            check(original);
            checkExit(original);
        }
        System.out.println("R75_PINNED_ORIGINAL_RENDER_CALLBACK_PREFIX_PASS");
        System.out.println("R92_PINNED_ORIGINAL_RENDER_CALLBACK_EXIT_PASS");
        System.out.println("R75_MEMORY_ONLY_NO_GAME_ATTACH_PASS");
    }
    private OriginalRendererFrameDiagnosticAudit(){}
}
