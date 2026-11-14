package dev.slinkywinky.forge;

import java.io.File;
import java.lang.instrument.Instrumentation;

/**
 * R214 isolated test agent: no Attach API, no remote process injection.
 * Fails closed unless exact java property, test TEMP working directory
 * and explicit -javaagent in a dummy local Forge launch.
 */
public final class R214NoSlowAgent {
    private R214NoSlowAgent() {}

    public static void premain(String arguments, Instrumentation instrument) {
        if (!Boolean.getBoolean("slinkywinky.r214NoSlowAgent"))
            return;
        final String temp = new File(System.getProperty("java.io.tmpdir"))
            .getAbsolutePath().replace('\\','/');
        final String cwd = new File(System.getProperty("user.dir"))
            .getAbsolutePath().replace('\\','/');
        if (!cwd.startsWith(temp) ||
            !cwd.matches(".*[/]slinkywinky-r21[456]-independent-forge-[^/]+"))
            throw new IllegalStateException("R214 agent refuses non-TEMP Minecraft launch");
        if (instrument == null)
            throw new IllegalStateException("R214 Java instrumentation missing");
        instrument.addTransformer(new R214SlowdownTransformer(), false);
        System.out.println("[SlinkyWinkyR214] R214_LOCAL_AGENT_ARMED");
    }
}
