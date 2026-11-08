package dev.slinkywinky.attach;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R162 OFFLINE-ONLY lexical invocation inventory of original ave.s()V.
 * Source order is not a control-flow/reachability proof and cannot admit a
 * live GUI input hook. Java 8 + ASM 5.2; never bundled in the agent.
 */
public final class R162InputOrderLedger implements Opcodes {
    private static final String KEY="org/lwjgl/input/Keyboard";
    private static final String MOUSE="org/lwjgl/input/Mouse";
    private static final String BRIDGED=
        "dev/slinkywinky/instrument/KeyboardQueueOwner";
    // Read-only original ave.class instruction-call sequence, verified on
    // the official Mojang SHA-1/size-pinned 1.8.9 JAR in R162.
    public static final String PINNED_ORDER_SHA256 =
        "6b58cf399b237ce7a84a84f1a62a8f5b56dc10e457f6217ea35aaadf99403e7f";
    private R162InputOrderLedger() {}
    public static final class Site {
        public final int callOrdinal;
        public final String phase;
        public final String family;
        public final String signature;
        Site(int ordinal,String phase,String family,String signature) {
            this.callOrdinal=ordinal;this.phase=phase;
            this.family=family;this.signature=signature;
        }
        public String normalized() {
            return callOrdinal+"|"+phase+"|"+family+"|"+signature;
        }
        @Override public String toString(){return normalized();}
    }
    public static final class Report {
        public final List<Site> sites;
        public final int methodCalls;
        public final int jumps,switches,handlers;
        Report(List<Site> s,int c,int j,int w,int h){
            sites=Collections.unmodifiableList(new ArrayList<Site>(s));
            methodCalls=c;jumps=j;switches=w;handlers=h;
        }
        public int count(String family) {
            int c=0;
            for(Site s:sites)if(family.equals(s.family))c++;
            return c;
        }
        public int count(String family,String phase) {
            int c=0;
            for(Site s:sites)
                if(family.equals(s.family)&&phase.equals(s.phase))c++;
            return c;
        }
        public String digest() {
            try {
                MessageDigest sha=MessageDigest.getInstance("SHA-256");
                for(Site s:sites) {
                    sha.update(s.normalized().getBytes(StandardCharsets.UTF_8));
                    sha.update((byte)10);
                }
                StringBuilder hex=new StringBuilder(64);
                for(byte b:sha.digest()) {
                    int x=b&255;if(x<16)hex.append('0');
                    hex.append(Integer.toHexString(x));
                }
                return hex.toString();
            } catch(Exception e){throw new IllegalStateException(e);}
        }
        /** No legal proof of foreground, vanilla input suppression or rollback. */
        public boolean canAuthorizeLiveInput() {return false;}
    }
    public static Report inspect(byte[] raw) {
        if(raw==null || raw.length<32 || raw.length>1000000)
            throw new IllegalArgumentException("Invalid original input owner");
        final ClassReader reader=new ClassReader(raw);
        if(!"ave".equals(reader.getClassName()))
            throw new IllegalArgumentException("Not original obfuscated ave");
        final int[] targets={0},calls={0},jumps={0},switches={0},handlers={0};
        final List<Site> found=new ArrayList<Site>();
        reader.accept(new ClassVisitor(ASM5) {
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String signature,String[] exceptions) {
                final boolean target="s".equals(name)&&"()V".equals(desc)&&
                    (access&ACC_PUBLIC)!=0 &&
                    (access&(ACC_STATIC|ACC_ABSTRACT|ACC_NATIVE))==0;
                if(target)targets[0]++;
                return new MethodVisitor(ASM5) {
                    private int ordinal=0;
                    private String phase="PRE_MOUSE_QUEUE";
                    @Override public void visitMethodInsn(int op,String owner,
                            String method,String descriptor,boolean itf) {
                        if(!target)return;
                        ordinal++;calls[0]++;
                        String family=null, canonicalOwner=owner;
                        if(MOUSE.equals(owner)){
                            if(op==INVOKESTATIC && method.equals("next") &&
                               descriptor.equals("()Z"))family="MOUSE_QUEUE";
                            else if(method.equals("getEventButtonState"))
                                family="MOUSE_BUTTON_STATE";
                            else if(method.equals("getEventDWheel"))
                                family="MOUSE_WHEEL";
                            else family="MOUSE_OTHER";
                        }else if(KEY.equals(owner)||BRIDGED.equals(owner)){
                            if(op==INVOKESTATIC && method.equals("next") &&
                               descriptor.equals("()Z")) {
                                family="KEYBOARD_QUEUE";
                                canonicalOwner=KEY;
                            }else if(method.equals("isKeyDown")&&
                                    descriptor.equals("(I)Z"))
                                family="DIRECT_KEY_DOWN";
                            else family="KEYBOARD_OTHER";
                        }else if("avb".equals(owner)){
                            if(op==INVOKESTATIC && method.equals("a") &&
                               descriptor.equals("(IZ)V"))family="BIND_STATE";
                            else if(op==INVOKESTATIC && method.equals("a") &&
                                    descriptor.equals("(I)V"))family="BIND_PRESS";
                            else if(op==INVOKEVIRTUAL && method.equals("f") &&
                                    descriptor.equals("()Z"))family="BIND_HELD_POLL";
                            else if(op==INVOKEVIRTUAL && method.equals("d") &&
                                    descriptor.equals("()Z"))family="BIND_PRESS_POLL";
                            else family="BIND_OTHER";
                        }else if("axu".equals(owner))family="GUI_DISPATCH";
                        if(family!=null)
                            found.add(new Site(ordinal,phase,family,
                                op+":"+canonicalOwner+"."+method+descriptor));
                        if("MOUSE_QUEUE".equals(family))
                            phase="AFTER_MOUSE_BEFORE_KEYBOARD";
                        if("KEYBOARD_QUEUE".equals(family))
                            phase="AFTER_KEYBOARD_QUEUE";
                    }
                    @Override public void visitJumpInsn(int opcode,
                            org.objectweb.asm.Label targetLabel) {
                        if(target)jumps[0]++;
                    }
                    @Override public void visitTableSwitchInsn(int min,
                            int max,org.objectweb.asm.Label dflt,
                            org.objectweb.asm.Label... labels) {
                        if(target)switches[0]++;
                    }
                    @Override public void visitLookupSwitchInsn(
                            org.objectweb.asm.Label dflt,int[] keys,
                            org.objectweb.asm.Label[] labels) {
                        if(target)switches[0]++;
                    }
                    @Override public void visitTryCatchBlock(
                            org.objectweb.asm.Label start,
                            org.objectweb.asm.Label end,
                            org.objectweb.asm.Label handler,String type) {
                        if(target)handlers[0]++;
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        if(targets[0]!=1)throw new IllegalArgumentException(
            "Not a single public original ave.s()V input owner");
        return new Report(found,calls[0],jumps[0],switches[0],handlers[0]);
    }
    /** Source-pinned original must match the existing independent R119 audit. */
    public static Report pinned(byte[] original) {
        OriginalInputOwnershipAudit.inspectPinned(original);
        final Report report=inspect(original);
        if(report.count("MOUSE_QUEUE")!=1 ||
           report.count("KEYBOARD_QUEUE")!=1 ||
           report.count("DIRECT_KEY_DOWN")!=17 ||
           report.count("BIND_HELD_POLL")!=13 ||
           report.count("BIND_PRESS_POLL")!=3 ||
           report.count("BIND_STATE")!=2 ||
           report.count("BIND_PRESS")!=2 ||
           report.count("MOUSE_BUTTON_STATE")!=3 ||
           report.count("MOUSE_WHEEL")!=1 ||
           report.count("DIRECT_KEY_DOWN","AFTER_KEYBOARD_QUEUE")!=17 ||
           report.count("BIND_HELD_POLL","AFTER_KEYBOARD_QUEUE")!=13 ||
           report.count("BIND_PRESS_POLL","AFTER_KEYBOARD_QUEUE")!=3 ||
           report.count("MOUSE_QUEUE","PRE_MOUSE_QUEUE")!=1 ||
           report.count("KEYBOARD_QUEUE","AFTER_MOUSE_BEFORE_KEYBOARD")!=1 ||
           report.sites.size()!=58 || report.methodCalls!=180 ||
           report.jumps!=179 || report.switches!=0 || report.handlers!=3 ||
           !PINNED_ORDER_SHA256.equals(report.digest()))
            throw new IllegalArgumentException(
                "R162 source-ordered instruction path shape changed");
        return report;
    }
}
