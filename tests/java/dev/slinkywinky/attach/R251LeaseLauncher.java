package dev.slinkywinky.attach;

import com.sun.tools.attach.VirtualMachine;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;

/** Explicit disposable-PID Java8 Attach caller. Never scans host JVMs. */
public final class R251LeaseLauncher {
    public static void main(String[] args) throws Exception {
        if(args.length!=6 ||
           !"1".equals(System.getenv("SLINKYWINKY_R251_DISPOSABLE_FIXTURE")))
            throw new IllegalArgumentException("R251 fixture opt-in required");
        long targetPid=Long.parseLong(args[0]);
        long claimedPid=Long.parseLong(args[2]);
        long birth=Long.parseLong(args[3]);
        long generation=Long.parseLong(args[4]);
        String token=args[5];
        Path agent=Paths.get(args[1]).toAbsolutePath().normalize();
        if(targetPid<=0 || claimedPid<=0 || birth<=0 || generation<=0 ||
           !Files.isRegularFile(agent) ||
           !"slinkywinky_r251_test_agent.jar".equals(agent.getFileName().toString()))
            throw new IllegalArgumentException("R251 exact target/agent required");
        // Only the disposable child PID provided by the orchestrator is used.
        // A forged claimed PID is an input to the TARGET's attestation gate,
        // never an alternate VM.attach() destination.
        VirtualMachine vm=VirtualMachine.attach(Long.toString(targetPid));
        try {
            vm.loadAgent(agent.toString(),
                "R251|"+claimedPid+"|"+birth+"|"+generation+"|"+token);
            System.out.println("R251_ATTACH_API_RETURNED");
        } catch(Exception rejected) {
            System.out.println("R251_ATTACH_API_REFUSED "+
                               rejected.getClass().getSimpleName());
        } finally {vm.detach();}
    }
    private R251LeaseLauncher(){}
}
