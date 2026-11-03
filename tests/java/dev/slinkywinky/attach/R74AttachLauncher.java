package dev.slinkywinky.attach;

import com.sun.tools.attach.VirtualMachine;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/** R74 test-only external Java 8 Attach launcher for disposable JVM PIDs. */
public final class R74AttachLauncher {
    public static void main(String[] args) throws Exception {
        if(args.length!=2 ||
           !"1".equals(System.getenv("SLINKYWINKY_R74_DISPOSABLE_FIXTURE")))
            throw new IllegalArgumentException("R74 fixture gate absent");
        long pid=Long.parseLong(args[0]);
        Path jar=Paths.get(args[1]).toAbsolutePath().normalize();
        if(pid<=0 || !Files.isRegularFile(jar) ||
           !"slinkywinky_r74_test_agent.jar".equals(jar.getFileName().toString()))
            throw new IllegalArgumentException("Unexpected fixture agent or PID");
        VirtualMachine vm=VirtualMachine.attach(Long.toString(pid));
        try {vm.loadAgent(jar.toString(),"R74_DISPOSABLE_OBFUSCATED_ONLY");}
        finally {vm.detach();}
        System.out.println("R74_EXTERNAL_JAVA8_ATTACH_PASS");
    }
    private R74AttachLauncher(){}
}
