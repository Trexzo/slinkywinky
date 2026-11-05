package dev.slinkywinky.attach;

import com.sun.tools.attach.VirtualMachine;
import com.sun.tools.attach.VirtualMachineDescriptor;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/** R77 test-only Java8 Attach launcher: exact disposable target and test JAR. */
public final class R77WglAttachLauncher {
    public static void main(String[] args) throws Exception {
        if(args.length!=2 ||
           !"1".equals(System.getenv("SLINKYWINKY_R74_DISPOSABLE_FIXTURE")) ||
           !"1".equals(System.getenv("SLINKYWINKY_R76_WGL_FIXTURE")) ||
           !"1".equals(System.getenv("SLINKYWINKY_R77_DISPOSABLE_FIXTURE")))
            throw new IllegalArgumentException("R77 exact disposable gates required");
        long pid=Long.parseLong(args[0]);
        Path jar=Paths.get(args[1]).toAbsolutePath().normalize();
        if(pid<=0 || !Files.isRegularFile(jar) ||
           !"slinkywinky_r74_test_agent.jar".equals(jar.getFileName().toString()))
            throw new IllegalArgumentException("Unexpected test agent or PID");
        VirtualMachineDescriptor chosen=null;
        for(VirtualMachineDescriptor item:VirtualMachine.list()) {
            if(item.id().equals(Long.toString(pid))) {
                chosen=item;
                break;
            }
        }
        if(chosen==null ||
           !"dev.slinkywinky.attach.R77WglRenderTarget".equals(chosen.displayName()))
            throw new IllegalArgumentException("Refusing non-R77 disposable target");
        VirtualMachine vm=VirtualMachine.attach(chosen);
        try {
            vm.loadAgent(jar.toString(),"R77_DISPOSABLE_WGL_ONLY");
        } finally {
            vm.detach();
        }
        System.out.println("R77_EXTERNAL_JAVA8_DISPOSABLE_ATTACH_PASS");
    }
    private R77WglAttachLauncher(){}
}
