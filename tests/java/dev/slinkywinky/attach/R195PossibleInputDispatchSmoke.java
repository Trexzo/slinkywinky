package dev.slinkywinky.attach;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.Arrays;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/** R195 source-only synthetic call graph fixtures. No JVM game or Attach. */
public final class R195PossibleInputDispatchSmoke implements Opcodes {
    private R195PossibleInputDispatchSmoke(){}
    private static void check(boolean ok,String description){
        if(!ok)throw new AssertionError("R195 "+description);
    }
    private static byte[] cls(String owner,String parent,String[] interfaces,
                              boolean iface,boolean abstractMethod,
                              boolean readMouse,int callKind,
                              String declared) {
        ClassWriter cw=new ClassWriter(0);
        int flags=ACC_PUBLIC|(iface?(ACC_INTERFACE|ACC_ABSTRACT):0);
        cw.visit(V1_8,flags,owner,null,parent,interfaces);
        if(abstractMethod){
            cw.visitMethod(ACC_PUBLIC|ACC_ABSTRACT,"run","()V",null,null)
                .visitEnd();
        } else {
            MethodVisitor mv=cw.visitMethod(
                ACC_PUBLIC|(callKind!=0?ACC_STATIC:0),
                "run","()V",null,null);
            mv.visitCode();
            if(readMouse){
                mv.visitInsn(ICONST_0);
                mv.visitMethodInsn(INVOKESTATIC,
                    "org/lwjgl/input/Mouse","isButtonDown","(I)Z",false);
                mv.visitInsn(POP);
            }
            if(callKind!=0){
                mv.visitInsn(ACONST_NULL);
                mv.visitMethodInsn(callKind,declared,
                    "run","()V",callKind==INVOKEINTERFACE);
            }
            mv.visitInsn(RETURN);
            mv.visitMaxs(1,0);
            mv.visitEnd();
        }
        cw.visitEnd();
        return cw.toByteArray();
    }
    private static void synthetic(){
        byte[] base=cls("r195/Base","java/lang/Object",new String[0],
            false,false,false,0,null);
        byte[] override=cls("r195/Override","r195/Base",new String[0],
            false,false,true,0,null);
        byte[] safe=cls("r195/Safe","r195/Base",new String[0],
            false,false,false,0,null);
        byte[] call=cls("r195/Caller","java/lang/Object",new String[0],
            false,false,false,INVOKEVIRTUAL,"r195/Base");
        R195PossibleInputDispatch.Graph concrete=
            R195PossibleInputDispatch.inspect(
                Arrays.asList(call,safe,override,base));
        R195PossibleInputDispatch.Report result=concrete.expand();
        check(concrete.directInputSites()==1 &&
                concrete.directInputMethods()==1,
                "one direct Mouse-reading method");
        check(result.depth("r195/Override","run","()V")==0 &&
                result.depth("r195/Caller","run","()V")==1,
                "virtual Base call expands to Mouse-reading Override");
        check(result.expandedEdges>0 && result.expandedVirtualSites>0,
                "real override edge added beyond declared target");
        check(result.depth("r195/Safe","run","()V")==-1,
                "unrelated non-reading override not a direct input root");
        check(!result.licensesExclusiveLiveInput(),
                "possible dispatch does not grant input interception");
        R195PossibleInputDispatch.Report reverse=
            R195PossibleInputDispatch.inspect(
                Arrays.asList(base,override,safe,call)).expand();
        check(result.fingerprint.equals(reverse.fingerprint),
                "class-file order must not change graph digest");
        byte[] removed=cls("r195/Override","r195/Base",new String[0],
            false,false,false,0,null);
        R195PossibleInputDispatch.Report noMouse=
            R195PossibleInputDispatch.inspect(
                Arrays.asList(base,safe,removed,call)).expand();
        check(noMouse.reachableMethods()==0 &&
                !noMouse.fingerprint.equals(result.fingerprint),
                "removing Mouse access removes the virtual caller");
        byte[] api=cls("r195/Api","java/lang/Object",new String[0],
            true,true,false,0,null);
        byte[] impl=cls("r195/Impl","java/lang/Object",
            new String[]{"r195/Api"},false,false,true,0,null);
        byte[] ifCaller=cls("r195/InterfaceCaller","java/lang/Object",
            new String[0],false,false,false,INVOKEINTERFACE,"r195/Api");
        R195PossibleInputDispatch.Graph ifaceGraph=
            R195PossibleInputDispatch.inspect(Arrays.asList(
                api,impl,ifCaller));
        R195PossibleInputDispatch.Report ifaceResult=ifaceGraph.expand();
        check(ifaceGraph.invokeInterfaceSites()==1 &&
                ifaceResult.depth("r195/Impl","run","()V")==0 &&
                ifaceResult.depth("r195/InterfaceCaller","run","()V")==1,
                "interface call expands to known concrete implementation");
        byte[] missing=cls("r195/UnknownCaller","java/lang/Object",
            new String[0],false,false,false,
            INVOKEINTERFACE,"r195/OutsideLibrary");
        R195PossibleInputDispatch.Report unknown=
            R195PossibleInputDispatch.inspect(Arrays.asList(impl,missing))
                .expand();
        check(unknown.virtualWithoutKnownReceiver>0 &&
                unknown.externalDeclaredTargets>0 &&
                unknown.depth("r195/UnknownCaller","run","()V")==-1,
                "external declaration cannot manufacture unknown hierarchy");
        System.out.println("R195_CLASS_OVERRIDE_POSSIBLE_INPUT_EDGE_PASS");
        System.out.println("R195_INTERFACE_POSSIBLE_INPUT_EDGE_PASS");
        System.out.println("R195_SOURCE_ORDER_AND_MUTATION_NEGATIVE_CONTROLS_PASS");
        System.out.println("R195_EXTERNAL_CLASS_HIERARCHY_GAP_IS_UNRESOLVED");
    }
    public static void main(String[] args)throws Exception {
        synthetic();
        if(args.length==0)return;
        check(args.length==1,"Require exact pinned Mojang 1.8.9 JAR");
        byte[] before=Files.readAllBytes(Paths.get(args[0]));
        R194OriginalInputCallReachability.Graph prior=
            R194OriginalInputCallReachability.inspectPinnedJar(args[0]);
        R194OriginalInputCallReachability.Reachability declared=
            prior.reachability();
        check(prior.methodCount()==19766 &&
                declared.exposedMethods()==237 &&
                "f0b1cb8aee451b56302a0035ec92665829c774a251718b4e00f61852d9e3bdfd"
                  .equals(declared.fingerprint),
                "R194 baseline drift");
        R195PossibleInputDispatch.Graph graph=
            R195PossibleInputDispatch.pinned(args[0]);
        R195PossibleInputDispatch.Report report=graph.expand();
        check(graph.classes()==2507 && graph.methods()==19766 &&
                graph.directInputSites()==146 &&
                graph.directInputMethods()==56,
                "source-pinned class and input counts");
        check(report.reachableMethods()==6655 &&
                report.oneHop==53 && report.twoPlus==6546 &&
                report.maxDepth==21 &&
                report.expandedEdges==39705 &&
                report.expandedVirtualSites==11540 &&
                "3c8399abdc065c43d933e14355b6835675898e2386851054940fa006ef60fbfe"
                  .equals(report.fingerprint) &&
                report.reachableMethods()>=declared.exposedMethods() &&
                !report.licensesExclusiveLiveInput(),
                "R195 source-pinned possible graph drift or unsafe ownership");
        check(Arrays.equals(before,Files.readAllBytes(Paths.get(args[0]))),
                "original classfile bytes changed");
        System.out.println("R195_PINNED_ORIGINAL_HIERARCHY_SCAN_PASS");
        System.out.println("R195_PINNED_DISPATCH_SHA256 "+report.fingerprint);
        System.out.println("R195_PINNED_POSSIBLE_METHODS "+report.reachableMethods());
        System.out.println("R195_PINNED_POSSIBLE_VIRTUAL_EDGE_EXPANSION "+
            report.expandedEdges);
        System.out.println("R195_PINNED_ORIGINAL_BYTES_UNMODIFIED_PASS");
        System.out.println("R195_NOT_RUNTIME_INPUT_OWNERSHIP_OR_GAMEPLAY_PROOF");
    }
}
