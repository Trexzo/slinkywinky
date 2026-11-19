package dev.slinkywinky.instrument;

import java.io.BufferedReader;
import java.io.InputStreamReader;

/** R243 disposable JVM for testing actual Java 8 Attach failure boundaries. */
public final class R243InputAttachTarget {
    private static void check(boolean pass,String why) {
        if(!pass)throw new AssertionError("R243 "+why);
    }
    private static void waitReady() throws Exception {
        long deadline=System.nanoTime()+20000000000L;
        while(!R241InputAttachAgent.isReady() && System.nanoTime()<deadline)
            Thread.sleep(20);
        check(R241InputAttachAgent.isReady(),"agent did not admit restore scenario");
    }
    public static void main(String[] args) throws Exception {
        final String mode=System.getenv("SLINKYWINKY_R243_FAULT_MODE");
        if(args.length!=0 ||
           !"1".equals(System.getenv("SLINKYWINKY_R241_DISPOSABLE_FIXTURE")) ||
           !"1".equals(System.getenv("SLINKYWINKY_R242_DISPOSABLE_NEGATIVE")) ||
           !"1".equals(System.getenv("SLINKYWINKY_R243_DISPOSABLE_NEGATIVE")) ||
           !("install".equals(mode)||"remove".equals(mode)||
             "restore".equals(mode)))
            throw new IllegalArgumentException("R243 disposable fault mode only");
        final R240InputFixture fixture=new R240InputFixture();
        final BufferedReader input=new BufferedReader(
            new InputStreamReader(System.in));
        System.out.println("R243_DISPOSABLE_TARGET_READY mode="+mode);
        System.out.flush();
        final String command=input.readLine();
        check("START".equals(command),"expected exact START command");
        if("restore".equals(mode)) {
            waitReady();
            for(int i=0;i<8;i++)fixture.tick();
            check(fixture.calls==8,"observer affected original method body");
            System.out.println("R243_RESTORE_NEGATIVE_READY");
            System.out.flush();
            check("RESTORE".equals(input.readLine()),"restore command missing");
            boolean rejected=false;
            try {R241InputAttachAgent.restoreOnOwner();}
            catch(IllegalStateException expected){
                rejected="R243 recovery after restore fault".equals(
                    expected.getMessage());
            }
            check(rejected,"injected restore fault was silently accepted");
            for(int i=0;i<8;i++)fixture.tick();
            check(fixture.calls==16,"restored body failed after fault");
            final int[] count=OriginalInputFrameDiagnostic.fixtureEnd();
            check(count[0]==8 && count[1]==0,
                  "observer resumed after restore failure");
            System.out.println("R243_RESTORE_RECOVERY_BODY_8_PLUS_8_PASS");
        } else {
            check(!R241InputAttachAgent.isReady(),"failed attach became ready");
            for(int i=0;i<8;i++)fixture.tick();
            check(fixture.calls==8,"failure altered input body");
            final int[] count=OriginalInputFrameDiagnostic.fixtureEnd();
            check(count[0]==0 && count[1]==0,
                  "an aborted install activated a callback");
            System.out.println("R243_ABORTED_INSTALL_BODY_UNCHANGED_PASS mode="+mode);
        }
        System.out.println("R243_POST_FAULT_REATTACH_WINDOW");
        System.out.flush();
        check("FINISH".equals(input.readLine()),"finish command missing");
        System.out.println("R243_DISPOSABLE_"+mode.toUpperCase()+"_CLEAN_EXIT");
        System.out.flush();
    }
    private R243InputAttachTarget(){}
}
