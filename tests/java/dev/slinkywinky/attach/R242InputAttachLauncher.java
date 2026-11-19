package dev.slinkywinky.attach;

import com.sun.tools.attach.VirtualMachine;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/**
 * R242 exact disposable PID negative-test launcher.
 * Bad options never authorize an alternate target or commercial JVM.
 */
public final class R242InputAttachLauncher {
    public static void main(String[] args) throws Exception {
        if(args.length!=3 ||
           !"1".equals(System.getenv("SLINKYWINKY_R241_DISPOSABLE_FIXTURE")) ||
           !"1".equals(System.getenv("SLINKYWINKY_R242_DISPOSABLE_NEGATIVE")))
            throw new IllegalArgumentException("R242 fixture opt-in required");
        long pid=Long.parseLong(args[0]);
        Path jar=Paths.get(args[1]).toAbsolutePath().normalize();
        if(pid<=0 || !Files.isRegularFile(jar) ||
           !"slinkywinky_r241_test_agent.jar".equals(jar.getFileName().toString()))
            throw new IllegalArgumentException("R242 invalid fixture PID or agent");
        String option;
        if("bad-options".equals(args[2]))option="R242_BAD_OPTION";
        else if("valid".equals(args[2]))option="R241_DISPOSABLE_INPUT_ONLY";
        else throw new IllegalArgumentException("R242 unsupported option");
        VirtualMachine vm=VirtualMachine.attach(Long.toString(pid));
        try {vm.loadAgent(jar.toString(),option);}
        finally {vm.detach();}
        System.out.println("R242_EXTERNAL_JAVA8_ATTACH_ACCEPTED");
    }
    private R242InputAttachLauncher(){}
}
