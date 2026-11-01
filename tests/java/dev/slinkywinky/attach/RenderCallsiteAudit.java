package dev.slinkywinky.attach;

import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R64. Exact 1.8.9 ORIGINAL CLIENT JAR only. Reports a structural candidate
 * for the deobfuscated EntityRenderer.updateCameraAndRender(float,long) call.
 * This is an OFFLINE audit: no instrumentation, JNI or Minecraft launch.
 */
public final class RenderCallsiteAudit {
    private static final byte[] EXPECTED_SHA1 = decodeHex(
        "3870888a6c3d349d3771a3e9d16c9bf5e076b908");
    private static final long EXPECTED_SIZE = 8461484L;
    static final String GAME_CLASS = "ave";
    static final String RENDER_DESCRIPTOR = "(FJ)V";

    interface ClassBytes {
        byte[] get(String internalName) throws Exception;
    }
    static final class Candidate {
        final String caller;
        final String renderer;
        final String method;
        Candidate(String caller, String renderer, String method) {
            this.caller=caller;this.renderer=renderer;this.method=method;
        }
        @Override public String toString() {
            return "caller=ave."+caller+" renderer="+renderer+
                   " method="+method+" descriptor="+RENDER_DESCRIPTOR;
        }
    }

    static Candidate inspect(final byte[] minecraftClass, final ClassBytes classes)
        throws Exception {
        if(minecraftClass==null || minecraftClass.length>1000000)
            throw new IllegalArgumentException("Missing or oversized original Minecraft class");
        final Set<String> rendererFields=new HashSet<String>();
        final List<Candidate> calls=new ArrayList<Candidate>();
        ClassReader reader=new ClassReader(minecraftClass);
        if(!GAME_CLASS.equals(reader.getClassName()))
            throw new IllegalArgumentException("Expected original 1.8.9 class ave");
        reader.accept(new ClassVisitor(Opcodes.ASM5) {
            @Override public org.objectweb.asm.FieldVisitor visitField(
                    int access,String name,String descriptor,String signature,Object value) {
                if((access & Opcodes.ACC_STATIC)==0 &&
                   descriptor.startsWith("L") && descriptor.endsWith(";"))
                    rendererFields.add(descriptor.substring(1,descriptor.length()-1));
                return null;
            }
            @Override public MethodVisitor visitMethod(
                    int access,final String name,String desc,String sig,String[] exceptions) {
                if((access & (Opcodes.ACC_STATIC|Opcodes.ACC_ABSTRACT|Opcodes.ACC_NATIVE))!=0)
                    return null;
                return new MethodVisitor(Opcodes.ASM5) {
                    @Override public void visitMethodInsn(int opcode,String owner,
                            String method,String descriptor,boolean itf) {
                        if(opcode==Opcodes.INVOKEVIRTUAL && !itf &&
                           RENDER_DESCRIPTOR.equals(descriptor))
                            calls.add(new Candidate(name,owner,method));
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        final Map<String,Candidate> valid=new HashMap<String,Candidate>();
        for(Candidate call:calls) {
            // Renderer must be one of ave's own typed instance fields, not a
            // matching signature on an unrelated or static utility class.
            if(!rendererFields.contains(call.renderer))continue;
            byte[] target=classes.get(call.renderer);
            if(target==null || target.length>1000000)continue;
            final int[] found={0};
            final String wanted=call.method;
            ClassReader targetReader=new ClassReader(target);
            if(!call.renderer.equals(targetReader.getClassName()))continue;
            targetReader.accept(new ClassVisitor(Opcodes.ASM5) {
                @Override public MethodVisitor visitMethod(int access,String method,
                        String desc,String sig,String[] exceptions) {
                    if(wanted.equals(method) && RENDER_DESCRIPTOR.equals(desc) &&
                       (access & Opcodes.ACC_PUBLIC)!=0 &&
                       (access & (Opcodes.ACC_STATIC|Opcodes.ACC_ABSTRACT|
                                  Opcodes.ACC_NATIVE))==0)
                        found[0]++;
                    return null;
                }
            },ClassReader.SKIP_CODE|ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
            if(found[0]==1)
                valid.put(call.renderer+"#"+call.method,call);
        }
        if(valid.size()!=1)
            throw new IllegalStateException("Render callsite not unique; admitted="+
                valid.size()+" candidates="+calls.size());
        return valid.values().iterator().next();
    }
    static byte[] decodeHex(String value) {
        if((value.length() & 1)!=0) throw new IllegalArgumentException("bad hex");
        byte[] out=new byte[value.length()/2];
        for(int i=0;i<out.length;i++) {
            int hi=Character.digit(value.charAt(2*i),16);
            int lo=Character.digit(value.charAt(2*i+1),16);
            if(hi<0 || lo<0)throw new IllegalArgumentException("bad hex");
            out[i]=(byte)((hi<<4)|lo);
        }
        return out;
    }
    static byte[] entry(ZipFile zip,String path) throws Exception {
        ZipEntry member=zip.getEntry(path);
        if(member==null || member.getSize()<0 || member.getSize()>1000000)
            throw new IllegalArgumentException("Missing or oversized class: "+path);
        InputStream stream=zip.getInputStream(member);
        try {
            ByteArrayOutputStream out=new ByteArrayOutputStream();
            byte[] data=new byte[16384];int count;
            while((count=stream.read(data))!=-1) {
                out.write(data,0,count);
                if(out.size()>1000000)throw new IllegalArgumentException("class too large");
            }
            return out.toByteArray();
        } finally {stream.close();}
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=1) throw new IllegalArgumentException(
            "Usage: RenderCallsiteAudit ABSOLUTE_INSTALLED_ORIGINAL_1_8_9_JAR");
        Path path=Paths.get(args[0]);
        if(!path.isAbsolute() || !Files.isRegularFile(path) ||
           Files.size(path)!=EXPECTED_SIZE)
            throw new IllegalArgumentException("Refusing missing, non-absolute or wrong-size JAR");
        MessageDigest digest=MessageDigest.getInstance("SHA-1");
        InputStream file=Files.newInputStream(path);
        try {
            byte[] block=new byte[32768];int n;
            while((n=file.read(block))!=-1)digest.update(block,0,n);
        } finally {file.close();}
        if(!Arrays.equals(digest.digest(),EXPECTED_SHA1))
            throw new IllegalArgumentException("Refusing unverified original Mojang 1.8.9 JAR");
        ZipFile archive=new ZipFile(path.toFile());
        try {
            final ZipFile safe=archive;
            Candidate mapped=inspect(entry(archive,"ave.class"),new ClassBytes(){
                @Override public byte[] get(String owner)throws Exception {
                    if(owner.indexOf('/')>=0 || owner.indexOf('\\')>=0 ||
                       owner.indexOf('.')>=0 || owner.length()>64)return null;
                    ZipEntry e=safe.getEntry(owner+".class");
                    return e==null?null:entry(safe,owner+".class");
                }
            });
            System.out.println("R64_RENDER_CALLSITE_CANDIDATE "+mapped);
            System.out.println("R64_ORIGINAL_JAR_SHA1_VALIDATED");
            System.out.println("R64_NOTE=OFFLINE_CANDIDATE_NOT_LIVE_RENDER_OR_GL_CERTIFICATION");
        } finally {archive.close();}
    }
    private RenderCallsiteAudit() {}
}
