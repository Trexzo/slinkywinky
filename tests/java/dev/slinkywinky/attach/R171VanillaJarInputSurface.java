package dev.slinkywinky.attach;

import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.Enumeration;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R171 OFFLINE-ONLY scan of every original Mojang Minecraft 1.8.9 class for
 * direct LWJGL2 Mouse/Keyboard access and key-binding poll/dispatch calls.
 *
 * This is a static invocation inventory; it is NOT a graph of all transitive
 * input effects, a hook, gameplay input ownership, or a rollback receipt.
 * Never bundle into the injectable Java agent.
 */
public final class R171VanillaJarInputSurface implements Opcodes {
    private static final String MOUSE="org/lwjgl/input/Mouse";
    private static final String KEYBOARD="org/lwjgl/input/Keyboard";
    private static final String KEYBIND="avb";
    private static final String JAR_SHA1=
        "3870888a6c3d349d3771a3e9d16c9bf5e076b908";
    private static final long JAR_LENGTH=8461484L;
    private R171VanillaJarInputSurface(){}

    public static final class Site {
        public final String ownerClass,method,descriptor,targetClass,targetMethod;
        public final String targetDescriptor,family;
        public final int opcode,ordinal;
        Site(String type,String name,String desc,int ordinal,int op,
             String target,String called,String calledDesc,String family){
            ownerClass=type;method=name;descriptor=desc;this.ordinal=ordinal;
            opcode=op;targetClass=target;targetMethod=called;
            targetDescriptor=calledDesc;this.family=family;
        }
        public String normalized(){
            return ownerClass+"."+method+descriptor+"#"+ordinal+"|"+
                opcode+"|"+family+"|"+targetClass+"."+targetMethod+
                targetDescriptor;
        }
        public String caller(){return ownerClass+"."+method+descriptor;}
        public boolean insideMainInput(){
            return "ave".equals(ownerClass) && "s".equals(method) &&
                   "()V".equals(descriptor);
        }
    }
    public static final class Report {
        public final List<Site> sites;
        public final int classesInspected;
        public final String fingerprint;
        Report(List<Site> sites,int classesInspected) {
            this.sites=Collections.unmodifiableList(new ArrayList<Site>(sites));
            this.classesInspected=classesInspected;
            StringBuilder transcript=new StringBuilder();
            for(Site site:sites)transcript.append(site.normalized()).append('\n');
            this.fingerprint=sha("SHA-256",transcript.toString().getBytes(
                java.nio.charset.StandardCharsets.UTF_8));
        }
        public int count(String family) {
            int count=0;for(Site s:sites)if(family.equals(s.family))count++;
            return count;
        }
        public int outsideMainInput() {
            int count=0;for(Site s:sites)if(!s.insideMainInput())count++;
            return count;
        }
        public int directOutsideMainInput() {
            int count=0;
            for(Site s:sites)if(!s.insideMainInput() &&
                ("MOUSE".equals(s.family)||"KEYBOARD".equals(s.family)))count++;
            return count;
        }
        public boolean wholeMethodCutIsSufficient(){
            // Even zero direct references would NOT prove input isolation:
            // original GLFW/Win32 input, method indirection, separate threads
            // and loaded-bytecode restoration are not covered by this scan.
            return false;
        }
    }

    static String sha(String algo,byte[] bytes){
        try{
            byte[] digest=MessageDigest.getInstance(algo).digest(bytes);
            StringBuilder out=new StringBuilder(digest.length*2);
            for(byte b:digest){
                int n=b&255;
                if(n<16)out.append('0');
                out.append(Integer.toHexString(n));
            }
            return out.toString();
        }catch(Exception e){throw new IllegalStateException(e);}
    }

    static String kind(int opcode,String target,String name,String desc){
        if(opcode==INVOKESTATIC && MOUSE.equals(target))return "MOUSE";
        if(opcode==INVOKESTATIC && KEYBOARD.equals(target))return "KEYBOARD";
        if(KEYBIND.equals(target)){
            if(opcode==INVOKESTATIC && "a".equals(name) &&
               ("(IZ)V".equals(desc)||"(I)V".equals(desc)))
                return "KEYBIND_MUTATION";
            if(opcode==INVOKEVIRTUAL && "()Z".equals(desc) &&
               ("f".equals(name)||"d".equals(name)))
                return "KEYBIND_POLL";
        }
        return null;
    }
    /** The only relevant static instruction categories; no reflection. */
    public static List<Site> scanClass(byte[] raw) {
        if(raw==null || raw.length<32 || raw.length>1000000)
            throw new IllegalArgumentException("Absent or oversized source class");
        final ClassReader reader=new ClassReader(raw);
        final String owner=reader.getClassName();
        final List<Site> found=new ArrayList<Site>();
        reader.accept(new ClassVisitor(ASM5){
            @Override public MethodVisitor visitMethod(int flags,final String name,
                   final String desc,String signature,String[] exceptions) {
                if((flags&(ACC_ABSTRACT|ACC_NATIVE))!=0)return null;
                return new MethodVisitor(ASM5) {
                    private int ordinal;
                    @Override public void visitMethodInsn(int opcode,
                            String target,String method,String signature,
                            boolean iface){
                        ordinal++;
                        final String family=kind(opcode,target,method,signature);
                        if(family!=null && !iface)
                            found.add(new Site(owner,name,desc,ordinal,opcode,
                                               target,method,signature,family));
                    }
                };
            }
        },ClassReader.SKIP_DEBUG|ClassReader.SKIP_FRAMES);
        return found;
    }

    private static final Comparator<Site> ORDER=new Comparator<Site>() {
        @Override public int compare(Site a,Site b){
            return a.normalized().compareTo(b.normalized());
        }
    };
    public static Report inspect(List<byte[]> originals){
        if(originals==null || originals.isEmpty() || originals.size()>15000)
            throw new IllegalArgumentException("Invalid class inventory");
        List<Site> sites=new ArrayList<Site>();
        for(byte[] raw:originals)sites.addAll(scanClass(raw));
        Collections.sort(sites,ORDER);
        return new Report(sites,originals.size());
    }

    public static Report inspectPinnedJar(String originalPath)throws Exception {
        Path path=Paths.get(originalPath);
        if(!path.isAbsolute() || !Files.isRegularFile(path) ||
           Files.size(path)!=JAR_LENGTH)
            throw new IllegalArgumentException("Not original Minecraft 1.8.9 JAR");
        byte[] jar=Files.readAllBytes(path);
        if(!JAR_SHA1.equals(sha("SHA-1",jar)))
            throw new IllegalArgumentException("Original Mojang JAR SHA-1 mismatch");
        List<byte[]> all=new ArrayList<byte[]>();
        try(ZipFile zip=new ZipFile(path.toFile())) {
            Enumeration<? extends ZipEntry> e=zip.entries();
            while(e.hasMoreElements()) {
                ZipEntry z=e.nextElement();
                if(z.isDirectory() || !z.getName().endsWith(".class"))continue;
                if(z.getSize()<32 || z.getSize()>1000000 || all.size()>15000)
                    throw new IllegalArgumentException("Invalid vanilla class entry");
                all.add(RenderCallsiteAudit.entry(zip,z.getName()));
            }
        }
        Report result=inspect(all);
        if(result.classesInspected<1000 || result.sites.size()<25 ||
           result.count("MOUSE")<1 || result.count("KEYBOARD")<1 ||
           result.count("KEYBIND_POLL")<1)
            throw new IllegalArgumentException("Suspicious source inventory");
        return result;
    }
    public static void main(String[] args)throws Exception {
        if(args.length!=1)throw new IllegalArgumentException(
            "Require exact original installed Mojang 1.8.9 JAR");
        Report report=inspectPinnedJar(args[0]);
        System.out.println("R171_EXACT_VANILLA_189_JAR_GLOBAL_INPUT_SCAN_PASS");
        System.out.println("R171_INPUT_CLASSES "+report.classesInspected);
        System.out.println("R171_INPUT_SITES "+report.sites.size());
        System.out.println("R171_INPUT_SURFACE_SHA256 "+report.fingerprint);
        System.out.println("R171_INPUT_CLASSES_OUTSIDE_AVE_S "+report.outsideMainInput());
        System.out.println("R171_INPUT_DIRECT_OUTSIDE_AVE_S "+report.directOutsideMainInput());
        System.out.println("R171_INPUT_BY_FAMILY mouse="+report.count("MOUSE")+
           " keyboard="+report.count("KEYBOARD")+
           " bindingPoll="+report.count("KEYBIND_POLL")+
           " bindingMutation="+report.count("KEYBIND_MUTATION"));
        for(Site site:report.sites)if(!site.insideMainInput())
            System.out.println("R171_OUTSIDE "+site.normalized());
        System.out.println("R171_STATIC_ONLY_NO_MINECRAFT_INPUT_HOOK");
    }
}
