package dev.slinkywinky.attach;

import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** R260 synthetic ASM negative controls; no game/launcher/Attach process. */
public final class R260OutgoingAttackCallsiteSmoke implements Opcodes {
    private static void check(boolean condition,String label){
        if(!condition)throw new AssertionError("R260 "+label);
    }
    static byte[] game(int variant){
        ClassWriter writer=new ClassWriter(0);
        String clazz=variant==5?"fake":"ave";
        writer.visit(V1_8,ACC_PUBLIC,clazz,null,"java/lang/Object",null);
        if(variant!=8){
            MethodVisitor click=writer.visitMethod(
                variant==4?ACC_PUBLIC|ACC_STATIC:ACC_PRIVATE,
                variant==3?"wrong":"aw","()V",null,null);
            click.visitCode();
            if(variant!=1){
                click.visitInsn(ACONST_NULL);
                click.visitInsn(ACONST_NULL);
                click.visitInsn(ACONST_NULL);
                click.visitMethodInsn(INVOKEVIRTUAL,
                    variant==6?"other":"bda","a",
                    variant==7?"(Lwn;)V":"(Lwn;Lpk;)V",false);
            }
            if(variant==2){
                click.visitInsn(ACONST_NULL);
                click.visitInsn(ACONST_NULL);
                click.visitInsn(ACONST_NULL);
                click.visitMethodInsn(INVOKEVIRTUAL,
                    "bda","a","(Lwn;Lpk;)V",false);
            }
            click.visitInsn(RETURN);
            click.visitMaxs(3,variant==4?0:1);
            click.visitEnd();
        }
        MethodVisitor tick=writer.visitMethod(ACC_PUBLIC,"s","()V",null,null);
        tick.visitCode();tick.visitVarInsn(ALOAD,0);
        tick.visitMethodInsn(INVOKESPECIAL,"ave","aw","()V",false);
        tick.visitInsn(RETURN);tick.visitMaxs(1,1);tick.visitEnd();
        writer.visitEnd();
        return writer.toByteArray();
    }
    static byte[] controller(boolean wrong){
        ClassWriter writer=new ClassWriter(0);
        writer.visit(V1_8,ACC_PUBLIC,wrong?"fake":"bda",
            null,"java/lang/Object",null);
        MethodVisitor method=writer.visitMethod(
            ACC_PUBLIC,"a","(Lwn;Lpk;)V",null,null);
        method.visitCode();
        method.visitVarInsn(ALOAD,1);method.visitVarInsn(ALOAD,2);
        method.visitMethodInsn(INVOKEVIRTUAL,"wn","f","(Lpk;)V",false);
        method.visitInsn(RETURN);method.visitMaxs(2,3);method.visitEnd();
        writer.visitEnd();
        return writer.toByteArray();
    }
    public static void main(String[] args){
        R260OutgoingAttackCallsiteAudit.Report proof=
            R260OutgoingAttackCallsiteAudit.inspect(game(0),controller(false));
        check(proof.clientAttackCalls==1 &&
              proof.clientTickClickCalls==1 &&
              proof.controllerPlayerAttackCalls==1,
              "synthetic exact-source graph");
        check(!proof.verifiesSuccessfulHit() &&
              !proof.authorizesLiveCallback() &&
              !proof.suppliesLastOutgoingAttackTime(),
              "source callsite must not grant gameplay authority");
        for(int variant=1;variant<=8;variant++){
            boolean rejected=false;
            try {
                R260OutgoingAttackCallsiteAudit.inspect(
                    game(variant),controller(false));
            }catch(RuntimeException expected){rejected=true;}
            check(rejected,"accepted unverified variant "+variant);
        }
        boolean wrongController=false;
        try {
            R260OutgoingAttackCallsiteAudit.inspect(
                game(0),controller(true));
        }catch(IllegalArgumentException expected){wrongController=true;}
        check(wrongController,"accepted wrong class identity");
        System.out.println("R260_OUTGOING_ATTACK_CALLSITE_9_NEGATIVE_CONTROLS_PASS");
        System.out.println("R260_NOT_AN_ACCEPTED_HIT_OR_RUNTIME_CALLBACK_PASS");
    }
}
