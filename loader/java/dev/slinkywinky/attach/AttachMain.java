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
            if (args.length != 3 || !"attach".equals(args[0]))
                throw new IllegalArgumentException("Usage: list | attach PID ABSOLUTE_DLL");
            final String pid = args[1];
            if (!pid.matches("[0-9]{1,10}"))
                throw new IllegalArgumentException("Numeric PID required");
            final File dll = new File(args[2]);
            if (!dll.isAbsolute() || !dll.isFile() || !dll.getName().equalsIgnoreCase("slinkywinky_agent.dll"))
                throw new IllegalArgumentException("Expected an absolute path to slinkywinky_agent.dll");
            VirtualMachineDescriptor target = descriptor(pid);
            if (target == null) throw new IllegalArgumentException("Target is not an attachable JVM owned by this user");
            final boolean test = "1".equals(System.getenv("SLINKYWINKY_ATTACH_TEST"));
            if (!test && !recognizedMinecraft(target.displayName()))
                throw new IllegalArgumentException("Refusing non-Minecraft target: " + target.displayName());
            VirtualMachine vm = VirtualMachine.attach(target);
            try {
                vm.loadAgentPath(dll.getCanonicalPath(), test ? "probe" : "minecraft");
            } finally {
                vm.detach();
            }
            System.out.println("ATTACH_RESULT=SUCCESS pid=" + pid + (test ? " mode=probe" : " mode=minecraft"));
        } catch (Exception failure) {
            System.err.println("ATTACH_RESULT=FAIL " + failure.getClass().getSimpleName() + ": " + failure.getMessage());
            System.exit(2);
        }
    }
}
