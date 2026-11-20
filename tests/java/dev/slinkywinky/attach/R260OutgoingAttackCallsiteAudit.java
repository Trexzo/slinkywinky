package dev.slinkywinky.attach;

import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R260: actual original vanilla 1.8.9 click -> client-controller
 * attack ATTEMPT callsite. Offline method analysis only. Never claims
 * an outgoing packet was accepted, the target was hit, or an attack
 * cooldown can be synthesized from EntityLivingBase.bg().
 *
 * Exact public joined.srg:
 * ave.aw()V: Minecraft.func_147116_af (clickMouse)
 * bda.a(Lwn;Lpk;)V: PlayerControllerMP.func_78764_a (attackEntity)
 * Both names must be verified against the SHA-1 pinned original JAR.
 */
public final class R260OutgoingAttackCallsiteAudit implements Opcodes {
    static final long ORIGINAL_SIZE=8461484L;
    static final String ORIGINAL_SHA1=
        "3870888a6c3d349d3771a3e9d16c9bf5e076b908";
    static final String AVE_SHA256=
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82";
    static final String ATTACK_DESCRIPTOR="(Lwn;Lpk;)V";
    private R260OutgoingAttackCallsiteAudit(){}

    public static final class Report {
        public final int clientClickMethods,attackControllerMethods;
        public final int clientAttackCalls,clientTickClickCalls;
        public final int controllerPlayerAttackCalls;
        Report(int click,int controller,int attack,int tick,int player){
            clientClickMethods=click;
            attackControllerMethods=controller;
            clientAttackCalls=attack;
            clientTickClickCalls=tick;
            controllerPlayerAttackCalls=player;
        }
        public boolean verifiesSuccessfulHit(){return false;}
        public boolean authorizesLiveCallback(){return false;}
        public boolean suppliesLastOutgoingAttackTime(){return false;}
    }
    private static byte[] validatedClass(byte[] bytes,String expected){
        if(bytes==null || bytes.length<32 || bytes.length>1000000)
            throw new IllegalArgumentException("Absent or oversized R260 class");
        if(!expected.equals(new ClassReader(bytes).getClassName()))
            throw new IllegalArgumentException("Wrong obfuscated class");
        return bytes;
    }
    public static Report inspect(byte[] game,byte[] controller){
        validatedClass(game,"ave");
        validatedClass(controller,"bda");
        final int[] counts=new int[5];
        new ClassReader(game).accept(new ClassVisitor(ASM5){
            @Override public MethodVisitor visitMethod(
                    int access,String name,String desc,String signature,String[] exceptions){
                final boolean click="aw".equals(name)&&"()V".equals(desc) &&
                    (access&(ACC_STATIC|ACC_ABSTRACT|ACC_NATIVE))==0;
                final boolean tick="s".equals(name)&&"()V".equals(desc) &&
                    (access&(ACC_STATIC|ACC_ABSTRACT|ACC_NATIVE))==0;
                if(click)counts[0]++;
                if(!click&&!tick)return null;
                return new MethodVisitor(ASM5){
                    @Override public void visitMethodInsn(
                            int opcode,String owner,String method,String descriptor,
                            boolean interfaceCall){
                        if(click && opcode==INVOKEVIRTUAL &&
                           "bda".equals(owner)&&"a".equals(method) &&
                           ATTACK_DESCRIPTOR.equals(descriptor)&&!interfaceCall)
                            counts[2]++;
                        if(tick && opcode==INVOKESPECIAL &&
                           "ave".equals(owner)&&"aw".equals(method) &&
                           "()V".equals(descriptor)&&!interfaceCall)
                            counts[3]++;
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        new ClassReader(controller).accept(new ClassVisitor(ASM5){
            @Override public MethodVisitor visitMethod(
                    int access,String name,String desc,String signature,String[] exceptions){
                final boolean attack="a".equals(name) &&
                    ATTACK_DESCRIPTOR.equals(desc) &&
                    (access&(ACC_STATIC|ACC_ABSTRACT|ACC_NATIVE))==0;
                if(!attack)return null;
                counts[1]++;
                return new MethodVisitor(ASM5){
                    @Override public void visitMethodInsn(
                            int opcode,String owner,String method,String descriptor,
                            boolean interfaceCall){
                        if(opcode==INVOKEVIRTUAL && "wn".equals(owner) &&
                           "f".equals(method)&&"(Lpk;)V".equals(descriptor) &&
                           !interfaceCall)
                            counts[4]++;
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        if(counts[0]!=1 || counts[1]!=1 || counts[2]!=1)
            throw new IllegalStateException("Original outgoing click/attack callsite not unique");
        return new Report(counts[0],counts[1],counts[2],counts[3],counts[4]);
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=1)
            throw new IllegalArgumentException("Expected official SHA-pinned Mojang 1.8.9 client JAR");
        Path file=Paths.get(args[0]);
        if(!file.isAbsolute() || !Files.isRegularFile(file) ||
           Files.size(file)!=ORIGINAL_SIZE)
            throw new IllegalArgumentException("Original client JAR length mismatch");
        MessageDigest digest=MessageDigest.getInstance("SHA-1");
        try(InputStream input=Files.newInputStream(file)){
            byte[] buffer=new byte[32768];
            for(int n;(n=input.read(buffer))!=-1;)digest.update(buffer,0,n);
        }
        if(!Arrays.equals(digest.digest(),
               RenderCallsiteAudit.decodeHex(ORIGINAL_SHA1)))
            throw new IllegalArgumentException("Original Mojang SHA-1 mismatch");
        byte[] ave,bda;
        try(ZipFile archive=new ZipFile(file.toFile())){
            ave=RenderCallsiteAudit.entry(archive,"ave.class");
            bda=RenderCallsiteAudit.entry(archive,"bda.class");
        }
        if(!AVE_SHA256.equals(RenderCallsiteAudit.sha256Hex(ave)))
            throw new IllegalArgumentException("Original ave class digest mismatch");
        Report observed=inspect(ave,bda);
        System.out.println("R260_ORIGINAL_189_OUTGOING_ATTACK_CALLSITE_PASS"+
            " click=ave.aw()V controller=bda.a(Lwn;Lpk;)V"+
            " directAttackCalls="+observed.clientAttackCalls+
            " tickDirectCalls="+observed.clientTickClickCalls+
            " playerAttackCalls="+observed.controllerPlayerAttackCalls);
        System.out.println("R260_ATTACK_ATTEMPT_ONLY_NO_CONFIRMED_HIT_OR_SERVER_ACK");
        System.out.println("R260_ORIGINAL_JAR_READONLY_NO_GAME_ATTACH");
    }
}
