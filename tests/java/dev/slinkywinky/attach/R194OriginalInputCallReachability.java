package dev.slinkywinky.attach;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayDeque;
import java.util.ArrayList;
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
 * R194 OFFLINE exact-declared-target method reachability, based on the
 * source-pinned R171 direct input inventory. No agent, gameplay, injection.
 * This is NOT a complete dynamic/virtual dispatch graph. No method body
 * execution or original key state can be inferred from static calls alone.
 */
public final class R194OriginalInputCallReachability implements Opcodes {
    private R194OriginalInputCallReachability() {}

    public static final class Graph {
        private final Set<String> methods=new HashSet<String>();
        private final Map<String,Set<String>> callEdges=
            new HashMap<String,Set<String>>();
        private final Set<String> roots=new HashSet<String>();
        private int matchingSites,virtualCalls,indynamicCalls,classes;
        private void accept(byte[] raw) {
            if(raw==null || raw.length<32 || raw.length>1000000)
                throw new IllegalArgumentException("Invalid class data");
            ClassReader cr=new ClassReader(raw);
            final String owner=cr.getClassName();
            classes++;
            cr.accept(new ClassVisitor(ASM5){
                @Override public MethodVisitor visitMethod(
                        int flags,final String method,final String desc,
                        String sig,String[] errors){
                    final String id=key(owner,method,desc);
                    methods.add(id);
                    if((flags&(ACC_ABSTRACT|ACC_NATIVE))!=0)return null;
                    return new MethodVisitor(ASM5) {
                        @Override public void visitMethodInsn(
                                int opcode,String target,String name,
                                String type,boolean iface) {
                            if(opcode==INVOKEVIRTUAL ||
                               opcode==INVOKEINTERFACE)virtualCalls++;
                            Set<String> to=callEdges.get(id);
                            if(to==null) {
                                to=new HashSet<String>();
                                callEdges.put(id,to);
                            }
                            to.add(key(target,name,type));
                            String family=R171VanillaJarInputSurface.kind(
                                opcode,target,name,type);
                            if(family!=null && !iface) {
                                roots.add(id);
                                matchingSites++;
                            }
                        }
                        @Override public void visitInvokeDynamicInsn(
                                String name,String desc,
                                org.objectweb.asm.Handle handle,
                                Object... arguments) {
                            indynamicCalls++;
                        }
                    };
                }
            },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        }
        public int classCount(){return classes;}
        public int methodCount(){return methods.size();}
        public int directCallsites(){return matchingSites;}
        public int directConsumerMethods(){return roots.size();}
        public int virtualCallsites(){return virtualCalls;}
        public int invokeDynamicCallsites(){return indynamicCalls;}

        /** Exact declared target only; subclasses/dispatch not resolved. */
        public Reachability reachability() {
            Map<String,Set<String>> callers=new HashMap<String,Set<String>>();
            int internalEdges=0,externalEdges=0;
            for(Map.Entry<String,Set<String>> entry:callEdges.entrySet()){
                for(String callee:entry.getValue()){
                    if(!methods.contains(callee)){externalEdges++;continue;}
                    internalEdges++;
                    Set<String> reverse=callers.get(callee);
                    if(reverse==null){
                        reverse=new HashSet<String>();
                        callers.put(callee,reverse);
                    }
                    reverse.add(entry.getKey());
                }
            }
            Map<String,Integer> levels=new HashMap<String,Integer>();
            ArrayDeque<String> todo=new ArrayDeque<String>();
            for(String root:roots){
                levels.put(root,Integer.valueOf(0));
                todo.add(root);
            }
            while(!todo.isEmpty()) {
                String current=todo.removeFirst();
                int next=levels.get(current).intValue()+1;
                Set<String> parents=callers.get(current);
                if(parents==null)continue;
                for(String parent:parents){
                    if(levels.containsKey(parent))continue;
                    levels.put(parent,Integer.valueOf(next));
                    todo.addLast(parent);
                }
            }
            int maxDepth=0,oneHop=0,twoPlus=0;
            TreeSet<String> transcript=new TreeSet<String>();
            for(Map.Entry<String,Integer> entry:levels.entrySet()){
                int depth=entry.getValue().intValue();
                if(depth>maxDepth)maxDepth=depth;
                if(depth==1)oneHop++;
                if(depth>=2)twoPlus++;
                transcript.add(entry.getKey()+"|"+depth);
            }
            StringBuilder bytes=new StringBuilder();
            for(String line:transcript)bytes.append(line).append('\n');
            return new Reachability(
                Collections.unmodifiableMap(levels),
                internalEdges,externalEdges,maxDepth,oneHop,twoPlus,
                R171VanillaJarInputSurface.sha("SHA-256",
                    bytes.toString().getBytes(
                        java.nio.charset.StandardCharsets.UTF_8)));
        }
    }
    public static final class Reachability {
        private final Map<String,Integer> levels;
        public final int internalEdges,externalDeclaredTargets;
        public final int maximumDepth,oneHopMethods,twoOrMoreHopMethods;
        public final String fingerprint;
        Reachability(Map<String,Integer> levels,int inside,int outside,
                     int depth,int one,int more,String sha) {
            this.levels=levels;internalEdges=inside;
            externalDeclaredTargets=outside;maximumDepth=depth;
            oneHopMethods=one;twoOrMoreHopMethods=more;fingerprint=sha;
        }
        public int exposedMethods(){return levels.size();}
        public boolean includes(String owner,String method,String desc) {
            return levels.containsKey(key(owner,method,desc));
        }
        public int distance(String owner,String method,String desc) {
            Integer result=levels.get(key(owner,method,desc));
            return result==null?-1:result.intValue();
        }
        public boolean proofOfExclusiveInput(){return false;}
    }
    private static String key(String owner,String name,String desc){
        return owner+"."+name+desc;
    }
    public static Graph inspect(List<byte[]> classes){
        if(classes==null || classes.isEmpty() || classes.size()>15000)
            throw new IllegalArgumentException("Missing class inventory");
        Graph graph=new Graph();
        for(byte[] raw:classes)graph.accept(raw);
        return graph;
    }
    public static Graph inspectPinnedJar(String jarPath)throws Exception {
        // Reuse R171's byte-exact SHA-1, size, count and direct site audit.
        final R171VanillaJarInputSurface.Report pinned=
            R171VanillaJarInputSurface.inspectPinnedJar(jarPath);
        if(pinned.classesInspected!=2507 || pinned.sites.size()!=146 ||
           !"b029538b90e0e23947c9224779028679f0faced032f5a469b07f69b8859dd11e"
               .equals(pinned.fingerprint))
            throw new IllegalArgumentException("R171 pin not reproduced");
        Path source=Paths.get(jarPath);
        List<byte[]> data=new ArrayList<byte[]>();
        try(ZipFile zip=new ZipFile(source.toFile())) {
            Enumeration<? extends ZipEntry> entries=zip.entries();
            while(entries.hasMoreElements()) {
                ZipEntry e=entries.nextElement();
                if(e.isDirectory() || !e.getName().endsWith(".class"))continue;
                data.add(RenderCallsiteAudit.entry(zip,e.getName()));
            }
        }
        Graph graph=inspect(data);
        if(graph.classCount()!=pinned.classesInspected ||
           graph.directCallsites()!=pinned.sites.size())
            throw new IllegalStateException("R194 and R171 site counts differ");
        return graph;
    }
    public static void main(String[] args)throws Exception {
        if(args.length!=1)throw new IllegalArgumentException(
            "Expected exact Mojang Minecraft 1.8.9 jar path");
        Graph g=inspectPinnedJar(args[0]);
        Reachability r=g.reachability();
        System.out.println("R194_EXACT_JAR_CALL_GRAPH_PASS classes="+g.classCount()+
            " methods="+g.methodCount()+" sites="+g.directCallsites()+
            " directMethods="+g.directConsumerMethods()+
            " internalEdges="+r.internalEdges+
            " externalTargets="+r.externalDeclaredTargets);
        System.out.println("R194_EXACT_DECLARED_TARGET_REACHABILITY "+
            "exposed="+r.exposedMethods()+" oneHop="+r.oneHopMethods+
            " twoPlus="+r.twoOrMoreHopMethods+" maxDepth="+r.maximumDepth);
        System.out.println("R194_DECLARED_CALL_GRAPH_SHA256 "+r.fingerprint);
        System.out.println("R194_AVE_MAIN_INPUT_DISTANCE "+
            r.distance("ave","s","()V"));
        System.out.println("R194_STATIC_ONLY_VIRTUAL_DYNAMIC_REFLECTION_UNRESOLVED");
        System.out.println("R194_NO_LIVE_INPUT_HOOK_OR_SUPPRESSION");
    }
}
