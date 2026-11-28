package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.Vanilla189OutboundEntryWeaver;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.InputStream;
import java.nio.file.Files;
import java.security.MessageDigest;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassVisitor;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

/**
 * R367: run production original outbound entry ASM weaver against the real
 * SHA-pinned Mojang ek.class entirely in memory. Never loads or writes it.
 */
public final class OriginalNetworkEntryAudit {
    private static void yes(boolean value,String what){
        if(!value)throw new IllegalStateException(what);
    }
    private static String hash(String algorithm,byte[] bytes)throws Exception{
        byte[] digest=MessageDigest.getInstance(algorithm).digest(bytes);
        StringBuilder out=new StringBuilder(digest.length*2);
        for(byte b:digest)out.append(String.format("%02x",b&0xff));
        return out.toString();
    }
    private static Map<String,List<String>> calls(byte[] body){
        Map<String,List<String>> result=new HashMap<String,List<String>>();
        new ClassReader(body).accept(new ClassVisitor(Opcodes.ASM5){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String signature,String[] exceptions){
                final String method=name+desc;
                yes(!result.containsKey(method),"Duplicate method descriptor");
                final List<String> ledger=new ArrayList<String>();
                result.put(method,ledger);
                return new MethodVisitor(Opcodes.ASM5){
                    @Override public void visitMethodInsn(int opcode,String owner,
                             String m,String desc,boolean itf){
                        ledger.add(opcode+" "+owner+"."+m+desc+" "+itf);
                    }
                };
            }
        },0);
        return result;
    }
    public static void main(String[] args)throws Exception{
        yes(args.length==1,"Exactly one original jar required");
        File original=new File(args[0]);
        yes(original.isFile()&&original.length()==8461484L,
            "Untrusted original Mojang 1.8.9 jar length");
        yes("3870888a6c3d349d3771a3e9d16c9bf5e076b908".equals(
            hash("SHA-1",Files.readAllBytes(original.toPath()))),
            "Wrong original Mojang client digest");
        byte[] source;
        try(ZipFile zip=new ZipFile(original)){
            ZipEntry entry=zip.getEntry("ek.class");
            yes(entry!=null&&entry.getSize()>0&&entry.getSize()<1000000,
                "Invalid original NetworkManager class entry");
            try(InputStream in=zip.getInputStream(entry);
                ByteArrayOutputStream out=new ByteArrayOutputStream()){
                byte[] buf=new byte[8192];int n;
                while((n=in.read(buf))!=-1){
                    yes(out.size()+n<1000000,"Oversized class entry");
                    out.write(buf,0,n);
                }
                source=out.toByteArray();
            }
        }
        yes("0b898cf41a16e432dd4e873ed3bfbaf2536c149d44269910f31dca5f242d9436"
            .equals(hash("SHA-256",source)),"Wrong original ek.class bytes");
        Map<String,List<String>> oldCalls=calls(source);
        yes(oldCalls.containsKey("a(Lff;)V"),"Original outbound method missing");

        ClassReader reader=new ClassReader(source);
        ClassWriter writer=new ClassWriter(reader,ClassWriter.COMPUTE_MAXS);
        AtomicInteger exact=new AtomicInteger(),entries=new AtomicInteger();
        reader.accept(new ClassVisitor(Opcodes.ASM5,writer){
            @Override public MethodVisitor visitMethod(int access,String name,
                    String desc,String signature,String[] exceptions){
                MethodVisitor down=super.visitMethod(access,name,desc,signature,exceptions);
                if(Vanilla189OutboundEntryWeaver.matches(name,desc,access)){
                    exact.incrementAndGet();
                    return Vanilla189OutboundEntryWeaver.wrap(down,entries);
                }
                return down;
            }
        },0);
        yes(exact.get()==1&&entries.get()==1,
            "Source entry ASM must match precisely one live-shaped method");
        Map<String,List<String>> newCalls=calls(writer.toByteArray());
        yes(newCalls.size()==oldCalls.size(),"Unexpected original JVM method count");
        String callback=Opcodes.INVOKESTATIC+
            " dev/slinkywinky/instrument/InstrumentationProbe.onOriginalOutboundPacket"+
            "(Ljava/lang/Object;Ljava/lang/Object;)V false";
        for(Map.Entry<String,List<String>> e:oldCalls.entrySet()){
            List<String> current=newCalls.get(e.getKey());
            yes(current!=null,"Missing original method after in-memory transform");
            List<String> expected=new ArrayList<String>(e.getValue());
            if("a(Lff;)V".equals(e.getKey()))expected.add(0,callback);
            yes(expected.equals(current),
                "Original method callsite order was modified: "+e.getKey());
        }
        System.out.println("R367_REAL_ORIGINAL_EK_PACKET_ASM_MEMORY_ONLY_PASS "+
            "method=a(Lff;)V callbacks=1 otherMethodsUnchanged=1 packetWrites=0");
        System.out.println("R367_NO_CLASS_EXPORT_NO_GAME_PROCESS_NO_PACKET_IO");
    }
}
