package dev.slinkywinky.instrument;

import java.lang.instrument.Instrumentation;

/** R251 disposable Java8 Attach identity and exclusive-lease fixture ONLY.
 * Not part of the production agent, JNI or original game integration.
 */
public final class R251LeaseAgent {
    private static boolean active;
    private static boolean poisoned;
    private static long lastClaimedGeneration;

    private static void refuse(String why){
        System.out.println("R251_ATTACH_REFUSED "+why);
        System.out.flush();
        throw new IllegalStateException("R251 "+why);
    }
    public static synchronized void agentmain(String options,Instrumentation inst) {
        if(!"1".equals(System.getenv("SLINKYWINKY_R251_DISPOSABLE_FIXTURE")))
            refuse("NO_DISPOSABLE_OPT_IN");
        if(inst==null || options==null)refuse("NO_ATTESTATION");
        String[] tokens=options.split("\\|",-1);
        if(tokens.length!=5 || !"R251".equals(tokens[0]))
            refuse("WRONG_PROTOCOL");
        final long pid,birth,generation;
        try {
            pid=Long.parseLong(tokens[1]);
            birth=Long.parseLong(tokens[2]);
            generation=Long.parseLong(tokens[3]);
        } catch(NumberFormatException error){
            refuse("MALFORMED_IDENTITY");return;
        }
        if(pid<=0 || birth<=0 || generation<=0 ||
           tokens[4].length()!=32 || !tokens[4].matches("[a-f0-9]{32}"))
            refuse("MALFORMED_PROOF");
        Class<?> seen=null;
        for(Class<?> candidate:inst.getAllLoadedClasses()) {
            if(candidate.getName().equals(R251LeaseTarget.class.getName())) {
                if(seen!=null)refuse("DUPLICATE_CLASS");
                seen=candidate;
            }
        }
        if(seen!=R251LeaseTarget.class ||
           seen.getClassLoader()!=ClassLoader.getSystemClassLoader())
            refuse("WRONG_CLASSLOADER");
        if(!R251LeaseTarget.matches(pid,birth,generation,tokens[4]))
            refuse("PID_BIRTH_GENERATION_OR_CHALLENGE_MISMATCH");
        if(poisoned)refuse("UNCERTAIN_OWNER_PERMANENTLY_LOCKED");
        if(active)refuse("COMPETING_OWNER_ACTIVE");
        if(generation<=lastClaimedGeneration)
            refuse("STALE_OR_REPLAYED_GENERATION");
        active=true;
        lastClaimedGeneration=generation;
        // This thread belongs to Attach, NOT the target's JVM/game thread.
        // Never authorize owner cleanup on the Attach worker thread.
        System.out.println("R251_ATTACH_LEASE_ADMITTED generation="+generation);
        System.out.flush();
    }
    static synchronized boolean retireOnTargetOwner(){
        if(!R251LeaseTarget.isOwnerThread() ||
           !active || poisoned)return false;
        active=false;
        return true;
    }
    static synchronized boolean poisonOnTargetOwner(){
        if(!R251LeaseTarget.isOwnerThread() || !active)
            return false;
        poisoned=true;
        return true;
    }
    static synchronized String status(){
        return "active="+active+" poisoned="+poisoned+
            " lastGeneration="+lastClaimedGeneration;
    }
    private R251LeaseAgent(){}
}
