package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.OriginalInputFrameDiagnostic;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipFile;

/**
 * R240 direct exact Mojang 1.8.9 ave.s()V input method audit.
 * Reads original JAR only; does not attach to JVM or install a transformer.
 */
public final class R240OriginalInputEntryWeaveAudit {
    private static void check(boolean good,String why){
        if(!good)throw new AssertionError("R240 "+why);
    }
    private static boolean refused(byte[] data) {
        try {
            OriginalInputFrameDiagnostic.weavePinnedOriginalForOffline(data);
            return false;
        } catch(IllegalArgumentException correct) {
            return true;
        }
    }
    public static void main(String[] args) throws Exception {
        check(args.length==1 && Files.isRegularFile(Paths.get(args[0])),
              "Expected immutable original Mojang JAR path");
        RenderCallsiteAudit.main(args); // official JAR size/SHA-1 gate
        byte[] source;
        try(ZipFile jar=new ZipFile(args[0])){
            source=RenderCallsiteAudit.entry(jar,"ave.class");
        }
        final byte[] baseline=source.clone();
        R162InputOrderLedger.Report original=R162InputOrderLedger.pinned(source);
        byte[] candidate=OriginalInputFrameDiagnostic
            .weavePinnedOriginalForOffline(source);
        check(candidate.length>source.length &&
              Arrays.equals(baseline,source),"Mojang input source mutated");
        check(refused(candidate),"Repeated instrumentation was admitted");
        byte[] changed=source.clone();
        changed[changed.length-1]^=1;
        check(refused(changed) && refused(new byte[]{1,2,3}),
              "Non-pinned original input bytecode was admitted");

        final List<String> before=OriginalKeyboardLoopWeaveAudit.trace(source);
        final List<String> expected=new ArrayList<String>(before);
        int index=-1;
        for(int i=0;i<expected.size();i++){
            if(expected.get(i).startsWith("METHOD:") &&
               expected.get(i).endsWith(":s:()V")) {
                check(index==-1,"Duplicate input method trace");
                index=i;
            }
        }
        check(index>=0,"Original input method trace missing");
        final String observation=
            "M:184:dev/slinkywinky/instrument/OriginalInputFrameDiagnostic:"+
            "onInputEntry:()V:false";
        expected.add(index+1,observation);
        List<String> actual=OriginalKeyboardLoopWeaveAudit.trace(candidate);
        check(expected.equals(actual),
              "ASM modified instructions outside original method-entry call");

        final R162InputOrderLedger.Report woven=
            R162InputOrderLedger.inspect(candidate);
        check(woven.sites.size()==58 &&
              original.sites.size()==woven.sites.size() &&
              woven.methodCalls==original.methodCalls+1 &&
              woven.jumps==original.jumps &&
              woven.switches==original.switches &&
              woven.handlers==original.handlers,
              "Original input dispatch consumers or exception graph changed");
        for(int i=0;i<original.sites.size();i++){
            final R162InputOrderLedger.Site a=original.sites.get(i);
            final R162InputOrderLedger.Site b=woven.sites.get(i);
            check(b.callOrdinal==a.callOrdinal+1 &&
                  a.family.equals(b.family) &&
                  a.phase.equals(b.phase) &&
                  a.signature.equals(b.signature),
                  "Original input consumer site changed: "+i);
        }
        check(Arrays.equals(source,baseline),
              "Audit modified the immutable source copy");
        System.out.println(
            "R240_EXACT_MOJANG_189_INPUT_METHOD_ENTRY_ONE_CALL_ONLY_PASS");
        System.out.println(
            "R240_ALL_58_ORIGINAL_INPUT_CONSUMER_SITES_UNCHANGED_PASS");
        System.out.println(
            "R240_MOJANG_SOURCE_UNCHANGED_NO_LIVE_INPUT_INTERCEPTION");
    }
}
