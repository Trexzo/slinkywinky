package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.RendererLivePreflight;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.zip.ZipFile;

/** R73 static compatibility proof using the R71 SHA1/size verified Mojang JAR. */
public final class RendererLivePreflightExactAudit {
    public static void main(String[] args) throws Exception {
        if(args.length!=1) throw new IllegalArgumentException(
            "Usage: RendererLivePreflightExactAudit ORIGINAL_1_8_9_JAR");
        Path jar=Paths.get(args[0]);
        if(!jar.isAbsolute() || !Files.isRegularFile(jar))
            throw new IllegalArgumentException("Expected absolute installed original JAR");
        // This repeats official JAR size/SHA1 and unique caller checks before
        // running the same SHA-pinned structural admission used for a live class.
        RenderCallsiteAudit.main(args);
        try(ZipFile original=new ZipFile(jar.toFile())) {
            RendererLivePreflight.checkPinnedOriginal(
                RenderCallsiteAudit.entry(original,"ave.class"),
                RenderCallsiteAudit.entry(original,"bfk.class"));
        }
        System.out.println("R73_STATIC_ORIGINAL_RENDERER_ADMISSION_PASS");
        System.out.println("R73_STATIC_AUDIT_IS_NOT_LIVE_GAME_EVIDENCE");
    }
    private RendererLivePreflightExactAudit() {}
}
