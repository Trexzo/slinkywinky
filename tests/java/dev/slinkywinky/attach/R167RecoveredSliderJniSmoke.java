package dev.slinkywinky.attach;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * R167 Java8 -> disposable JNI -> real recovered Auto Block slider ->
 * validated native settings. No original game process/input hooks.
 */
public final class R167RecoveredSliderJniSmoke {
    // R168 uses the same disposable native menu owner for source paging.
    static native boolean navigate(int action);
    // R175 tests the same source-verified numeric track for passive hover.
    static native int[] sliderTarget(int permille);
    private static void require(boolean pass,String reason) {
        if(!pass)throw new AssertionError("R167 "+reason);
    }
    private static float blockRange() {
        String receipt=R166RetainedPointerJniSmoke.profile();
        require(receipt!=null &&
                receipt.startsWith("# SlinkyWinky standalone profile;") &&
                receipt.contains("\nversion=1\n"),
                "Original retained profile missing");
        final String needle="\nblock_range=";
        int start=receipt.indexOf(needle);
        require(start>=0,"Validated native block_range missing");
        start+=needle.length();
        int end=receipt.indexOf('\n',start);
        require(end>start,"Native source scalar has no terminator");
        return Float.parseFloat(receipt.substring(start,end));
    }
    private static boolean pointer(int phase,int x,int y,long time,
                                   boolean exclusive,boolean foreground,
                                   int width,int height) {
        return R166RetainedPointerJniSmoke.pointer(
            phase,x,y,time,exclusive,foreground,width,height);
    }
    private static boolean point(int phase,int x,int y,long time) {
        return pointer(phase,x,y,time,true,true,676,630);
    }
    public static void main(String[] args)throws Exception {
        require(args.length==1 &&
                args[0].endsWith("slinkywinky_r166_pointer_test_jni.dll"),
                "Only the segregated test JNI DLL is allowed");
        System.load(args[0]);
        require(!navigate(2) && sliderTarget(500)==null,
                "Unarmed native slider dispatch accepted");
        require(R166RetainedPointerJniSmoke.begin(),
                "Disposable WGL owner refused");
        try {
            require(!navigate(-1) && !navigate(0) && !navigate(7) &&
                    sliderTarget(500)==null,
                    "Unfocused source selector admitted");
            require(navigate(2),"Could not focus source Auto Block row");
            require(navigate(4),"Could not expand source Auto Block controls");
            int[] middle=sliderTarget(500);
            int[] invalidHigh=sliderTarget(980);
            int[] lower=sliderTarget(100);
            require(middle!=null && middle.length==2 &&
                    invalidHigh!=null && lower!=null &&
                    sliderTarget(-1)==null && sliderTarget(1000)==null,
                    "Recovered visible slider track not source-validated");
            final int x=middle[0],y=middle[1];
            final float initial=blockRange();
            require(initial>3.0f && initial<3.2f,
                    "Unexpected original native default block range");

            final AtomicBoolean foreignDenied=new AtomicBoolean(false);
            Thread foreign=new Thread(new Runnable() {
                @Override public void run() {
                    foreignDenied.set(!navigate(2) && sliderTarget(500)==null &&
                        !point(0,x,y,500));
                }
            });
            foreign.start();foreign.join();
            require(foreignDenied.get(),"Foreign JVM thread acquired native UI");

            require(!pointer(0,x,y,1000,false,true,676,630),
                    "Unattested synthetic exclusivity accepted");
            require(!point(2,x,y,1030),"Release after denied press accepted");
            require(!pointer(0,x,y,1100,true,false,676,630),
                    "Lost source foreground accepted");
            require(!pointer(0,x,y,1200,true,true,677,630),
                    "Wrong viewport accepted");
            require(blockRange()==initial,"Rejected source changed profile");
            // R184: source geometry from an unpainted navigation is never
            // a valid input target even in this disposable JNI fixture.
            require(!point(0,x,y,1900),"Unpainted source press admitted");
            require(R166RetainedPointerJniSmoke.redraw(),
                    "R184 post-navigation source repaint failed");
            require(point(0,x,y,2000),"Mid-slider press refused");
            require(point(2,x,y,2050),"Mid-slider release refused");
            float mid=blockRange();
            require(mid>4.2f && mid<4.8f,
                    "Java-to-native scalar not committed to real profile");
            require(R166RetainedPointerJniSmoke.redraw(),
                    "Retained WGL redraw failed after slider commit");
            require(Math.abs(blockRange()-mid)<0.0001f,
                    "Repaint modified committed slider value");

            // Source slider permits up to 8; native model validates <=6.
            // Release must be rejected with old settings unchanged.
            require(point(0,invalidHigh[0],invalidHigh[1],3000),
                    "High source slider press refused");
            require(!point(2,invalidHigh[0],invalidHigh[1],3050),
                    "Out-of-engine-range 7+ source slider was committed");
            require(Math.abs(blockRange()-mid)<0.0001f,
                    "Rejected numeric value mutated native settings");
            require(!point(0,lower[0],lower[1],3900),
                    "Rejected slider release left stale frame clickable");
            require(R166RetainedPointerJniSmoke.redraw(),
                    "R184 repaint after rejected slider value failed");
            require(point(0,lower[0],lower[1],4000),
                    "Low source slider press refused");
            require(point(2,lower[0],lower[1],4050),
                    "Low source slider release refused");
            float low=blockRange();
            require(low>1.3f && low<2.3f && low<mid,
                    "Java source slider low value failed native quantization");
            require(!point(0,x,y,4900),
                    "Committed slider value left unpainted frame clickable");
            require(R166RetainedPointerJniSmoke.redraw(),
                    "R184 repaint after slider commit failed");
            require(point(0,x,y,5000),"Cancel transaction setup refused");
            require(point(3,x,y,5010),"Pending numeric capture cancellation failed");
            require(!point(2,x,y,5020),"Cancelled slider capture committed");
            require(Math.abs(blockRange()-low)<0.0001f,
                    "Cancelled numeric gesture changed profile");

            require(point(0,x,y,6000),"Viewport-mismatch setup refused");
            require(!pointer(2,x,y,6050,true,true,677,630),
                    "Wrong WGL viewport committed pending slider");
            require(!point(2,x,y,6100),"Viewport abort did not cancel capture");
            require(Math.abs(blockRange()-low)<0.0001f,
                    "Viewport mismatch changed native settings");
            System.out.println("R167_JAVA8_JNI_RECOVERED_AUTO_BLOCK_SLIDER_PROFILE_PASS");
            System.out.println("R167_ENGINE_RANGE_DENIAL_AND_QUANTIZED_VALUES_PASS");
            System.out.println("R167_JNI_OWNER_FOCUS_VIEWPORT_CANCEL_DENIAL_PASS");
            System.out.println("R167_NO_ORIGINAL_MINECRAFT_INPUT_HOOK");
        } finally {
            require(R166RetainedPointerJniSmoke.end(),
                    "Disposable WGL owner did not cleanly release");
        }
        require(!navigate(2) && sliderTarget(500)==null,
                "Disposed JNI slider session still active");
    }
}
