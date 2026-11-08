package dev.slinkywinky.attach;

import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Map;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R163 offline original-Minecraft input control-flow inventory.
 * Lexical CFG is a conservative static overapproximation, NOT a proof of
 * physical input isolation, game-thread ownership, or live class restoration.
 * This entire class is tests/java-only and is excluded from the agent JAR.
 */
public final class R163InputControlGraph implements Opcodes {
    private static final String KEY="org/lwjgl/input/Keyboard";
    private static final String MOUSE="org/lwjgl/input/Mouse";
    private static final String BRIDGED=
        "dev/slinkywinky/instrument/KeyboardQueueOwner";
    public static final String PINNED_CFG_SHA256 =
        "2b9173842b6b507e45190da633e9c83d5f9d0e5d2969ff181995b8970fd97256";
    private R163InputControlGraph() {}

    private static final class Node {
        final int opcode;
        final String signature;
        final String kind;
        final List<String> edges=new ArrayList<String>();
        final List<Label> jumpTargets=new ArrayList<Label>();
        boolean fallthrough=true;
        Node(int opcode,String signature,String kind){
            this.opcode=opcode;this.signature=signature;this.kind=kind;
        }
    }
    private static final class Region {
        final Label from,to,handler;
        final String type;
        Region(Label from,Label to,Label handler,String type){
            this.from=from;this.to=to;this.handler=handler;this.type=type;
        }
    }
    public static final class Report {
        public final int instructions,jumps,handlers,sourceSites;
        public final int reachableInstructions,reachableSources;
        public final int reachableDirectKeys,reachableHeldPolls;
        public final int reachablePressPolls;
        public final int exceptionalEdges;
        private final String fingerprint;
        Report(int instructions,int jumps,int handlers,int sourceSites,
               int reachableInstructions,int reachableSources,
               int reachableDirectKeys,int reachableHeldPolls,
               int reachablePressPolls,int exceptionalEdges,String fingerprint){
            this.instructions=instructions;this.jumps=jumps;
            this.handlers=handlers;this.sourceSites=sourceSites;
            this.reachableInstructions=reachableInstructions;
            this.reachableSources=reachableSources;
            this.reachableDirectKeys=reachableDirectKeys;
            this.reachableHeldPolls=reachableHeldPolls;
            this.reachablePressPolls=reachablePressPolls;
            this.exceptionalEdges=exceptionalEdges;
            this.fingerprint=fingerprint;
        }
        public String fingerprint(){return fingerprint;}
        /** Offline static inspection cannot authorize production input hooks. */
        public boolean canAuthorizeLiveModal(){return false;}
    }
    private static String classify(int op,String owner,String name,String desc){
        if(MOUSE.equals(owner)){
            if(op==INVOKESTATIC && "next".equals(name) &&
               "()Z".equals(desc))return "MOUSE_QUEUE";
            if("getEventButtonState".equals(name))return "MOUSE_BUTTON_STATE";
            if("getEventDWheel".equals(name))return "MOUSE_WHEEL";
            return "MOUSE_OTHER";
        }
        if(KEY.equals(owner) || BRIDGED.equals(owner)){
            if(op==INVOKESTATIC && "next".equals(name) &&
               "()Z".equals(desc))return "KEYBOARD_QUEUE";
            if("isKeyDown".equals(name) && "(I)Z".equals(desc))
                return "DIRECT_KEY_DOWN";
            return "KEYBOARD_OTHER";
        }
        if("avb".equals(owner)){
            if(op==INVOKESTATIC && "a".equals(name) &&
               "(IZ)V".equals(desc))return "BIND_STATE";
            if(op==INVOKESTATIC && "a".equals(name) &&
               "(I)V".equals(desc))return "BIND_PRESS";
            if(op==INVOKEVIRTUAL && "f".equals(name) &&
               "()Z".equals(desc))return "BIND_HELD_POLL";
            if(op==INVOKEVIRTUAL && "d".equals(name) &&
               "()Z".equals(desc))return "BIND_PRESS_POLL";
            return "BIND_OTHER";
        }
        if("axu".equals(owner))return "GUI_DISPATCH";
        return null;
    }
    private static String sha(String value){
        try{
            MessageDigest md=MessageDigest.getInstance("SHA-256");
            byte[] raw=md.digest(value.getBytes(StandardCharsets.UTF_8));
            StringBuilder out=new StringBuilder(64);
            for(byte b:raw){
                int n=b&255;
                if(n<16)out.append('0');
                out.append(Integer.toHexString(n));
            }
            return out.toString();
        }catch(Exception e){throw new IllegalStateException(e);}
    }
    private static int position(Map<Label,Integer> labels,Label label,
                                int count,boolean allowsEnd){
        Integer result=labels.get(label);
        if(result==null || result<0 ||
           result>(allowsEnd?count:count-1))
            throw new IllegalArgumentException("Unresolved CFG label");
        return result;
    }
    public static Report inspect(byte[] original){
        if(original==null || original.length<32 || original.length>1000000)
            throw new IllegalArgumentException("Missing or oversized input class");
        final ClassReader reader=new ClassReader(original);
        if(!"ave".equals(reader.getClassName()))
            throw new IllegalArgumentException("Not original vanilla ave");
        final List<Node> nodes=new ArrayList<Node>();
        final Map<Label,Integer> labels=new IdentityHashMap<Label,Integer>();
        final List<Region> regions=new ArrayList<Region>();
        final int[] exact={0},jumpCount={0};
        reader.accept(new ClassVisitor(ASM5){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String sig,String[] exceptions) {
                final boolean target="s".equals(name) && "()V".equals(desc) &&
                    (access&ACC_PUBLIC)!=0 &&
                    (access&(ACC_STATIC|ACC_ABSTRACT|ACC_NATIVE))==0;
                if(target)exact[0]++;
                return new MethodVisitor(ASM5){
                    private void add(int op,String sig,String kind){
                        if(target)nodes.add(new Node(op,sig,kind));
                    }
                    @Override public void visitLabel(Label label){
                        if(target)labels.put(label,nodes.size());
                    }
                    @Override public void visitInsn(int op){
                        add(op,"",null);
                        if(target && (op==RETURN || op==ARETURN || op==IRETURN ||
                           op==LRETURN || op==DRETURN || op==FRETURN ||
                           op==ATHROW))nodes.get(nodes.size()-1).fallthrough=false;
                    }
                    @Override public void visitIntInsn(int op,int value){
                        add(op,Integer.toString(value),null);
                    }
                    @Override public void visitVarInsn(int op,int value){
                        if(target && op==RET)
                            throw new IllegalArgumentException("Legacy RET CFG unsupported");
                        add(op,Integer.toString(value),null);
                    }
                    @Override public void visitTypeInsn(int op,String type){
                        add(op,type,null);
                    }
                    @Override public void visitFieldInsn(int op,String owner,
                            String name,String desc){
                        add(op,owner+"."+name+desc,null);
                    }
                    @Override public void visitMethodInsn(int op,String owner,
                            String name,String desc,boolean itf){
                        final String kind=classify(op,owner,name,desc);
                        // Canonicalize R111's single dormant keyboard-queue
                        // substitution; no other source owner is normalized.
                        if(kind!=null && kind.equals("KEYBOARD_QUEUE"))owner=KEY;
                        add(op,owner+"."+name+desc+":"+itf,kind);
                    }
                    @Override public void visitInvokeDynamicInsn(String name,
                            String desc,org.objectweb.asm.Handle bootstrap,
                            Object... args) {
                        add(INVOKEDYNAMIC,name+desc,null);
                    }
                    @Override public void visitJumpInsn(int op,Label label){
                        if(!target)return;
                        if(op==JSR)
                            throw new IllegalArgumentException("Legacy JSR CFG unsupported");
                        Node n=new Node(op,"",null);
                        n.jumpTargets.add(label);
                        if(op==GOTO)n.fallthrough=false;
                        nodes.add(n);jumpCount[0]++;
                    }
                    @Override public void visitLdcInsn(Object value){
                        add(LDC,String.valueOf(value),null);
                    }
                    @Override public void visitIincInsn(int var,int inc){
                        add(IINC,var+":"+inc,null);
                    }
                    @Override public void visitTableSwitchInsn(int min,int max,
                            Label dflt,Label... targets){
                        if(!target)return;
                        Node n=new Node(TABLESWITCH,min+":"+max,null);
                        n.jumpTargets.add(dflt);
                        Collections.addAll(n.jumpTargets,targets);
                        n.fallthrough=false;nodes.add(n);
                    }
                    @Override public void visitLookupSwitchInsn(Label dflt,
                            int[] keys,Label[] targets){
                        if(!target)return;
                        Node n=new Node(LOOKUPSWITCH,Arrays.toString(keys),null);
                        n.jumpTargets.add(dflt);
                        Collections.addAll(n.jumpTargets,targets);
                        n.fallthrough=false;nodes.add(n);
                    }
                    @Override public void visitMultiANewArrayInsn(
                            String desc,int dimensions){
                        add(MULTIANEWARRAY,desc+":"+dimensions,null);
                    }
                    @Override public void visitTryCatchBlock(Label start,
                            Label end,Label handler,String type) {
                        if(target)regions.add(new Region(start,end,handler,type));
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        if(exact[0]!=1 || nodes.isEmpty() || nodes.size()>100000)
            throw new IllegalArgumentException("Unrecognized vanilla input method");
        int exceptionEdges=0;
        for(int i=0;i<nodes.size();i++){
            Node node=nodes.get(i);
            if(node.fallthrough && i+1<nodes.size())
                node.edges.add("N"+(i+1));
            for(Label destination:node.jumpTargets)
                node.edges.add("J"+position(labels,destination,nodes.size(),false));
        }
        for(Region region:regions){
            int from=position(labels,region.from,nodes.size(),true);
            int to=position(labels,region.to,nodes.size(),true);
            int handler=position(labels,region.handler,nodes.size(),false);
            if(from>=to)throw new IllegalArgumentException("Invalid protected region");
            for(int i=from;i<to;i++){
                nodes.get(i).edges.add("X"+handler+":"+
                    (region.type==null?"ALL":region.type));
                exceptionEdges++;
            }
        }
        final boolean[] reachable=new boolean[nodes.size()];
        final ArrayDeque<Integer> queue=new ArrayDeque<Integer>();
        reachable[0]=true;queue.addLast(0);
        while(!queue.isEmpty()){
            int index=queue.removeFirst();
            for(String edge:nodes.get(index).edges){
                int colon=edge.indexOf(':');
                int dest=Integer.parseInt(edge.substring(1,
                    colon<0?edge.length():colon));
                if(!reachable[dest]){
                    reachable[dest]=true;queue.addLast(dest);
                }
            }
        }
        int siteCount=0,reachableCount=0,reachableSites=0,direct=0,held=0,press=0;
        StringBuilder transcript=new StringBuilder();
        transcript.append("R163CFG1|n=").append(nodes.size())
            .append("|branches=").append(jumpCount[0])
            .append("|handlers=").append(regions.size()).append('\n');
        for(int i=0;i<nodes.size();i++){
            Node n=nodes.get(i);
            if(reachable[i])reachableCount++;
            if(n.kind!=null){
                siteCount++;
                if(reachable[i]){
                    reachableSites++;
                    if(n.kind.equals("DIRECT_KEY_DOWN"))direct++;
                    if(n.kind.equals("BIND_HELD_POLL"))held++;
                    if(n.kind.equals("BIND_PRESS_POLL"))press++;
                }
            }
            Collections.sort(n.edges);
            transcript.append(i).append('|').append(n.opcode)
                .append('|').append(n.signature).append('|')
                .append(n.kind==null?"":n.kind).append('|');
            for(String edge:n.edges)transcript.append(edge).append(',');
            transcript.append('\n');
        }
        return new Report(nodes.size(),jumpCount[0],regions.size(),siteCount,
            reachableCount,reachableSites,direct,held,press,exceptionEdges,
            sha(transcript.toString()));
    }
    /** Exact original-game SHA and independent ordered callsite proof first. */
    public static Report pinned(byte[] original) {
        R162InputOrderLedger.pinned(original);
        Report graph=inspect(original);
        if(!PINNED_CFG_SHA256.equals(graph.fingerprint()) ||
           graph.instructions!=1009 || graph.jumps!=179 ||
           graph.handlers!=3 || graph.sourceSites!=58 ||
           graph.exceptionalEdges!=9 || graph.reachableSources!=58 ||
           graph.reachableDirectKeys!=17 ||
           graph.reachableHeldPolls!=13 ||
           graph.reachablePressPolls!=3)
            throw new IllegalArgumentException(
                "R163 original Minecraft 1.8.9 input CFG changed");
        return graph;
    }
}
