package dev.slinkywinky.attach;

import java.security.MessageDigest;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R119 read-only control-flow inventory. Test/audit code ONLY:
 * never installed in the native/JVM agent, and never transforms game code.
 *
 * A queue-only Keyboard.next() substitution leaves pre-queue mouse
 * dispatch, direct key state queries and held-keybinding polling reachable.
 */
public final class OriginalInputOwnershipAudit implements Opcodes {
    private static final String ORIGINAL_SHA256 =
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82";
    private static final String K="org/lwjgl/input/Keyboard";
    private static final String M="org/lwjgl/input/Mouse";
    private OriginalInputOwnershipAudit(){}
    public static final class Report {
        public final String className;
        public int targetMethods;
        public int keyboardNext,bridgedNext,keyboardEventKey,keyboardEventState;
        public int rawKeyDown,mouseNext,mouseButtonEvents,mouseWheelEvents;
        public int bindStateBeforeQueue,bindStateAfterQueue;
        public int bindPressBeforeQueue,bindPressAfterQueue;
        public int heldBindingPolls,pressBindingPolls,guiCalls;
        public int keyDownBeforeQueue,keyDownAfterQueue;
        public int mouseBeforeQueue,mouseAfterQueue;
        Report(String name){className=name;}
        /**
         * This intentionally is a NEGATIVE gate, not a license to hook:
         * even zero counters do not prove a safe restore or input owner.
         */
        public boolean queueOnlyHasResidualGamePaths() {
            return rawKeyDown>0 || heldBindingPolls>0 ||
                   pressBindingPolls>0 || mouseNext>0 ||
                   bindStateBeforeQueue>0 || bindStateAfterQueue>0 ||
                   bindPressBeforeQueue>0 || bindPressAfterQueue>0;
        }
        public void refuseQueueOnlyModal() {
            if(queueOnlyHasResidualGamePaths())
                throw new IllegalStateException(
                    "R119 queue-only hook cannot isolate gameplay input: "+summary());
            throw new IllegalStateException(
                "R119 missing positive proof of complete input ownership and restore");
        }
        public String summary() {
            return "keyboardNext="+keyboardNext+" bridgedNext="+bridgedNext+
                " rawIsKeyDown="+rawKeyDown+
                " mouseNext="+mouseNext+
                " mouseButton="+mouseButtonEvents+
                " mouseWheel="+mouseWheelEvents+
                " avbStateBefore="+bindStateBeforeQueue+
                " avbStateAfter="+bindStateAfterQueue+
                " avbPressBefore="+bindPressBeforeQueue+
                " avbPressAfter="+bindPressAfterQueue+
                " avbHeldPolls="+heldBindingPolls+
                " avbPressPolls="+pressBindingPolls+
                " keyDownBefore="+keyDownBeforeQueue+
                " keyDownAfter="+keyDownAfterQueue;
        }
    }
    private static void require(boolean b,String why) {
        if(!b)throw new IllegalArgumentException(why);
    }
    private static String digest(byte[] raw) {
        try {
            byte[] hash=MessageDigest.getInstance("SHA-256").digest(raw);
            StringBuilder s=new StringBuilder();
            for(byte b:hash) {
                int value=b&255;
                if(value<16)s.append('0');
                s.append(Integer.toHexString(value));
            }
            return s.toString();
        }catch(Exception bad){throw new IllegalStateException(bad);}
    }
    public static Report inspectCandidate(byte[] raw) {
        require(raw!=null && raw.length>32 && raw.length<=1000000,
                "Missing/oversized original input class");
        require((raw[0]&255)==0xca && (raw[1]&255)==0xfe &&
                (raw[2]&255)==0xba && (raw[3]&255)==0xbe,
                "Unrecognized JVM classfile magic");
        final ClassReader reader=new ClassReader(raw);
        final Report r=new Report(reader.getClassName());
        require("ave".equals(r.className),"Only obfuscated Minecraft 1.8.9 ave");
        reader.accept(new ClassVisitor(ASM5) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String descriptor,String sig,String[] errors) {
                final boolean target="s".equals(name) &&
                    "()V".equals(descriptor) && (access&ACC_PUBLIC)!=0 &&
                    (access&(ACC_STATIC|ACC_NATIVE|ACC_ABSTRACT))==0;
                if(target)r.targetMethods++;
                return new MethodVisitor(ASM5) {
                    private boolean afterQueue;
                    @Override public void visitMethodInsn(int op,String owner,
                            String method,String desc,boolean itf) {
                        if(!target)return;
                        if(op==INVOKESTATIC && "next".equals(method) &&
                           "()Z".equals(desc) &&
                           (K.equals(owner) ||
                            "dev/slinkywinky/instrument/KeyboardQueueOwner".equals(owner))) {
                            if(K.equals(owner))r.keyboardNext++;
                            else r.bridgedNext++;
                            afterQueue=true;
                        }
                        if(op==INVOKESTATIC && K.equals(owner)) {
                            if("isKeyDown".equals(method)&&"(I)Z".equals(desc)) {
                                r.rawKeyDown++;
                                if(afterQueue)r.keyDownAfterQueue++;
                                else r.keyDownBeforeQueue++;
                            }
                            if("getEventKey".equals(method)&&"()I".equals(desc))
                                r.keyboardEventKey++;
                            if("getEventKeyState".equals(method)&&"()Z".equals(desc))
                                r.keyboardEventState++;
                        }
                        if(op==INVOKESTATIC && M.equals(owner)) {
                            if("next".equals(method)&&"()Z".equals(desc)) {
                                r.mouseNext++;
                                if(afterQueue)r.mouseAfterQueue++;
                                else r.mouseBeforeQueue++;
                            }
                            if("getEventButtonState".equals(method))
                                r.mouseButtonEvents++;
                            if("getEventDWheel".equals(method))
                                r.mouseWheelEvents++;
                        }
                        if("avb".equals(owner)) {
                            if(op==INVOKESTATIC && "a".equals(method) &&
                               "(IZ)V".equals(desc)) {
                                if(afterQueue)r.bindStateAfterQueue++;
                                else r.bindStateBeforeQueue++;
                            }
                            if(op==INVOKESTATIC && "a".equals(method) &&
                               "(I)V".equals(desc)) {
                                if(afterQueue)r.bindPressAfterQueue++;
                                else r.bindPressBeforeQueue++;
                            }
                            if(op==INVOKEVIRTUAL && "f".equals(method) &&
                               "()Z".equals(desc))r.heldBindingPolls++;
                            if(op==INVOKEVIRTUAL && "d".equals(method) &&
                               "()Z".equals(desc))r.pressBindingPolls++;
                        }
                        if("axu".equals(owner))r.guiCalls++;
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        require(r.targetMethods==1,"Original input owner s()V missing/ambiguous");
        return r;
    }
    public static Report inspectPinned(byte[] raw) {
        require(raw!=null && raw.length<=1000000 &&
                ORIGINAL_SHA256.equals(digest(raw)),
                "Original vanilla 1.8.9 ave.class SHA-256 mismatch");
        final Report r=inspectCandidate(raw);
        System.out.println("R119_PINNED_INPUT_METHOD_INSPECTED "+r.summary());
        require(r.keyboardNext==1 && r.bridgedNext==0 &&
                r.keyboardEventKey==2 && r.keyboardEventState==3 &&
                r.rawKeyDown==17 &&
                r.mouseNext==1 && r.mouseBeforeQueue==1 &&
                r.mouseAfterQueue==0 &&
                r.mouseButtonEvents==3 && r.mouseWheelEvents==1 &&
                r.bindStateBeforeQueue==1 && r.bindStateAfterQueue==1 &&
                r.bindPressBeforeQueue==1 && r.bindPressAfterQueue==1 &&
                r.heldBindingPolls==13 && r.pressBindingPolls==3 &&
                r.keyDownBeforeQueue==0 && r.keyDownAfterQueue==17 &&
                r.guiCalls>=1,
                "Original keybind, direct key state or mouse dispatch shape changed");
        return r;
    }
}
