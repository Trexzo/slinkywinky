package dev.slinkywinky.attach;

import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R107. Read-only, source-pinned offline audit of vanilla Minecraft 1.8.9
 * keyboard event dispatch. No bytecode transformation or input handling.
 */
public final class KeyboardInputCallsiteAudit {
    private static final long JAR_SIZE=8461484L;
    private static final String JAR_SHA1=
        "3870888a6c3d349d3771a3e9d16c9bf5e076b908";
    private static final String MC_SHA256=
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82";
    private static final String LWJGL="org/lwjgl/input/Keyboard";
    private KeyboardInputCallsiteAudit() {}

    static final class Call {
        final int opcode;
        final String owner,name,desc;
        Call(int op,String o,String n,String d) {
            opcode=op;owner=o;name=n;desc=d;
        }
        boolean matches(int op,String o,String n,String d) {
            return opcode==op && owner.equals(o) &&
                   name.equals(n) && desc.equals(d);
        }
    }

    static int count(List<Call> calls,int first,int last,
                     int op,String owner,String name,String desc) {
        int total=0;
        for(int i=first;i<last;i++)
            if(calls.get(i).matches(op,owner,name,desc))total++;
        return total;
    }
    static int first(List<Call> calls,int op,String owner,String name,String desc) {
        for(int i=0;i<calls.size();i++)
            if(calls.get(i).matches(op,owner,name,desc))return i;
        return -1;
    }

    static String inspect(byte[] raw) {
        if(raw==null || raw.length>1000000 || raw.length<32)
            throw new IllegalArgumentException("Missing or oversized ave class");
        final ClassReader reader=new ClassReader(raw);
        if(!"ave".equals(reader.getClassName()))
            throw new IllegalArgumentException("Input audit requires obfuscated ave");
        final List<Call> calls=new ArrayList<Call>();
        final int[] methods={0};
        final int[] allNext={0};
        reader.accept(new ClassVisitor(Opcodes.ASM5) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] exceptions) {
                final boolean target="s".equals(name) && "()V".equals(desc) &&
                    (access & (Opcodes.ACC_STATIC|Opcodes.ACC_ABSTRACT|
                        Opcodes.ACC_NATIVE))==0 && (access & Opcodes.ACC_PUBLIC)!=0;
                if(target)methods[0]++;
                return new MethodVisitor(Opcodes.ASM5) {
                    @Override public void visitMethodInsn(int opcode,String owner,
                            String method,String descriptor,boolean itf) {
                        if(opcode==Opcodes.INVOKESTATIC && LWJGL.equals(owner) &&
                           "next".equals(method) && "()Z".equals(descriptor))
                            allNext[0]++;
                        if(target)calls.add(new Call(opcode,owner,method,descriptor));
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        if(methods[0]!=1 || allNext[0]!=1)
            throw new IllegalStateException("Unrecognized original input owner");
        final int call=Opcodes.INVOKESTATIC;
        final int next=first(calls,call,LWJGL,"next","()Z");
        if(next<0 || count(calls,0,calls.size(),call,LWJGL,"getEventKey","()I")!=2 ||
           count(calls,0,calls.size(),call,LWJGL,"getEventKeyState","()Z")!=3 ||
           count(calls,0,calls.size(),call,LWJGL,"getEventCharacter","()C")!=1)
            throw new IllegalStateException("Original queued keyboard event path changed");
        // Both pre-queue and post-queue dispatches exist. A visible overlay
        // does NOT by itself prevent these gameplay bindings from seeing keys.
        int preKey=count(calls,0,next,call,"avb","a","(IZ)V");
        int prePress=count(calls,0,next,call,"avb","a","(I)V");
        int postKey=count(calls,next+1,calls.size(),call,"avb","a","(IZ)V");
        int postPress=count(calls,next+1,calls.size(),call,"avb","a","(I)V");
        int guiMethods=0;
        for(int i=next+1;i<calls.size();i++) {
            Call current=calls.get(i);
            if(current.owner.equals("axu"))guiMethods++;
        }
        if(preKey!=1 || prePress!=1 || postKey!=1 || postPress!=1 || guiMethods<1)
            throw new IllegalStateException("Normal gameplay/GUI dispatch route changed");
        return "owner=ave.s()V next=1 eventKey=2 eventState=3 eventChar=1"+
               " gameBindingsBefore=2 gameBindingsAfter=2 guiDispatchPresent=true";
    }

    public static void main(String[] args) throws Exception {
        if(args.length!=1)throw new IllegalArgumentException(
            "Expected absolute SHA-pinned ORIGINAL Minecraft 1.8.9 JAR");
        Path path=Paths.get(args[0]);
        if(!path.isAbsolute() || !Files.isRegularFile(path) ||
           Files.size(path)!=JAR_SIZE)
            throw new IllegalArgumentException("Missing or wrong-size client");
        MessageDigest sha1=MessageDigest.getInstance("SHA-1");
        try(InputStream input=Files.newInputStream(path)) {
            byte[] buffer=new byte[32768];int n;
            while((n=input.read(buffer))!=-1)sha1.update(buffer,0,n);
        }
        if(!Arrays.equals(sha1.digest(),RenderCallsiteAudit.decodeHex(JAR_SHA1))) {
            throw new IllegalArgumentException("Original client digest mismatch");
        }
        byte[] game;
        try(ZipFile archive=new ZipFile(path.toFile())) {
            game=RenderCallsiteAudit.entry(archive,"ave.class");
        }
        if(!MC_SHA256.equals(RenderCallsiteAudit.sha256Hex(game)))
            throw new IllegalArgumentException("Original class bytes changed");
        System.out.println("R107_ORIGINAL_189_KEYBOARD_EVENT_PATH_AUDITED "+
            inspect(game));
        System.out.println("R107_NORMAL_GAMEPLAY_DISPATCH_PRESENT_NOT_MODAL_INPUT_CERTIFICATION");
    }
}
