package dev.slinkywinky.attach;

import java.util.Arrays;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.FieldVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R173 exact-original Minecraft 1.8.9 static GUI source inspection.
 * Never exports, transforms, loads or writes Minecraft into a running JVM.
 */
public final class R173OriginalGuiFieldAudit implements Opcodes {
    private static final String PINNED_AVE_SHA256=
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82";
    private R173OriginalGuiFieldAudit() {}
    private static void require(boolean okay,String why) {
        if(!okay)throw new AssertionError("R173 "+why);
    }
    static boolean provenGetter(byte[] input) {
        if(input==null || input.length<32 || input.length>1000000)return false;
        final int[] screen={0},singleton={0},getter={0},readS={0},
                    returnValue={0},unsafe={0};
        ClassReader reader=new ClassReader(input);
        if(!"ave".equals(reader.getClassName()))return false;
        reader.accept(new ClassVisitor(ASM5) {
            @Override public FieldVisitor visitField(int flags,String name,
                    String desc,String signature,Object value) {
                if("m".equals(name) && "Laxu;".equals(desc) &&
                   (flags&ACC_PUBLIC)!=0 && (flags&ACC_STATIC)==0)
                    screen[0]++;
                if("S".equals(name) && "Lave;".equals(desc) &&
                   (flags&ACC_STATIC)!=0)singleton[0]++;
                return null;
            }
            @Override public MethodVisitor visitMethod(int flags,String name,
                    String desc,String signature,String[] exceptions) {
                if(!"A".equals(name) || !"()Lave;".equals(desc))return null;
                if((flags&ACC_STATIC)==0 || (flags&ACC_PUBLIC)==0 ||
                   (flags&(ACC_NATIVE|ACC_ABSTRACT))!=0)unsafe[0]++;
                getter[0]++;
                return new MethodVisitor(ASM5) {
                    @Override public void visitFieldInsn(int opcode,String owner,
                             String field,String signature) {
                        if(opcode==GETSTATIC && "ave".equals(owner) &&
                           "S".equals(field) && "Lave;".equals(signature))
                            readS[0]++;
                        else unsafe[0]++;
                    }
                    @Override public void visitInsn(int opcode){
                        if(opcode==ARETURN)returnValue[0]++;
                        else unsafe[0]++;
                    }
                    @Override public void visitVarInsn(int opcode,int var){
                        unsafe[0]++;
                    }
                    @Override public void visitMethodInsn(int opcode,
                            String owner,String method,String desc,boolean interfaceMethod){
                        unsafe[0]++;
                    }
                    @Override public void visitJumpInsn(int opcode,
                            org.objectweb.asm.Label label){unsafe[0]++;}
                    @Override public void visitLdcInsn(Object constant){unsafe[0]++;}
                    @Override public void visitTypeInsn(int opcode,String type){
                        unsafe[0]++;
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        return screen[0]==1 && singleton[0]==1 && getter[0]==1 &&
               readS[0]==1 && returnValue[0]==1 && unsafe[0]==0;
    }

    static byte[] corruptGetterInMemory(byte[] raw) {
        final int[] replaced={0};
        ClassReader reader=new ClassReader(raw);
        ClassWriter writer=new ClassWriter(reader,0);
        reader.accept(new ClassVisitor(ASM5,writer) {
            @Override public MethodVisitor visitMethod(int flags,String name,
                    String desc,String signature,String[] exceptions) {
                final MethodVisitor sink=super.visitMethod(flags,name,desc,signature,exceptions);
                if(!"A".equals(name) || !"()Lave;".equals(desc))return sink;
                return new MethodVisitor(ASM5,sink) {
                    @Override public void visitFieldInsn(int opcode,String owner,
                             String field,String desc) {
                        if(opcode==GETSTATIC && "ave".equals(owner) &&
                           "S".equals(field) && "Lave;".equals(desc)) {
                            field="OTHER_SINGLETON";
                            replaced[0]++;
                        }
                        super.visitFieldInsn(opcode,owner,field,desc);
                    }
                };
            }
        },0);
        require(replaced[0]==1,"Original singleton read not unique");
        return writer.toByteArray();
    }

    public static void main(String[] args)throws Exception {
        require(args.length==1,"Exact original Mojang JAR argument required");
        final R171VanillaJarInputSurface.Report original=
            R171VanillaJarInputSurface.inspectPinnedJar(args[0]);
        require(original.classesInspected==2507,
            "Unpinned original Minecraft JAR accepted");
        byte[] raw;
        try(ZipFile z=new ZipFile(args[0])) {
            raw=RenderCallsiteAudit.entry(z,"ave.class");
        }
        require(PINNED_AVE_SHA256.equals(
            R171VanillaJarInputSurface.sha("SHA-256",raw)),
            "Original ave source fingerprint changed");
        require(provenGetter(raw),
            "Original game singleton or public axu screen source shape changed");
        byte[] untouched=raw.clone();
        byte[] changed=corruptGetterInMemory(raw);
        require(!provenGetter(changed),
            "Same-count alternate singleton read went undetected");
        require(provenGetter(raw) && Arrays.equals(raw,untouched),
            "Original client bytes changed during test");
        System.out.println("R173_MOJANG_189_AVE_SINGLETON_GUI_FIELD_EXACT_SOURCE_PASS");
        System.out.println("R173_ALTERNATE_SINGLETON_NEGATIVE_CONTROL_PASS");
        System.out.println("R173_OFFLINE_ONLY_ORIGINAL_BYTES_UNCHANGED");
    }
}
