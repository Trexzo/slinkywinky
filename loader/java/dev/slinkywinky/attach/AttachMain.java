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
            final boolean renderContext = args.length == 4 && "--render-context=8".equals(args[3]);
            final boolean renderWindowAudit = args.length == 4 && "--render-window-audit=8".equals(args[3]);
            final boolean renderOverlay = args.length == 4 && "--render-overlay=8".equals(args[3]);
            final boolean renderMultiframe = args.length == 4 && "--render-overlay=72".equals(args[3]);
            final boolean renderRetained = args.length == 4 && "--render-retained=72".equals(args[3]);
            final boolean renderOperator = args.length == 4 && "--render-menu=operator".equals(args[3]);
            final boolean renderVisibility = args.length == 4 && "--render-visibility=72".equals(args[3]);
            final boolean manualVisibility = args.length == 4 && "--render-visibility=manual".equals(args[3]);
            final String requestedOption = args.length == 4 ? args[3] : "";
            final boolean extendedHook = "--hook-probe=120".equals(requestedOption) ||
                                         "--hook-probe=600".equals(requestedOption);
            final boolean continuousHook = "--hook-probe=continuous".equals(requestedOption);
            final boolean hookProbe = "--hook-probe".equals(requestedOption) ||
                                      extendedHook || continuousHook;
            if (renderWindowAudit && (!"1".equals(System.getenv("SLINKYWINKY_RENDER_WINDOW")) ||
                                      !"1".equals(System.getenv("SLINKYWINKY_RENDER_CONTEXT")) ||
                                      !"1".equals(System.getenv("SLINKYWINKY_RENDER_CALLBACK"))))
                throw new IllegalArgumentException("R103 read-only original WGL window audit requires separate consent");
            if (renderOperator && (!"1".equals(System.getenv("SLINKYWINKY_RENDER_OPERATOR")) ||
                                    !"1".equals(System.getenv("SLINKYWINKY_RENDER_OVERLAY")) ||
                                    !"1".equals(System.getenv("SLINKYWINKY_RENDER_CONTEXT")) ||
                                    !"1".equals(System.getenv("SLINKYWINKY_RENDER_CALLBACK"))))
                throw new IllegalArgumentException("R100 requires independent original-only operator-menu consent");
            if (renderRetained && (!"1".equals(System.getenv("SLINKYWINKY_RENDER_RETAINED")) ||
                                    !"1".equals(System.getenv("SLINKYWINKY_RENDER_OVERLAY")) ||
                                    !"1".equals(System.getenv("SLINKYWINKY_RENDER_CONTEXT")) ||
                                    !"1".equals(System.getenv("SLINKYWINKY_RENDER_CALLBACK"))))
                throw new IllegalArgumentException("R99 needs separate finite retained original-menu consent");
            if (manualVisibility && (!"1".equals(System.getenv("SLINKYWINKY_RENDER_MANUAL")) ||
                                     !"1".equals(System.getenv("SLINKYWINKY_RENDER_OVERLAY")) ||
                                     !"1".equals(System.getenv("SLINKYWINKY_RENDER_CONTEXT")) ||
                                     !"1".equals(System.getenv("SLINKYWINKY_RENDER_CALLBACK"))))
                throw new IllegalArgumentException("R94 requires explicit manual visibility consent");
            if (renderVisibility && (!"1".equals(System.getenv("SLINKYWINKY_RENDER_VISIBILITY")) ||
                                     !"1".equals(System.getenv("SLINKYWINKY_RENDER_OVERLAY")) ||
                                     !"1".equals(System.getenv("SLINKYWINKY_RENDER_CONTEXT")) ||
                                     !"1".equals(System.getenv("SLINKYWINKY_RENDER_CALLBACK"))))
                throw new IllegalArgumentException("R93 needs explicit scripted visibility consent");
            if (renderMultiframe && (!"1".equals(System.getenv("SLINKYWINKY_RENDER_MULTIFRAME")) ||
                                     !"1".equals(System.getenv("SLINKYWINKY_RENDER_OVERLAY")) ||
                                     !"1".equals(System.getenv("SLINKYWINKY_RENDER_CONTEXT")) ||
                                     !"1".equals(System.getenv("SLINKYWINKY_RENDER_CALLBACK"))))
                throw new IllegalArgumentException("R92 requires explicit finite multi-frame overlay consent");
            if (renderOverlay && (!"1".equals(System.getenv("SLINKYWINKY_RENDER_OVERLAY")) ||
                                  !"1".equals(System.getenv("SLINKYWINKY_RENDER_CONTEXT")) ||
                                  !"1".equals(System.getenv("SLINKYWINKY_RENDER_CALLBACK"))))
                throw new IllegalArgumentException("R91 requires explicitly enabled overlay, context and callback");
            if (renderContext && (!"1".equals(System.getenv("SLINKYWINKY_RENDER_CONTEXT")) ||
                                  !"1".equals(System.getenv("SLINKYWINKY_RENDER_CALLBACK"))))
                throw new IllegalArgumentException("R78 requires SLINKYWINKY_RENDER_CONTEXT=1 and SLINKYWINKY_RENDER_CALLBACK=1");
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
                !renderPreflight && !renderCallback && !renderContext && !renderWindowAudit && !renderOverlay && !renderMultiframe && !renderRetained && !renderOperator && !renderVisibility && !manualVisibility && !hookProbe) throw new IllegalArgumentException("Unknown option");
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
            if ((renderCallback || renderContext || renderWindowAudit || renderOverlay || renderMultiframe || renderRetained || renderOperator) && test)
                throw new IllegalArgumentException("R75 cannot run in Attach test mode");
            if (!test && !recognizedMinecraft(target.displayName()))
                throw new IllegalArgumentException("Refusing non-Minecraft target: " + target.displayName());
            VirtualMachine vm = VirtualMachine.attach(target);
            try {
                if (instrumentProbe || hookProbe || renderPreflight || renderCallback || renderContext || renderWindowAudit || renderOverlay || renderMultiframe || renderRetained || renderOperator || renderVisibility || manualVisibility) {
                    File instrumentationJar = new File(dll.getParentFile(), "slinkywinky_instrument.jar");
                    if (!instrumentationJar.isFile())
                        throw new IllegalArgumentException("Missing instrumentation agent JAR");
                    String options = renderMultiframe ? "minecraft-render-overlay-r92|" +
                        new File(dll.getParentFile(),"slinkywinky_jni.dll").getCanonicalPath() :
                                     (renderOverlay ? "minecraft-render-overlay-r91|" +
                        new File(dll.getParentFile(),"slinkywinky_jni.dll").getCanonicalPath() :
                                     (renderContext ? "minecraft-render-context-r78|" +
                        new File(dll.getParentFile(),"slinkywinky_jni.dll").getCanonicalPath() :
                                     (renderCallback ? "minecraft-render-callback-r75" :
                                     (renderPreflight ? "minecraft-render-preflight" :
                                     (test ? "probe" : "minecraft")))));
                    if (renderWindowAudit) options = "minecraft-render-window-audit-r101|" +
                        new File(dll.getParentFile(),"slinkywinky_jni.dll").getCanonicalPath();
                    if (renderOperator) options = "minecraft-render-menu-r100|" +
                        new File(dll.getParentFile(),"slinkywinky_jni.dll").getCanonicalPath();
                    if (renderRetained) options = "minecraft-render-retained-r99|" +
                        new File(dll.getParentFile(),"slinkywinky_jni.dll").getCanonicalPath();
                    if (manualVisibility) options = "minecraft-render-manual-r94|" +
                        new File(dll.getParentFile(),"slinkywinky_jni.dll").getCanonicalPath();
                    if (renderVisibility) options = "minecraft-render-visibility-r93|" +
                        new File(dll.getParentFile(),"slinkywinky_jni.dll").getCanonicalPath();
                    if ((renderContext || renderWindowAudit || renderOverlay || renderMultiframe || renderRetained || renderOperator || renderVisibility || manualVisibility) && !new File(dll.getParentFile(),"slinkywinky_jni.dll").isFile())
                        throw new IllegalArgumentException("R78 requires standalone JNI library");
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
            String receipt = renderWindowAudit ? " mode=render-window-audit-r103" :
                (renderOperator ? " mode=render-menu-r100" :
                (renderRetained ? " mode=render-retained-r99" :
                (manualVisibility ? " mode=render-manual-r94" :
                (renderVisibility ? " mode=render-visibility-r93" :
                (renderMultiframe ? " mode=render-overlay-r92" : (renderOverlay ? " mode=render-overlay-r91" : (renderContext ? " mode=render-context-r78" : (renderCallback ? " mode=render-callback-r75" : (renderPreflight ? " mode=render-preflight" : (hookProbe ? " mode=hook-probe" : (instrumentProbe ? " mode=instrument-probe" : (test ? " mode=probe" : (tickProbe ? " mode=minecraft-ticks" : " mode=minecraft")))))))))))));
            System.out.println("ATTACH_RESULT=SUCCESS pid=" + pid + receipt);
        } catch (Exception failure) {
            System.err.println("ATTACH_RESULT=FAIL " + failure.getClass().getSimpleName() + ": " + failure.getMessage());
            System.exit(2);
        }
    }
}
