package dev.slinkywinky.attach;

import com.sun.tools.attach.VirtualMachine;
import com.sun.tools.attach.VirtualMachineDescriptor;
import java.io.File;
import java.util.List;

/** Java 8's supported same-user Attach API; no Forge, memory patching or stealth. */
public final class AttachMain {
    private static boolean recognizedMinecraft(String name) {
        return name != null && (name.contains("net.minecraft.launchwrapper.Launch") ||
            name.contains("net.minecraft.client.main.Main") ||
            name.contains("GradleStart") || name.contains("GradleStartCommon"));
    }
    private static VirtualMachineDescriptor descriptor(String pid) {
        for (VirtualMachineDescriptor d : VirtualMachine.list()) {
            if (d.id().equals(pid)) return d;
        }
        return null;
    }
    public static void main(String[] args) {
        try {
            if (args.length == 1 && "list".equals(args[0])) {
                for (VirtualMachineDescriptor d : VirtualMachine.list()) {
                    System.out.println(d.id() + " " + d.displayName() +
                        (recognizedMinecraft(d.displayName()) ? " [minecraft-candidate]" : ""));
                }
                return;
            }
            if ((args.length != 3 && args.length != 4) || !"attach".equals(args[0]))
                throw new IllegalArgumentException("Usage: list | attach PID ABSOLUTE_DLL");
            final boolean tickProbe = args.length == 4 && "--tick-probe".equals(args[3]);
            final boolean instrumentProbe = args.length == 4 && "--instrument-probe".equals(args[3]);
            final boolean renderPreflight = args.length == 4 && "--render-preflight".equals(args[3]);
            final boolean renderCallback = args.length == 4 && "--render-callback=8".equals(args[3]);
            final String requestedOption = args.length == 4 ? args[3] : "";
            final boolean extendedHook = "--hook-probe=120".equals(requestedOption) ||
                                         "--hook-probe=600".equals(requestedOption);
            final boolean continuousHook = "--hook-probe=continuous".equals(requestedOption);
            final boolean hookProbe = "--hook-probe".equals(requestedOption) ||
                                      extendedHook || continuousHook;
            if (renderCallback && !"1".equals(System.getenv("SLINKYWINKY_RENDER_CALLBACK")))
                throw new IllegalArgumentException("R75 requires SLINKYWINKY_RENDER_CALLBACK=1");
            if (renderPreflight && !"1".equals(System.getenv("SLINKYWINKY_RENDER_PREFLIGHT")))
                throw new IllegalArgumentException("Render preflight requires SLINKYWINKY_RENDER_PREFLIGHT=1");
            if (hookProbe && !"1".equals(System.getenv("SLINKYWINKY_EXPERIMENTAL_HOOK")))
                throw new IllegalArgumentException("Unverified hook requires explicit SLINKYWINKY_EXPERIMENTAL_HOOK=1");
            if (extendedHook && !"1".equals(System.getenv("SLINKYWINKY_EXTENDED_DIAGNOSTIC")))
                throw new IllegalArgumentException("Extended read-only hook requires SLINKYWINKY_EXTENDED_DIAGNOSTIC=1");
            if (continuousHook && !"1".equals(System.getenv("SLINKYWINKY_CONTINUOUS_OBSERVATION")))
                throw new IllegalArgumentException("Continuous hook requires SLINKYWINKY_CONTINUOUS_OBSERVATION=1");
            if (args.length == 4 && !tickProbe && !instrumentProbe &&
                !renderPreflight && !renderCallback && !hookProbe) throw new IllegalArgumentException("Unknown option");
            final String pid = args[1];
            if (!pid.matches("[0-9]{1,10}"))
                throw new IllegalArgumentException("Numeric PID required");
            final File dll = new File(args[2]);
            if (!dll.isAbsolute() || !dll.isFile() || !dll.getName().equalsIgnoreCase("slinkywinky_agent.dll"))
                throw new IllegalArgumentException("Expected an absolute path to slinkywinky_agent.dll");
            VirtualMachineDescriptor target = descriptor(pid);
            if (target == null) throw new IllegalArgumentException("Target is not an attachable JVM owned by this user");
            final boolean test = "1".equals(System.getenv("SLINKYWINKY_ATTACH_TEST"));
            // Never allow the synthetic Attach-test bypass for the original-only R75 hook.
            if (renderCallback && test)
                throw new IllegalArgumentException("R75 cannot run in Attach test mode");
            if (!test && !recognizedMinecraft(target.displayName()))
                throw new IllegalArgumentException("Refusing non-Minecraft target: " + target.displayName());
            VirtualMachine vm = VirtualMachine.attach(target);
            try {
                if (instrumentProbe || hookProbe || renderPreflight || renderCallback) {
                    File instrumentationJar = new File(dll.getParentFile(), "slinkywinky_instrument.jar");
                    if (!instrumentationJar.isFile())
                        throw new IllegalArgumentException("Missing instrumentation agent JAR");
                    String options = renderCallback ? "minecraft-render-callback-r75" :
                                     (renderPreflight ? "minecraft-render-preflight" :
                                     (test ? "probe" : "minecraft"));
                    if (hookProbe) {
                        File jniLibrary = new File(dll.getParentFile(), "slinkywinky_jni.dll");
                        if (!jniLibrary.isFile())
                            throw new IllegalArgumentException("Missing standalone JNI library");
                        options = (continuousHook ? (test ? "hook-continuous" : "minecraft-continuous") :
                                               (test ? "hook-probe" : "minecraft-hook")) +
                            "|" + jniLibrary.getCanonicalPath() +
                            (extendedHook ? "|" + requestedOption.substring("--hook-probe=".length()) : "");
                    }
                    vm.loadAgent(instrumentationJar.getCanonicalPath(), options);
                } else {
                    vm.loadAgentPath(dll.getCanonicalPath(), test ? "probe" : (tickProbe ? "minecraft-ticks" : "minecraft"));
                }
            } finally {
                vm.detach();
            }
            System.out.println("ATTACH_RESULT=SUCCESS pid=" + pid + (renderCallback ? " mode=render-callback-r75" : (renderPreflight ? " mode=render-preflight" : (hookProbe ? " mode=hook-probe" : (instrumentProbe ? " mode=instrument-probe" : (test ? " mode=probe" : (tickProbe ? " mode=minecraft-ticks" : " mode=minecraft")))))));
        } catch (Exception failure) {
            System.err.println("ATTACH_RESULT=FAIL " + failure.getClass().getSimpleName() + ": " + failure.getMessage());
            System.exit(2);
        }
    }
}
