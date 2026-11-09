package dev.slinkywinky.attach;

import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * R182 disposable Java 8 -> segregated test DLL -> real C++ retained WGL.
 * Supplies synthetic foreground flags. No physical mouse/vanilla game is used.
 */
public final class R182FocusCycleJniSmoke {
    private static native int[] targetCenter();
    private static native int[] focusEvidence(int width,int height);
    private static native int[] sourceEvidence();

    private static void check(boolean condition,String why){
        if(!condition)throw new AssertionError("R182 "+why);
    }
    private static void focus(int losses,int regains,int hits){
        final int[] actual=focusEvidence(676,630);
        check(Arrays.equals(actual,new int[]{losses,regains,hits}),
              "Wrong native focus transition counters: "+
              Arrays.toString(actual));
    }
    public static void main(String[] args)throws Exception {
        check(args.length==1 &&
              args[0].endsWith("slinkywinky_r166_pointer_test_jni.dll"),
              "Only disposable native WGL test DLL is permitted");
        System.load(args[0]);
        check(focusEvidence(676,630)==null && sourceEvidence()==null,
              "Focus evidence escaped inactive JNI/WGL owner");
        check(R166RetainedPointerJniSmoke.begin(),
              "Could not initialize source-matched native WGL owner");
        try {
            focus(0,0,0);
            check(targetCenter()==null,"Unexpanded slider target was visible");
            check(R167RecoveredSliderJniSmoke.navigate(2),
                  "Auto Block source row unavailable");
            check(R167RecoveredSliderJniSmoke.navigate(4),
                  "Auto Block source settings not expanded");
            check(targetCenter()==null,
                  "R180 should fence unpainted source geometry");
            check(R166RetainedPointerJniSmoke.redraw(),
                  "Actual source settings GL11 draw failed");
            final int[] center=targetCenter();
            check(center!=null && center.length==2,
                  "Exact painted Auto Block / Main / Range target absent");
            final int x=center[0],y=center[1];
            final String before=R166RetainedPointerJniSmoke.profile();
            check(before!=null,"Original native preview profile missing");

            final AtomicBoolean denied=new AtomicBoolean(false);
            Thread alien=new Thread(new Runnable(){
                @Override public void run(){
                    denied.set(focusEvidence(676,630)==null &&
                               sourceEvidence()==null &&
                               targetCenter()==null);
                }
            });
            alien.start();alien.join();
            check(denied.get(),"Foreign thread read focus counters");
            check(focusEvidence(677,630)==null &&
                  focusEvidence(676,631)==null,
                  "Wrong physical viewport read focus counters");

            check(R175RetainedHoverJniSmoke.observeHover(
                      x,y,true,676,630),"First foreground source hover refused");
            focus(0,0,0); // An initial foreground hit is not recovery.
            int[] source=sourceEvidence();
            check(source!=null && source.length==3 && source[2]==1,
                  "Initial source-exact hover did not count");
            check(R175RetainedHoverJniSmoke.observeHover(
                      x,y,false,676,630),"Loss of foreground not accepted");
            focus(1,0,0);
            check(sourceEvidence()[2]==1,
                  "Background source hover counted as exact hit");
            check(R175RetainedHoverJniSmoke.observeHover(
                      x,y,true,676,630),"Reacquired foreground hover refused");
            focus(1,1,1);
            check(sourceEvidence()[2]==2,
                  "Post-reacquisition source hover was not exact");

            check(R175RetainedHoverJniSmoke.observeHover(
                      x,y,false,676,630),"Second focus loss refused");
            focus(2,1,1);
            check(R175RetainedHoverJniSmoke.observeHover(
                      -25,-25,true,676,630),"Off-client foreground denied");
            focus(2,2,1); // Regain alone is NOT an exact hit.
            check(R175RetainedHoverJniSmoke.observeHover(
                      x,y,true,676,630),"Second recovery source hover refused");
            focus(2,2,2);
            check(sourceEvidence()[2]==3,
                  "Source hit counters diverged from actual native hover");
            check(before.equals(R166RetainedPointerJniSmoke.profile()),
                  "Read-only focus observations mutated engine profile");

            check(R175RetainedHoverJniSmoke.observeHover(
                      x,y,false,676,630),"Final loss of focus refused");
            focus(3,2,2);
            check(sourceEvidence()[2]==3,
                  "Final background hover forged a hit");
            System.out.println("R182_JAVA8_JNI_NATIVE_WGL_FOCUS_LOSS_REACQUISITION_PASS");
            System.out.println("R182_REGAIN_ALONE_NEVER_CERTIFIES_SOURCE_HIT_PASS");
            System.out.println("R182_WRONG_OWNER_VIEWPORT_PROFILE_DENIAL_PASS");
            System.out.println("R182_SYNTHETIC_FOREGROUND_NOT_HARDWARE_OR_GAME_INPUT");
        } finally {
            check(R166RetainedPointerJniSmoke.end(),
                  "Disposable source WGL context did not cleanly dispose");
        }
        check(focusEvidence(676,630)==null && targetCenter()==null &&
              sourceEvidence()==null,
              "Focus evidence survived WGL teardown");
    }
}
