package dev.slinkywinky.attach;

import java.util.Arrays;
import java.util.concurrent.atomic.AtomicBoolean;

/** R179 Java 8 -> segregated JNI DLL -> exact C++ source slider geometry. */
public final class R179ExactSourceTargetJniSmoke {
    private static native int[] target();
    private static native int[] evidence();
    private static native boolean nextTab();

    private static void check(boolean p,String why){
        if(!p)throw new AssertionError("R179 "+why);
    }
    private static int[] expectEvidence(int drawn,int eligible,int exactHits){
        int[] e=evidence();
        check(e!=null && Arrays.equals(e,new int[]{drawn,eligible,exactHits}),
              "Source target WGL draw counts mismatch "+
              Arrays.toString(e)+" expected "+
              Arrays.toString(new int[]{drawn,eligible,exactHits}));
        return e;
    }
    public static void main(String[] args)throws Exception {
        check(args.length==1 &&
              args[0].endsWith("slinkywinky_r166_pointer_test_jni.dll"),
              "Only isolated WGL/JNI test library permitted");
        System.load(args[0]);
        check(target()==null && evidence()==null,
              "Inactive native render target leaked");
        check(R166RetainedPointerJniSmoke.begin(),
              "Native disposable retained WGL window refused");
        try {
            expectEvidence(1,0,0);
            check(target()==null,"Source slider without expansion");
            check(R167RecoveredSliderJniSmoke.navigate(2),
                  "Original Auto Block source module not focused");
            check(target()==null,
                  "Unexpanded module exposed numeric source input rectangle");
            check(R167RecoveredSliderJniSmoke.navigate(4),
                  "Original source numeric settings did not expand");

            // R180 negative: model geometry exists but no GL11 frame
            // containing the newly opened source slider has been painted.
            check(target()==null,
                  "Unpainted source settings exposed hit geometry");
            int[] ref=R167RecoveredSliderJniSmoke.sliderTarget(500);
            check(ref!=null && ref.length==2,
                  "Prepared source geometry missing in isolated model");
            check(R175RetainedHoverJniSmoke.observeHover(
                      ref[0],ref[1],true,676,630),
                  "Unpainted source must allow an empty passive sample");
            check(target()==null,
                  "Empty passive sample exposed unpainted source target");
            expectEvidence(1,0,0);
            check(R166RetainedPointerJniSmoke.redraw(),
                  "Source page first paint failed");
            int[] rect=target();
            check(rect!=null && rect.length==6 &&
                  rect[0]>=0 && rect[1]>=0 &&
                  rect[2]<=676 && rect[3]<=630 &&
                  rect[0]<rect[4] && rect[4]<rect[2] &&
                  rect[1]<rect[5] && rect[5]<rect[3],
                  "Live retained native source target rectangle malformed");
            check(ref!=null && ref.length==2 &&
                  ref[0]>=rect[0] && ref[0]<rect[2] &&
                  ref[1]>=rect[1] && ref[1]<rect[3],
                  "Legacy independent reference source hit disagrees with actual native rectangle");
            expectEvidence(2,1,0);
            final String profile=R166RetainedPointerJniSmoke.profile();
            check(profile!=null,"Native retained profile missing");

            final AtomicBoolean foreignDenied=new AtomicBoolean(false);
            Thread alien=new Thread(new Runnable() {
                @Override public void run(){
                    foreignDenied.set(target()==null && evidence()==null);
                }
            });
            alien.start();alien.join();
            check(foreignDenied.get(),
                  "Cross-thread source geometry/JNI evidence accepted");

            check(R175RetainedHoverJniSmoke.observeHover(
                      rect[4],rect[5],true,676,630),
                  "Source center did not hover first-party native numeric track");
            expectEvidence(2,1,1);
            check(R175RetainedHoverJniSmoke.observeHover(
                      rect[4],rect[5],true,676,630),
                  "Repeated source numeric hover refused");
            expectEvidence(2,1,2);
            check(R166RetainedPointerJniSmoke.redraw(),
                  "Additional real GL11 source-slider draw failed");
            expectEvidence(3,2,2);
            check(R175RetainedHoverJniSmoke.observeHover(
                      rect[4],rect[5],false,676,630),
                  "Focus loss must fail closed to no numeric hover");
            expectEvidence(3,2,2);
            check(R175RetainedHoverJniSmoke.observeHover(
                      rect[0]-2,rect[5],true,676,630),
                  "Outside source numeric rect should be harmless");
            expectEvidence(3,2,2);
            check(profile.equals(R166RetainedPointerJniSmoke.profile()),
                  "Source inspection changed original preview profile");
            check(!R175RetainedHoverJniSmoke.observeHover(
                      rect[4],rect[5],true,677,630),
                  "Wrong viewport source hover admitted");
            expectEvidence(3,2,2);

            check(nextTab(),
                  "Original source final tab transition not admitted");
            check(target()==null,
                  "Source numeric track persisted on wrong tab");
            check(R166RetainedPointerJniSmoke.redraw(),
                  "Real GL11 wrong-tab retained redraw refused");
            expectEvidence(4,2,2);
            check(profile.equals(R166RetainedPointerJniSmoke.profile()),
                  "Source tab transition wrote client gameplay settings");
            System.out.println("R179_JAVA8_JNI_EXACT_CXX_SOURCE_TARGET_GEOMETRY_PASS");
            System.out.println("R179_NATIVE_WGL_SOURCE_ELIGIBLE_AND_EXACT_HIT_COUNTERS_PASS");
            System.out.println("R179_FOREGROUND_REFUSAL_OWNER_AND_PROFILE_ISOLATION_PASS");
            System.out.println("R179_NO_ORIGINAL_GAME_CLICK_DISPATCH");
            System.out.println("R180_UNPAINTED_NAVIGATION_CANNOT_EXPOSE_HOVER_TARGET_PASS");
        } finally {
            check(R166RetainedPointerJniSmoke.end(),
                  "Disposable WGL context release failed");
        }
        check(target()==null && evidence()==null,
              "Source target remained after native WGL disposal");
    }
}
