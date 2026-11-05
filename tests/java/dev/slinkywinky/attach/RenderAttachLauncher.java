package dev.slinkywinky.attach;

import com.sun.tools.attach.VirtualMachine;
import java.nio.file.Path;
import java.nio.file.Paths;

/** R66 isolated external Java 8 Attach caller; only explicit disposable PID. */
public final class RenderAttachLauncher {
    public static void main(String[] args) throws Exception {
        if(args.length!=2 ||
           !"1".equals(System.getenv("SLINKYWINKY_R66_DISPOSABLE_FIXTURE")))
            throw new IllegalArgumentException("Disposable target gate required");
        long pid=Long.parseLong(args[0]);
        Path jar=Paths.get(args[1]).toAbsolutePath().normalize();
        if(pid<=0 || !java.nio.file.Files.isRegularFile(jar) ||
           !"slinkywinky_r66_test_agent.jar".equals(jar.getFileName().toString()))
            throw new IllegalArgumentException("Invalid exact test agent path or PID");
        VirtualMachine vm=VirtualMachine.attach(Long.toString(pid));
        try {vm.loadAgent(jar.toString(),"R66_FIXTURE_ONLY");}
        finally {vm.detach();}
        System.out.println("R66_EXTERNAL_JAVA8_ATTACH_PASS");
    }
    private RenderAttachLauncher(){}
}
