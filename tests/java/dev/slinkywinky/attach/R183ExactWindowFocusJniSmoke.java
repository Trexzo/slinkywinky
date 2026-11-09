package dev.slinkywinky.attach;

import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

/** R183 disposable Java8 JNI/real WGL model, synthetic OS focus statuses. */
public final class R183ExactWindowFocusJniSmoke {
    private static native int[] target();
    private static native boolean sample(int state,boolean admitted,
                                         int x,int y,int width,int height);
    private static native int[] evidence(int width,int height);
    private static void check(boolean pass,String why) {
        if(!pass)throw new AssertionError("R183 "+why);
    }
    private static void expect(int... counts) {
        final int[] actual=evidence(676,630);
        check(Arrays.equals(actual,counts),
              "Wrong exact-window focus counters: "+Arrays.toString(actual));
    }
    private static void tick(int focus,boolean admitted,int x,int y) {
        check(sample(focus,admitted,x,y,676,630),
              "Native WGL focus observation refused");
    }
    public static void main(String[] args)throws Exception {
        check(args.length==1 &&
              args[0].endsWith("slinkywinky_r166_pointer_test_jni.dll"),
              "Only disposable JNI/WGL test library permitted");
        System.load(args[0]);
        check(evidence(676,630)==null && target()==null,
              "Unarmed original focus evidence escaped owner");
        check(R166RetainedPointerJniSmoke.begin(),
              "Disposable WGL owner creation failed");
        try {
            expect(0,0,0,0,0,0);
            check(R167RecoveredSliderJniSmoke.navigate(2),
                  "Source Auto Block row was not focused");
            check(R167RecoveredSliderJniSmoke.navigate(4),
                  "Source settings did not expand");
            check(target()==null,"Unpainted slider leaked target");
            check(R166RetainedPointerJniSmoke.redraw(),
                  "Actual first-party GL11 source slider paint failed");
            final int[] point=target();
            check(point!=null && point.length==2,
                  "Source-identical slider hit missing");
            final int x=point[0],y=point[1];
            final String profile=R166RetainedPointerJniSmoke.profile();

            check(evidence(677,630)==null && evidence(676,631)==null,
                  "Wrong WGL viewport got focus chronology");
            final AtomicBoolean foreignDenied=new AtomicBoolean(false);
            Thread alien=new Thread(new Runnable() {
                @Override public void run() {
                    foreignDenied.set(evidence(676,630)==null && target()==null &&
                        !sample(1,true,x,y,676,630));
                }
            });
            alien.start();alien.join();
            check(foreignDenied.get(),"Foreign thread accessed WGL focus owner");
            check(!sample(2,true,x,y,676,630) &&
                  !sample(-2,false,x,y,676,630),
                  "Unknown OS focus status was accepted");
            check(!sample(1,true,x,y,677,630),
                  "Wrong dimensions mutated focus chronology");
            expect(0,0,0,0,0,0);

            tick(1,true,x,y);
            expect(1,0,0,0,0,0); // Initial exact hover is not recovery.
            tick(-1,false,x,y);
            expect(1,0,1,0,0,0); // Refused window is NOT background.
            tick(1,true,x,y);
            expect(2,0,1,0,0,0); // Cannot bridge refused observation.
            tick(0,false,x,y);
            expect(2,1,1,1,0,0); // Genuine background transition.
            tick(1,true,-25,-25);
            expect(3,1,1,1,1,0); // Regain alone does not imply exact hit.
            tick(1,true,x,y);
            expect(4,1,1,1,1,1); // Source hit after actual ordered regain.
            tick(1,false,x,y);
            expect(4,1,2,1,1,1); // Pointer denial, no focus loss.
            tick(0,false,x,y);
            expect(4,2,2,1,1,1); // Refusal breaks prior epoch.
            tick(1,true,x,y);
            expect(5,2,2,1,1,1); // Cannot bridge to a new regain.
            tick(0,false,x,y);
            expect(5,3,2,2,1,1);
            tick(1,true,x,y);
            expect(6,3,2,2,2,2);

            check(profile!=null &&
                  profile.equals(R166RetainedPointerJniSmoke.profile()),
                  "Read-only focus evidence mutated native engine profile");
            System.out.println("R183_JAVA8_JNI_REAL_WGL_TRI_STATE_FOCUS_PASS");
            System.out.println("R183_JNI_REFUSED_POINTER_NEVER_BACKGROUND_PASS");
            System.out.println("R183_JNI_EXACT_SOURCE_HITS_ONLY_AFTER_REGAIN_PASS");
            System.out.println("R183_SYNTHETIC_OS_STATUS_NOT_ORIGINAL_GAME_PROOF");
        } finally {
            check(R166RetainedPointerJniSmoke.end(),
                  "Disposable GL11 test owner cleanup failed");
        }
        check(evidence(676,630)==null && target()==null,
              "Exact OS focus counters leaked past native WGL teardown");
    }
}
