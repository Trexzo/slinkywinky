package dev.slinkywinky.instrument;

import java.io.BufferedReader;
import java.io.InputStreamReader;

/**
 * R242 test-only external JVM with deterministic checkpoints.
 * Never imports, starts or attaches to Minecraft.
 */
public final class R242InputAttachTarget {
    private static void check(boolean condition,String why) {
        if(!condition)throw new AssertionError("R242 "+why);
    }
    private static void command(BufferedReader reader,String expected)
        throws Exception {
        String received=reader.readLine();
        check(expected.equals(received),"wrong isolated command: "+received);
    }
    public static void main(String[] args) throws Exception {
        if(args.length!=0 ||
           !"1".equals(System.getenv("SLINKYWINKY_R241_DISPOSABLE_FIXTURE")) ||
           !"1".equals(System.getenv("SLINKYWINKY_R242_DISPOSABLE_NEGATIVE")))
            throw new IllegalArgumentException("R242 disposable fixture only");
        final R240InputFixture fixture=new R240InputFixture();
        final BufferedReader reader=new BufferedReader(new InputStreamReader(System.in));
        System.out.println("R242_DISPOSABLE_INPUT_TARGET_READY");
        System.out.flush();
        long deadline=System.nanoTime()+20000000000L;
        while(!R241InputAttachAgent.isReady() && System.nanoTime()<deadline)
            Thread.sleep(20);
        check(R241InputAttachAgent.isReady(),"expected one valid Attach admission");
        for(int i=0;i<8;i++)fixture.tick();
        check(fixture.calls==8,"original input body suppressed in first 8 calls");
        final Throwable[] foreignFailure=new Throwable[1];
        Thread other=new Thread(new Runnable(){
            @Override public void run(){
                try {fixture.tick();}
                catch(Throwable error){foreignFailure[0]=error;}
            }
        },"r242-disposable-foreign-input");
        other.start();other.join(5000);
        check(!other.isAlive() && foreignFailure[0]==null && fixture.calls==9,
              "foreign callback altered original method body");
        System.out.println("R242_FOREIGN_ENTRY_OBSERVED_AND_DUPLICATE_WINDOW_READY");
        System.out.flush();
        command(reader,"RESTORE");
        boolean refused=false;
        try {R241InputAttachAgent.restoreOnOwner();}
        catch(IllegalStateException expected) {
            refused="R241 owner entry count mismatch".equals(expected.getMessage());
        }
        check(refused,"foreign-thread observation unexpectedly certified");
        System.out.println("R242_FOREIGN_THREAD_COUNT_REJECTED_AFTER_SAFE_RESTORE");
        System.out.println("R242_POST_RESTORE_REATTACH_WINDOW_READY");
        System.out.flush();
        command(reader,"FINISH");
        for(int i=0;i<8;i++)fixture.tick();
        check(fixture.calls==17,"restored input method behavior changed");
        int[] stopped=OriginalInputFrameDiagnostic.fixtureEnd();
        check(stopped[0]==8 && stopped[1]==1,
              "observer resumed after rejection and restoration");
        System.out.println("R242_ORIGINAL_INPUT_BODY_9_PLUS_8_PASS calls=17");
        System.out.println("R242_DISPOSABLE_FAILURE_CASE_CLEAN_EXIT");
        System.out.flush();
    }
    private R242InputAttachTarget(){}
}
