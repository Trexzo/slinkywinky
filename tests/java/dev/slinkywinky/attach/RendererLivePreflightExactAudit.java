package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RendererLivePreflight;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.zip.ZipFile;
import java.util.Arrays;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** R73 static compatibility proof using the R71 SHA1/size verified Mojang JAR. */
public final class RendererLivePreflightExactAudit {
    public static void main(String[] args) throws Exception {
        if(args.length!=1) throw new IllegalArgumentException(
            "Usage: RendererLivePreflightExactAudit ORIGINAL_1_8_9_JAR");
        Path jar=Paths.get(args[0]);
        if(!jar.isAbsolute() || !Files.isRegularFile(jar))
            throw new IllegalArgumentException("Expected absolute installed original JAR");
        // This repeats official JAR size/SHA1 and unique caller checks before
        // running the same SHA-pinned structural admission used for a live class.
        RenderCallsiteAudit.main(args);
        try(ZipFile original=new ZipFile(jar.toFile())) {
            final byte[] ave=RenderCallsiteAudit.entry(original,"ave.class");
            final byte[] bfk=RenderCallsiteAudit.entry(original,"bfk.class");
            RendererLivePreflight.checkPinnedOriginal(ave,bfk);

            // R90: simulate VM classfile reserialization in memory. The source
            // must be exact, while reordered constant-pool bytes may still
            // describe precisely the same executable methods.
            final ClassWriter rewritten=new ClassWriter(0);
            new ClassReader(bfk).accept(rewritten,0);
            final byte[] normalized=rewritten.toByteArray();
            final String bfkSha=RenderCallsiteAudit.sha256Hex(bfk);
            RendererLivePreflight.checkEquivalentClassBytes(bfk,normalized,bfkSha);
            RendererLivePreflight.checkEquivalentClassBytes(ave,ave,
                RenderCallsiteAudit.sha256Hex(ave));

            final ClassWriter modified=new ClassWriter(0);
            new ClassReader(bfk).accept(new ClassVisitor(Opcodes.ASM5,modified) {
                @Override public MethodVisitor visitMethod(int access,String name,
                        String desc,String signature,String[] exceptions) {
                    MethodVisitor downstream=super.visitMethod(
                        access,name,desc,signature,exceptions);
                    if(!"a".equals(name) || !"(FJ)V".equals(desc))
                        return downstream;
                    return new MethodVisitor(Opcodes.ASM5,downstream) {
                        @Override public void visitCode() {
                            super.visitCode();
                            super.visitInsn(Opcodes.NOP);
                        }
                    };
                }
            },0);
            final byte[] changed=modified.toByteArray();
            if(Arrays.equals(bfk,changed))
                throw new IllegalStateException("R90 mutation negative control inert");
            boolean changedRejected=false,sourceRejected=false;
            try {
                RendererLivePreflight.checkEquivalentClassBytes(bfk,changed,bfkSha);
            } catch(IllegalStateException expected) {
                changedRejected=true;
            }
            try {
                RendererLivePreflight.checkEquivalentClassBytes(changed,changed,bfkSha);
            } catch(IllegalStateException expected) {
                sourceRejected=true;
            }
            if(!changedRejected || !sourceRejected)
                throw new IllegalStateException("R90 runtime mutation or forged source admitted");
            System.out.println("R90_OFFLINE_VM_RESERIALIZE_ACCEPTED_MUTATED_CODE_DENIED_PASS");
        }
        System.out.println("R73_STATIC_ORIGINAL_RENDERER_ADMISSION_PASS");
        System.out.println("R73_STATIC_AUDIT_IS_NOT_LIVE_GAME_EVIDENCE");
    }
    private RendererLivePreflightExactAudit() {}
}
