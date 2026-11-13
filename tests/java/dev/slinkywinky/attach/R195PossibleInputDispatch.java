package dev.slinkywinky.attach;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Enumeration;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R195 OFFLINE: source-pinned conservative possible dispatch expansion.
 * A reachable caller is not evidence that its code executes in Minecraft.
 * Incomplete hierarchy, JNI, reflection, invokedynamic and OS input prevent
 * an exclusivity guarantee. No agent, no game hooks, no original mutation.
 */
public final class R195PossibleInputDispatch implements Opcodes {
    private R195PossibleInputDispatch(){}

    private static String sig(String name,String desc) {
        return name+desc;
    }
    private static String method(String type,String name,String desc) {
        return type+"."+sig(name,desc);
    }
    private static final class Info {
        final String name,superName;
        final List<String> interfaces;
        final int flags;
        final Map<String,Integer> methods=new HashMap<String,Integer>();
        Info(String name,String parent,String[] interfaces,int flags){
            this.name=name;this.superName=parent;
            this.interfaces=Arrays.asList(interfaces.clone());
            this.flags=flags;
        }
        boolean concrete() {
            return (flags&(ACC_INTERFACE|ACC_ABSTRACT))==0;
        }
    }
    private static final class Call {
        final String from,owner,name,desc;
        final int op;
        Call(String from,int op,String owner,String name,String desc){
            this.from=from;this.owner=owner;this.name=name;
            this.desc=desc;this.op=op;
        }
    }
    public static final class Graph {
        private final Map<String,Info> info=new HashMap<String,Info>();
        private final Set<String> allMethods=new HashSet<String>();
        private final List<Call> calls=new ArrayList<Call>();
        private final Set<String> roots=new HashSet<String>();
        private int inputSites,virtualSites,interfaceSites,dynamicSites;

        private void add(byte[] bytes) {
            if(bytes==null || bytes.length<32 || bytes.length>1000000)
                throw new IllegalArgumentException("Invalid or oversized class");
            final ClassReader cr=new ClassReader(bytes);
            final String type=cr.getClassName();
            if(info.containsKey(type))
                throw new IllegalArgumentException("Duplicate class "+type);
            cr.accept(new ClassVisitor(ASM5) {
                private Info current;
                @Override public void visit(int version,int flags,
                        String name,String signature,String parent,
                        String[] interfaces) {
                    current=new Info(name,parent,
                        interfaces==null?new String[0]:interfaces,flags);
                    info.put(name,current);
                }
                @Override public MethodVisitor visitMethod(int flags,
                        final String name,final String desc,
                        String signature,String[] errors) {
                    final String id=method(type,name,desc);
                    current.methods.put(sig(name,desc),Integer.valueOf(flags));
                    allMethods.add(id);
                    if((flags&(ACC_ABSTRACT|ACC_NATIVE))!=0)return null;
                    return new MethodVisitor(ASM5) {
                        @Override public void visitMethodInsn(int opcode,
                                String owner,String target,String descriptor,
                                boolean iface) {
                            calls.add(new Call(id,opcode,owner,target,descriptor));
                            if(opcode==INVOKEVIRTUAL)virtualSites++;
                            if(opcode==INVOKEINTERFACE)interfaceSites++;
                            if(!iface &&
                               R171VanillaJarInputSurface.kind(opcode,
                                   owner,target,descriptor)!=null) {
                                inputSites++;
                                roots.add(id);
                            }
                        }
                        @Override public void visitInvokeDynamicInsn(
                                String name,String descriptor,
                                org.objectweb.asm.Handle handle,
                                Object... arguments) {
                            dynamicSites++;
                        }
                    };
                }
            },ClassReader.SKIP_FRAMES|ClassReader.SKIP_DEBUG);
        }
        public int classes(){return info.size();}
        public int methods(){return allMethods.size();}
        public int directInputSites(){return inputSites;}
        public int directInputMethods(){return roots.size();}
        public int invokeVirtualSites(){return virtualSites;}
        public int invokeInterfaceSites(){return interfaceSites;}
        public int invokedynamicSites(){return dynamicSites;}

        private Map<String,Set<String>> possibleReceivers(){
            Map<String,Set<String>> result=new HashMap<String,Set<String>>();
            for(Info concrete:info.values()){
                if(!concrete.concrete())continue;
                Set<String> visited=new HashSet<String>();
                ArrayDeque<String> queue=new ArrayDeque<String>();
                queue.add(concrete.name);
                while(!queue.isEmpty()){
                    String next=queue.removeFirst();
                    if(!visited.add(next))continue;
                    Set<String> receivers=result.get(next);
                    if(receivers==null){
                        receivers=new HashSet<String>();
                        result.put(next,receivers);
                    }
                    receivers.add(concrete.name);
                    Info cls=info.get(next);
                    if(cls==null)continue;
                    if(cls.superName!=null)queue.addLast(cls.superName);
                    for(String i:cls.interfaces)queue.addLast(i);
                }
            }
            return result;
        }
        private String resolveClassMethod(String receiver,String signature){
            Set<String> seen=new HashSet<String>();
            String cur=receiver;
            while(cur!=null && seen.add(cur)){
                Info cls=info.get(cur);
                if(cls==null)return null;
                Integer flags=cls.methods.get(signature);
                if(flags!=null &&
                   (flags.intValue()&(ACC_STATIC|ACC_PRIVATE|ACC_ABSTRACT))==0)
                    return cur+"."+signature;
                cur=cls.superName;
            }
            return null;
        }
        // Default interface methods are additional possible targets, not
        // proofs of one selected implementation; ambiguous defaults remain.
        private Set<String> defaults(String receiver,String signature){
            Set<String> found=new HashSet<String>();
            Set<String> seen=new HashSet<String>();
            ArrayDeque<String> todo=new ArrayDeque<String>();
            todo.add(receiver);
            while(!todo.isEmpty()){
                String type=todo.removeFirst();
                if(!seen.add(type))continue;
                Info cls=info.get(type);
                if(cls==null)continue;
                Integer flags=cls.methods.get(signature);
                if((cls.flags&ACC_INTERFACE)!=0 && flags!=null &&
                   (flags.intValue()&(ACC_ABSTRACT|ACC_STATIC|ACC_PRIVATE))==0)
                    found.add(type+"."+signature);
                if(cls.superName!=null)todo.addLast(cls.superName);
                for(String i:cls.interfaces)todo.addLast(i);
            }
            return found;
        }
        /** Possible targets under *known* concrete classes only. */
        public Report expand(){
            Map<String,Set<String>> receivers=possibleReceivers();
            Map<String,Set<String>> edges=new HashMap<String,Set<String>>();
            int exactEdges=0,expandedEdges=0,noKnownReceiver=0;
            int unresolvableTarget=0,unknownDeclaredTarget=0;
            int virtualWithExtra=0;
            for(Call call:calls){
                Set<String> targets=edges.get(call.from);
                if(targets==null){
                    targets=new HashSet<String>();
                    edges.put(call.from,targets);
                }
                final String declared=method(
                    call.owner,call.name,call.desc);
                if(allMethods.contains(declared)) {
                    if(targets.add(declared))exactEdges++;
                } else unknownDeclaredTarget++;
                if(call.op!=INVOKEVIRTUAL &&
                   call.op!=INVOKEINTERFACE)continue;
                Set<String> concrete=receivers.get(call.owner);
                if(concrete==null || concrete.isEmpty()){
                    noKnownReceiver++;
                    continue;
                }
                boolean added=false;
                String signature=sig(call.name,call.desc);
                for(String receiver:concrete){
                    String actual=resolveClassMethod(receiver,signature);
                    if(actual!=null){
                        if(targets.add(actual)){
                            expandedEdges++;
                            added=true;
                        }
                        continue;
                    }
                    Set<String> possibleDefaults=defaults(receiver,signature);
                    if(possibleDefaults.isEmpty())unresolvableTarget++;
                    for(String defaultMethod:possibleDefaults){
                        if(targets.add(defaultMethod)){
                            expandedEdges++;
                            added=true;
                        }
                    }
                }
                if(added)virtualWithExtra++;
            }
            Map<String,Set<String>> callers=new HashMap<String,Set<String>>();
            int internalEdges=0;
            for(Map.Entry<String,Set<String>> entry:edges.entrySet()){
                for(String to:entry.getValue()){
                    if(!allMethods.contains(to))continue;
                    internalEdges++;
                    Set<String> parents=callers.get(to);
                    if(parents==null){
                        parents=new HashSet<String>();
                        callers.put(to,parents);
                    }
                    parents.add(entry.getKey());
                }
            }
            Map<String,Integer> distances=new HashMap<String,Integer>();
            ArrayDeque<String> next=new ArrayDeque<String>();
            for(String root:roots){
                distances.put(root,Integer.valueOf(0));
                next.addLast(root);
            }
            while(!next.isEmpty()){
                String seen=next.removeFirst();
                int depth=distances.get(seen).intValue()+1;
                Set<String> parents=callers.get(seen);
                if(parents==null)continue;
                for(String parent:parents){
                    if(distances.containsKey(parent))continue;
                    distances.put(parent,Integer.valueOf(depth));
                    next.addLast(parent);
                }
            }
            int oneHop=0,twoPlus=0,maxDepth=0;
            TreeSet<String> transcript=new TreeSet<String>();
            for(Map.Entry<String,Integer> e:distances.entrySet()){
                int distance=e.getValue().intValue();
                if(distance==1)oneHop++;
                if(distance>=2)twoPlus++;
                maxDepth=Math.max(maxDepth,distance);
                transcript.add(e.getKey()+"|"+distance);
            }
            StringBuilder fingerprint=new StringBuilder();
            for(String line:transcript)
                fingerprint.append(line).append('\n');
            return new Report(
                Collections.unmodifiableMap(distances),exactEdges,
                expandedEdges,internalEdges,unknownDeclaredTarget,
                noKnownReceiver,unresolvableTarget,virtualWithExtra,
                oneHop,twoPlus,maxDepth,
                R171VanillaJarInputSurface.sha("SHA-256",
                    fingerprint.toString().getBytes(
                        java.nio.charset.StandardCharsets.UTF_8)));
        }
    }
    public static final class Report {
        private final Map<String,Integer> distance;
        public final int exactEdges,expandedEdges,internalEdges;
        public final int externalDeclaredTargets,virtualWithoutKnownReceiver;
        public final int unresolvedConcreteImplementations,expandedVirtualSites;
        public final int oneHop,twoPlus,maxDepth;
        public final String fingerprint;
        Report(Map<String,Integer> reachability,
               int exact,int extra,int inside,int external,
               int noReceiver,int unresolved,int expandedSites,
               int one,int more,int depth,String digest) {
            distance=reachability;exactEdges=exact;expandedEdges=extra;
            internalEdges=inside;externalDeclaredTargets=external;
            virtualWithoutKnownReceiver=noReceiver;
            unresolvedConcreteImplementations=unresolved;
            expandedVirtualSites=expandedSites;oneHop=one;twoPlus=more;
            maxDepth=depth;fingerprint=digest;
        }
        public int reachableMethods(){return distance.size();}
        public int depth(String owner,String name,String desc){
            Integer n=distance.get(method(owner,name,desc));
            return n==null?-1:n.intValue();
        }
        public boolean licensesExclusiveLiveInput(){return false;}
    }
    public static Graph inspect(List<byte[]> classes){
        if(classes==null || classes.isEmpty() || classes.size()>15000)
            throw new IllegalArgumentException("Missing/oversized inventory");
        Graph graph=new Graph();
        for(byte[] raw:classes)graph.add(raw);
        return graph;
    }
    public static Graph pinned(String path)throws Exception {
        R171VanillaJarInputSurface.Report source=
            R171VanillaJarInputSurface.inspectPinnedJar(path);
        if(source.classesInspected!=2507 || source.sites.size()!=146 ||
           !"b029538b90e0e23947c9224779028679f0faced032f5a469b07f69b8859dd11e"
            .equals(source.fingerprint))
            throw new IllegalArgumentException("R171 original pin mismatch");
        Path file=Paths.get(path);
        List<byte[]> classes=new ArrayList<byte[]>();
        try(ZipFile jar=new ZipFile(file.toFile())){
            Enumeration<? extends ZipEntry> e=jar.entries();
            while(e.hasMoreElements()){
                ZipEntry entry=e.nextElement();
                if(entry.isDirectory() ||
                   !entry.getName().endsWith(".class"))continue;
                classes.add(RenderCallsiteAudit.entry(
                    jar,entry.getName()));
            }
        }
        Graph graph=inspect(classes);
        if(graph.classes()!=2507 || graph.directInputSites()!=146)
            throw new IllegalArgumentException("R195 source diverges from R171");
        return graph;
    }
    public static void main(String[] args)throws Exception {
        if(args.length!=1)throw new IllegalArgumentException(
            "Require exact Mojang 1.8.9 JAR");
        Graph graph=pinned(args[0]);
        Report report=graph.expand();
        System.out.println("R195_PINNED_ORIGINAL_POSSIBLE_DISPATCH_PASS"+
            " classes="+graph.classes()+" methods="+graph.methods()+
            " directSites="+graph.directInputSites()+
            " directMethods="+graph.directInputMethods());
        System.out.println("R195_POSSIBLE_DISPATCH_REACHABILITY"+
            " methods="+report.reachableMethods()+
            " oneHop="+report.oneHop+" twoPlus="+report.twoPlus+
            " maxDepth="+report.maxDepth);
        System.out.println("R195_DISPATCH_EDGE_COVERAGE"+
            " exact="+report.exactEdges+
            " expanded="+report.expandedEdges+
            " expandedSites="+report.expandedVirtualSites+
            " noReceiver="+report.virtualWithoutKnownReceiver+
            " unresolvedImpl="+report.unresolvedConcreteImplementations+
            " missingDeclared="+report.externalDeclaredTargets);
        System.out.println("R195_POSSIBLE_GRAPH_SHA256 "+report.fingerprint);
        System.out.println("R195_NO_DYNAMIC_RUNTIME_EVENT_OWNERSHIP_PROOF");
    }
}
