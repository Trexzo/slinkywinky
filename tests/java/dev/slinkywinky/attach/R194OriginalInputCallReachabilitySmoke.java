package dev.slinkywinky.attach;

import java.util.Arrays;
import java.nio.file.Files;
import java.nio.file.Paths;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** R194 disposable Java 8/ASM 5.2 negative controls; no game launched. */
public final class R194OriginalInputCallReachabilitySmoke implements Opcodes {
    private static void require(boolean ok,String why) {
        if(!ok)throw new AssertionError("R194 "+why);
    }
    private static byte[] node(String owner,String next,String target) {
        ClassWriter cw=new ClassWriter(0);
        cw.visit(V1_8,ACC_PUBLIC|ACC_FINAL,owner,null,
                 "java/lang/Object",null);
        MethodVisitor mv=cw.visitMethod(
            ACC_PUBLIC|ACC_STATIC,"run","()V",null,null);
        mv.visitCode();
        if(next!=null){
            mv.visitMethodInsn(INVOKESTATIC,next,"run","()V",false);
        } else {
            mv.visitInsn(ICONST_0);
            mv.visitMethodInsn(INVOKESTATIC,target,
                "isButtonDown","(I)Z",false);
            mv.visitInsn(POP);
        }
        mv.visitInsn(RETURN);
        mv.visitMaxs(1,0);
        mv.visitEnd();
        cw.visitEnd();
        return cw.toByteArray();
    }
    // Deliberate under-approximation: a virtual call declared on Base
    // may at runtime dispatch to Sub.run() (which reads Mouse). Our
    // exact-declared-target graph does NOT pretend to resolve this edge.
    private static byte[] virtualClass(String owner,String parent,
                                       boolean mouseReader,boolean caller){
        ClassWriter cw=new ClassWriter(0);
        cw.visit(V1_8,ACC_PUBLIC,owner,null,parent,null);
        MethodVisitor mv=cw.visitMethod(
            caller?ACC_PUBLIC|ACC_STATIC:ACC_PUBLIC,
            "run","()V",null,null);
        mv.visitCode();
        if(caller) {
            mv.visitInsn(ACONST_NULL);
            mv.visitMethodInsn(INVOKEVIRTUAL,
                "r194/Base","run","()V",false);
        } else if(mouseReader) {
            mv.visitInsn(ICONST_0);
            mv.visitMethodInsn(INVOKESTATIC,
                "org/lwjgl/input/Mouse","isButtonDown","(I)Z",false);
            mv.visitInsn(POP);
        }
        mv.visitInsn(RETURN);
        mv.visitMaxs(1,caller?0:1);
        mv.visitEnd();
        cw.visitEnd();
        return cw.toByteArray();
    }
    private static void virtualDispatchGap() {
        R194OriginalInputCallReachability.Graph graph=
            R194OriginalInputCallReachability.inspect(Arrays.asList(
                virtualClass("r194/Base","java/lang/Object",false,false),
                virtualClass("r194/Sub","r194/Base",true,false),
                virtualClass("r194/Call","java/lang/Object",false,true)));
        R194OriginalInputCallReachability.Reachability r=
            graph.reachability();
        require(graph.virtualCallsites()==1 &&
                graph.directCallsites()==1,
                "synthetic virtual dispatch gap shape");
        require(r.distance("r194/Sub","run","()V")==0 &&
                r.distance("r194/Call","run","()V")==-1,
                "declared method graph is not full override dispatch");
        require(!r.proofOfExclusiveInput(),
                "virtual unresolved blocks ownership certification");
        System.out.println(
            "R194_VIRTUAL_DISPATCH_UNRESOLVED_NEGATIVE_CONTROL_PASS");
    }
    private static void synthetic() {
        byte[] leaf=node("r194/Leaf",null,
                         "org/lwjgl/input/Mouse");
        byte[] parent=node("r194/Parent","r194/Leaf",null);
        byte[] grand=node("r194/Grand","r194/Parent",null);
        R194OriginalInputCallReachability.Graph graph=
            R194OriginalInputCallReachability.inspect(
                Arrays.asList(grand,leaf,parent));
        R194OriginalInputCallReachability.Reachability reaches=
            graph.reachability();
        require(graph.classCount()==3 && graph.directCallsites()==1,
                "only the leaf calls Mouse");
        require(graph.directConsumerMethods()==1 &&
                reaches.exposedMethods()==3,
                "transitive input exposes two upstream methods");
        require(reaches.distance("r194/Leaf","run","()V")==0 &&
                reaches.distance("r194/Parent","run","()V")==1 &&
                reaches.distance("r194/Grand","run","()V")==2,
                "declared call graph reverse BFS");
        require(!reaches.proofOfExclusiveInput(),
                "static graph cannot grant original input ownership");
        R194OriginalInputCallReachability.Reachability reorder=
            R194OriginalInputCallReachability.inspect(
                Arrays.asList(parent,grand,leaf)).reachability();
        require(reaches.fingerprint.equals(reorder.fingerprint),
                "class-order independent graph receipt");
        byte[] changed=node("r194/Leaf",null,"r194/OtherMouse");
        R194OriginalInputCallReachability.Graph tampered=
            R194OriginalInputCallReachability.inspect(
                Arrays.asList(grand,parent,changed));
        require(tampered.directCallsites()==0 &&
                tampered.reachability().exposedMethods()==0 &&
                !reaches.fingerprint.equals(
                    tampered.reachability().fingerprint),
                "leaf input mutation must revoke ALL propagated callers");
        // A declaration-only call to an external class never fabricates
        // an internal callee body. Unknown targets are a coverage gap.
        byte[] unresolved=node("r194/Unresolved","r194/Missing",null);
        R194OriginalInputCallReachability.Reachability external=
            R194OriginalInputCallReachability.inspect(
                Arrays.asList(leaf,unresolved)).reachability();
        require(external.exposedMethods()==1 &&
                external.externalDeclaredTargets>0,
                "external call targets are not magically resolved");
        System.out.println("R194_TWO_HOP_ORIGINAL_INPUT_NEGATIVE_CONTROL_PASS");
        System.out.println("R194_UNRESOLVED_EXTERNAL_CALLEE_NO_FALSE_EDGE_PASS");
        System.out.println("R194_INDEPENDENT_CLASS_ORDER_FINGERPRINT_PASS");
    }
    public static void main(String[] args)throws Exception {
        synthetic();
        virtualDispatchGap();
        if(args.length==0)return;
        require(args.length==1,"accept only original JAR path");
        byte[] before=Files.readAllBytes(Paths.get(args[0]));
        R194OriginalInputCallReachability.Graph pinned=
            R194OriginalInputCallReachability.inspectPinnedJar(args[0]);
        R194OriginalInputCallReachability.Reachability reachable=
            pinned.reachability();
        require(pinned.classCount()==2507 &&
                pinned.directCallsites()==146 &&
                pinned.directConsumerMethods()>2,
                "exact pinned vanilla full graph");
        require(pinned.methodCount()==19766 &&
                pinned.directConsumerMethods()==56 &&
                reachable.exposedMethods()==237 &&
                reachable.oneHopMethods==46 &&
                reachable.twoOrMoreHopMethods==135 &&
                reachable.maximumDepth==6 &&
                "f0b1cb8aee451b56302a0035ec92665829c774a251718b4e00f61852d9e3bdfd"
                    .equals(reachable.fingerprint),
                "R194 source graph method coverage or SHA-256 drift");
        require(!reachable.proofOfExclusiveInput(),
                "original source read cannot certify exclusivity");
        byte[] after=Files.readAllBytes(Paths.get(args[0]));
        require(Arrays.equals(before,after),
                "original Mojang JAR must remain byte-identical");
        System.out.println("R194_EXACT_PINNED_JAR_GRAPH_PASS");
        System.out.println("R194_GRAPH_METHODS "+pinned.methodCount());
        System.out.println("R194_GRAPH_CALLS internal="+reachable.internalEdges+
                           " external="+reachable.externalDeclaredTargets);
        System.out.println("R194_GRAPH_REACHABLE direct="+
            pinned.directConsumerMethods()+" exposed="+
            reachable.exposedMethods()+" one="+reachable.oneHopMethods+
            " twoPlus="+reachable.twoOrMoreHopMethods+
            " depth="+reachable.maximumDepth);
        System.out.println("R194_GRAPH_SHA256 "+reachable.fingerprint);
        System.out.println("R194_MAIN_INPUT_DISTANCE "+
            reachable.distance("ave","s","()V"));
        System.out.println("R194_NOT_VIRTUAL_OR_HARDWARE_DISPATCH_PROOF");
    }
}
