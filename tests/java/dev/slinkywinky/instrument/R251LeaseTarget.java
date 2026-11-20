package dev.slinkywinky.instrument;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.lang.management.ManagementFactory;
import java.security.SecureRandom;

/** R251 exclusively disposable, non-Minecraft Java 8 Attach target. */
public final class R251LeaseTarget {
    private static final long PROCESS_START =
        ManagementFactory.getRuntimeMXBean().getStartTime();
    private static final long PID = resolvePid();
    private static final Thread OWNER = Thread.currentThread();
    // R251.1: atomically published immutable proof avoids monitor inversion:
    // synchronized agentmain -> target.matches used to acquire target class
    // while target.rotate -> synchronized Agent.release acquires agent class.
    // A lock-free proof read prevents this two-monitor deadlock scenario.
    private static final class Epoch {
        final long generation;
        final String challenge;
        Epoch(long gen,String nonce){generation=gen;challenge=nonce;}
    }
    private static volatile Epoch current = new Epoch(1L,newChallenge());
    private static volatile boolean terminal = false;

    private static long resolvePid() {
        String name=ManagementFactory.getRuntimeMXBean().getName();
        int at=name.indexOf('@');
        if(at<=0)throw new IllegalStateException("R251 PID unavailable");
        return Long.parseLong(name.substring(0,at));
    }
    private static String newChallenge() {
        byte[] data=new byte[16];
        new SecureRandom().nextBytes(data);
        StringBuilder out=new StringBuilder(32);
        for(byte value:data){
            int b=value&255;
            out.append(Character.forDigit((b>>>4)&15,16));
            out.append(Character.forDigit(b&15,16));
        }
        return out.toString();
    }
    static boolean matches(long pid,long birth,long epoch,String token){
        final Epoch observed=current; // coherent version and challenge
        return !terminal && pid==PID && birth==PROCESS_START &&
            epoch==observed.generation && observed.challenge.equals(token);
    }
    static long generation(){return current.generation;}
    static boolean isOwnerThread(){return Thread.currentThread()==OWNER;}
    private static synchronized void rotate() {
        if(Thread.currentThread()!=OWNER || terminal ||
           !R251LeaseAgent.retireOnTargetOwner())
            throw new IllegalStateException("R251 unsafe rotation refused");
        current=new Epoch(current.generation+1L,newChallenge());
    }
    private static synchronized String receipt(){
        final Epoch observed=current;
        return PID+" "+PROCESS_START+" "+observed.generation+
            " "+observed.challenge;
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=0 ||
           !"1".equals(System.getenv("SLINKYWINKY_R251_DISPOSABLE_FIXTURE")))
            throw new IllegalArgumentException("R251 disposable only");
        BufferedReader stdin=new BufferedReader(new InputStreamReader(System.in));
        System.out.println("R251_TARGET_READY "+receipt());
        System.out.flush();
        String command;
        while((command=stdin.readLine())!=null){
            if("STATUS".equals(command)){
                System.out.println("R251_STATUS "+R251LeaseAgent.status());
            } else if("ROTATE".equals(command)){
                rotate();
                System.out.println("R251_TARGET_NEW_EPOCH "+receipt());
            } else if("FOREIGN_RELEASE".equals(command)){
                final boolean[] denied={false};
                Thread intruder=new Thread(new Runnable(){
                    @Override public void run(){
                        denied[0]=!R251LeaseAgent.retireOnTargetOwner();
                    }
                },"r251-disposable-foreign-owner");
                intruder.start();intruder.join(5000L);
                if(intruder.isAlive() || !denied[0])
                    throw new IllegalStateException("R251 foreign release admitted");
                System.out.println("R251_FOREIGN_RELEASE_DENIED");
            } else if("POISON".equals(command)){
                if(!R251LeaseAgent.poisonOnTargetOwner())
                    throw new IllegalStateException("R251 poison refused");
                System.out.println("R251_UNCERTAIN_OWNER_LOCKED");
            } else if("QUIT".equals(command)){
                terminal=true;
                System.out.println("R251_DISPOSABLE_TARGET_CLEAN_EXIT");
                System.out.flush();
                return;
            } else throw new IllegalArgumentException("R251 unexpected command");
            System.out.flush();
        }
        throw new IllegalStateException("R251 control channel closed unexpectedly");
    }
    private R251LeaseTarget(){}
}
