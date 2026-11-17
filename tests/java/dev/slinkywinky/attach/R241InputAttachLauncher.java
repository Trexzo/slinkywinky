package dev.slinkywinky.attach;

import com.sun.tools.attach.VirtualMachine;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/** R241 strict test-only Attach helper: only the designated disposable PID. */
public final class R241InputAttachLauncher {
    public static void main(String[] args) throws Exception {
        if(args.length!=2 ||
           !"1".equals(System.getenv("SLINKYWINKY_R241_DISPOSABLE_FIXTURE")))
            throw new IllegalArgumentException("R241 explicit test opt-in required");
        long pid=Long.parseLong(args[0]);
        Path jar=Paths.get(args[1]).toAbsolutePath().normalize();
        if(pid<=0 || !Files.isRegularFile(jar) ||
           !"slinkywinky_r241_test_agent.jar".equals(jar.getFileName().toString()))
            throw new IllegalArgumentException("R241 invalid test PID or agent");
        VirtualMachine vm=VirtualMachine.attach(Long.toString(pid));
        try {vm.loadAgent(jar.toString(),"R241_DISPOSABLE_INPUT_ONLY");}
        finally {vm.detach();}
        System.out.println("R241_EXTERNAL_JAVA8_ATTACH_PASS");
    }
    private R241InputAttachLauncher(){}
}
