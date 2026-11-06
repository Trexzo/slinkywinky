package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.OriginalKeyboardBridgeWeaver;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.zip.ZipFile;

/** Test-only original Mojang 1.8.9 bytecode bridge audit. No live Attach. */
public final class OriginalKeyboardBridgeWeaveAudit {
    private static void check(boolean good,String problem) {
        if(!good)throw new AssertionError(problem);
    }
    private static boolean refused(byte[] bytes) {
        try {
            OriginalKeyboardBridgeWeaver.weavePinnedOriginal(bytes);
            return false;
        }catch(IllegalArgumentException good){return true;}
    }
    public static void main(String[] args)throws Exception {
        check(args.length==1,"Expected pinned Mojang 1.8.9 JAR");
        Path jar=Paths.get(args[0]);
        check(jar.isAbsolute() && Files.isRegularFile(jar),"Expected original JAR");
        RenderCallsiteAudit.main(args); // pinned JAR SHA1 and size
        byte[] raw;
        try(ZipFile zip=new ZipFile(jar.toFile())){
            raw=RenderCallsiteAudit.entry(zip,"ave.class");
        }
        check(OriginalKeyboardBridgeWeaver.exactOriginal(raw),"Bad original hash");
        byte[] baseline=raw.clone();
        byte[] patched=OriginalKeyboardBridgeWeaver.weavePinnedOriginal(raw);
        check(Arrays.equals(baseline,raw),"Original buffer modified");
        check(!OriginalKeyboardBridgeWeaver.exactOriginal(patched),
              "Patch still matches unmodified SHA");
        String before="M:184:org/lwjgl/input/Keyboard:next:()Z:false";
        String after="M:184:dev/slinkywinky/instrument/KeyboardQueueOwner:next:()Z:false";
        List<String> prior=OriginalKeyboardLoopWeaveAudit.trace(raw);
        check(java.util.Collections.frequency(prior,before)==1,
              "Expected unique source queue");
        List<String> expected=new ArrayList<String>(prior);
        expected.set(expected.indexOf(before),after);
        List<String> actual=OriginalKeyboardLoopWeaveAudit.trace(patched);
        check(expected.equals(actual),
              "Unexpected ASM changes outside the exact keyboard queue call");
        check(refused(patched),"Double instrumentation admitted");
        byte[] mutated=raw.clone();
        mutated[mutated.length-1]^=1;
        check(refused(mutated),"Changed game bytes admitted");
        check(refused(new byte[]{1,2,3}),"Invalid bytes admitted");
        System.out.println("R111_ORIGINAL_189_PRODUCTION_BRIDGE_ONE_CALLSITE_OFFLINE_PASS");
        System.out.println("R111_ALL_OTHER_METHOD_AND_INSTRUCTION_TRACES_UNCHANGED_PASS");
        System.out.println("R111_PREQUEUE_KEYBINDINGS_REMAIN_ACTIVE_NOT_MODAL_CERTIFICATION");
        System.out.println("R111_NO_GAME_ATTACH_OR_LIVE_INPUT_INTERCEPTOR");
    }
}
