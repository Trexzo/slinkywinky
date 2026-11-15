package dev.slinkywinky.forge;

import java.lang.instrument.ClassFileTransformer;
import java.security.ProtectionDomain;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.tree.AbstractInsnNode;
import org.objectweb.asm.tree.ClassNode;
import org.objectweb.asm.tree.FieldInsnNode;
import org.objectweb.asm.tree.InsnList;
import org.objectweb.asm.tree.LdcInsnNode;
import org.objectweb.asm.tree.MethodInsnNode;
import org.objectweb.asm.tree.MethodNode;
import org.objectweb.asm.tree.VarInsnNode;

/**
 * R214: precise SOURCE-DERIVED local 1.8.9 ASM insertion after vanilla's
 * two 0.2f item-use movement-input field writes, before the following
 * player movement handling. Not an injectable production hook.
 *
 * It is admitted only by an explicit TEST javaagent in a named TEMP
 * singleplayer instance. The transformer does nothing by default.
 */
public final class R214SlowdownTransformer
        implements ClassFileTransformer, Opcodes {
    static final String PLAYER = "net/minecraft/client/entity/EntityPlayerSP";
    static final String MOVEMENT = "net/minecraft/util/MovementInput";
    static final String HOOK = "dev/slinkywinky/forge/R214NoSlowHook";
    static final String HOOK_METHOD = "afterVanillaSlowdown";
    static final String HOOK_DESC = "(L" + PLAYER + ";)V";
    static final String[] LIVING_METHODS =
        {"onLivingUpdate", "func_70636_d"};
    static final String[] STRAFE_FIELDS =
        {"moveStrafe", "field_78902_a"};
    static final String[] FORWARD_FIELDS =
        {"moveForward", "field_78900_b"};
    static final String[] REFRESH_METHODS =
        {"updatePlayerMoveState", "func_78898_a"};

    @Override public byte[] transform(ClassLoader loader, String className,
            Class<?> type, ProtectionDomain protection, byte[] source) {
        if (source == null || className == null ||
            !PLAYER.equals(className.replace('.','/')))
            return null;
        try {
            final byte[] patched = instrument(source);
            System.out.println("[SlinkyWinkyR214] R214_POST_SLOW_PRE_CONSUME_ASM_PATCH_PASS");
            return patched;
        } catch (RuntimeException failure) {
            System.err.println("[SlinkyWinkyR214] R214_TRANSFORM_REFUSED " +
                               failure.getMessage());
            // Unsupported versions fail closed: never accidentally patch
            // a different instruction or mutate a gameplay field.
            return null;
        }
    }

    /** Source-testable deterministic transformation, no Minecraft launch. */
    public static byte[] instrument(byte[] source) {
        if (source == null) throw new IllegalArgumentException("Missing class bytes");
        final ClassNode type = new ClassNode(ASM5);
        new ClassReader(source).accept(type, 0);
        if (!PLAYER.equals(type.name))
            throw new IllegalArgumentException("Unrecognized class for No Slow hook");

        MethodNode selected = null;
        AbstractInsnNode insertion = null;
        int candidates = 0;
        for (Object o : type.methods) {
            final MethodNode method = (MethodNode)o;
            if (!equalsAny(method.name, LIVING_METHODS) ||
                !"()V".equals(method.desc)) continue;
            int refreshes = 0;
            for (AbstractInsnNode insn=method.instructions.getFirst();
                 insn!=null;insn=insn.getNext()) {
                if (insn.getOpcode()==INVOKEVIRTUAL &&
                    insn instanceof MethodInsnNode) {
                    final MethodInsnNode call=(MethodInsnNode)insn;
                    if(MOVEMENT.equals(call.owner) &&
                       equalsAny(call.name,REFRESH_METHODS) &&
                       "()V".equals(call.desc))
                        ++refreshes;
                }
            }
            if(refreshes!=1) continue;
            AbstractInsnNode lastStrafe = null;
            int distance = 1000;
            for (AbstractInsnNode insn=method.instructions.getFirst();
                 insn!=null;insn=insn.getNext()) {
                if(lastStrafe!=null) ++distance;
                if(scaledFieldWrite(insn,STRAFE_FIELDS)) {
                    lastStrafe=insn;
                    distance=0;
                }
                if(scaledFieldWrite(insn,FORWARD_FIELDS) &&
                   lastStrafe!=null && distance>=1 && distance<=12) {
                    ++candidates;
                    selected=method;
                    insertion=insn;
                }
            }
        }
        if(candidates!=1 || selected==null || insertion==null)
            throw new IllegalStateException("Missing or ambiguous 1.8.9 slowdown signature: "+
                                            candidates);
        // The original two slowdown writes have finished. Insert only
        // an ordinary static call before the next Minecraft instruction.
        final InsnList added=new InsnList();
        added.add(new VarInsnNode(ALOAD,0));
        added.add(new MethodInsnNode(INVOKESTATIC,HOOK,HOOK_METHOD,HOOK_DESC,false));
        selected.instructions.insert(insertion,added);
        final ClassWriter writer=new ClassWriter(ClassWriter.COMPUTE_MAXS);
        type.accept(writer);
        return writer.toByteArray();
    }

    private static boolean scaledFieldWrite(AbstractInsnNode insn, String[] names) {
        if(insn.getOpcode()!=PUTFIELD || !(insn instanceof FieldInsnNode)) return false;
        final FieldInsnNode field=(FieldInsnNode)insn;
        if(!MOVEMENT.equals(field.owner) || !"F".equals(field.desc) ||
           !equalsAny(field.name,names)) return false;
        final AbstractInsnNode mul=insn.getPrevious();
        final AbstractInsnNode constant=mul==null?null:mul.getPrevious();
        return mul!=null && mul.getOpcode()==FMUL &&
               constant instanceof LdcInsnNode &&
               ((LdcInsnNode)constant).cst instanceof Float &&
               Float.compare((Float)((LdcInsnNode)constant).cst,0.2f)==0;
    }

    private static boolean equalsAny(String found, String[] choices) {
        for(String expected:choices)
            if(expected.equals(found)) return true;
        return false;
    }
}
