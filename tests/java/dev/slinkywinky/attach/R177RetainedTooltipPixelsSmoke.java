package dev.slinkywinky.attach;

import java.util.concurrent.atomic.AtomicLong;

/**
 * R177 disposable JNI/WGL test. Verifies actual first-party GL11 tooltip
 * pixels over the exact recovered source-backed Auto Block Range control.
 * Readback is restricted to the native tooltip rectangle, not a screenshot
 * of the desktop. Does not attach to or control Minecraft.
 */
public final class R177RetainedTooltipPixelsSmoke {
    private static native long pixelDigest();
    private static void require(boolean yes,String why) {
        if(!yes)throw new AssertionError("R177 "+why);
    }
    public static void main(String[] args)throws Exception {
        require(args.length==1 &&
                args[0].endsWith("slinkywinky_r166_pointer_test_jni.dll"),
                "Only disposable native test library is admitted");
        System.load(args[0]);
        require(pixelDigest()==-1L,"Unarmed native GL11 frame readback");
        require(R166RetainedPointerJniSmoke.begin(),
                "Source-retained WGL test owner could not start");
        try {
            require(pixelDigest()==-1L,
                    "Tooltip region admitted without source row");
            require(R167RecoveredSliderJniSmoke.navigate(2),
                    "Native Auto Block source row was not selected");
            require(R167RecoveredSliderJniSmoke.navigate(4),
                    "Source-backed slider menu did not expand");
            int[] track=R167RecoveredSliderJniSmoke.sliderTarget(500);
            require(track!=null && track.length==2,
                    "Recovered numeric track source point absent");
            int x=track[0],y=track[1];
            String baselineProfile=R166RetainedPointerJniSmoke.profile();
            require(baselineProfile!=null,
                    "Exact native baseline profile unavailable");
            long noHover=pixelDigest();
            require(noHover!=-1L,"Could not read original native tooltip area");
            require(R175RetainedHoverJniSmoke.observeHover(
                        x,y,true,676,630),
                    "Real native numeric hover refused");
            long hovered=pixelDigest();
            require(hovered!=-1L && hovered!=noHover,
                    "GL11 source tooltip pixels did not visibly change");
            require(R175RetainedHoverJniSmoke.observeHover(
                        x,y,true,676,630),
                    "Idempotent hover observation refused");
            require(pixelDigest()==hovered,
                    "Repeated native tooltip GL11 pixels are not stable");

            require(R175RetainedHoverJniSmoke.observeHover(
                        x,y,false,676,630),
                    "Foreground loss did not clear source hover");
            require(pixelDigest()==noHover,
                    "Source tooltip pixels persisted after focus loss");
            require(R175RetainedHoverJniSmoke.observeHover(
                        x,y,true,676,630),
                    "Source tooltip did not reactivate after foreground");
            require(pixelDigest()==hovered,
                    "Reopened native tooltip pixels differ from original");
            require(R175RetainedHoverJniSmoke.observeHover(
                        -1,-1,true,676,630),
                    "Leaving exact source viewport did not clear hover");
            require(pixelDigest()==noHover,
                    "Source tooltip pixels persisted outside the viewport");

            AtomicLong foreign=new AtomicLong(0);
            Thread other=new Thread(new Runnable(){
                @Override public void run(){
                    foreign.set(pixelDigest());
                }
            });
            other.start();other.join();
            require(foreign.get()==-1L,
                    "Foreign Java thread was allowed to read source WGL pixels");
            require(!R175RetainedHoverJniSmoke.observeHover(
                        x,y,true,677,630),
                    "Wrong-viewport native tooltip observation admitted");
            require(pixelDigest()==noHover,
                    "Denied viewport altered original GL11 pixels");
            require(baselineProfile.equals(
                    R166RetainedPointerJniSmoke.profile()),
                    "Passive tooltip pixel test modified client settings");
            System.out.println("R177_JAVA8_JNI_WGL_ACTUAL_SOURCE_TOOLTIP_PIXEL_DELTA_PASS");
            System.out.println("R177_TOOLTIP_FOCUS_RECOVERY_PIXEL_RESTORATION_PASS");
            System.out.println("R177_TOOLTIP_NO_CLICKS_NO_PROFILE_MUTATION_PASS");
            System.out.println("R177_TEST_ONLY_WGL_NOT_ORIGINAL_MINECRAFT_FOREGROUND");
        } finally {
            require(R166RetainedPointerJniSmoke.end(),
                    "Disposable WGL test renderer failed to close");
        }
        require(pixelDigest()==-1L,
                "Original disposable native render session not cleaned up");
    }
}
