package dev.slinkywinky.attach;

/**
 * R77 test-only receiver invoked directly from a transformed synthetic
 * bfk.a(FJ)V method. All calls run on its original Java render-owner thread.
 * The JNI target is the disposable window created by R76, never Minecraft.
 */
public final class R77WglRenderReceiver {
    private static Thread owner;
    private static int callbacks;
    private static boolean drawn;
    private static boolean invalid;
    private static boolean closed;

    public static synchronized void beginOwner() {
        if(owner!=null || closed ||
           !"1".equals(System.getenv("SLINKYWINKY_R77_DISPOSABLE_FIXTURE")))
            throw new IllegalStateException("R77 render receiver not fresh");
        owner=Thread.currentThread();
    }
    public static void onRender(float partialTicks,long frameTime) {
        // Test-only WGL calls must happen directly on the admitted render thread.
        if(closed || invalid || callbacks>=8)return;
        if(Thread.currentThread()!=owner ||
           Float.isNaN(partialTicks) || Float.isInfinite(partialTicks) ||
           partialTicks<0f || partialTicks>1f || frameTime<0) {
            invalid=true;
            return;
        }
        try {
            final int actual=R76WglJvmSmoke.r77Sample();
            callbacks++;
            if(actual!=(callbacks==8?2:1)) {
                invalid=true;
                return;
            }
            if(callbacks==8)drawn=R76WglJvmSmoke.r77Draw();
            if(callbacks==8 && !drawn)invalid=true;
        } catch(Throwable problem) {
            invalid=true;
        }
    }
    public static synchronized int completeOwner() {
        if(owner!=Thread.currentThread() || closed)
            throw new IllegalStateException("R77 receiver owner mismatch");
        if(invalid || !drawn || callbacks!=8 ||
           R76WglJvmSmoke.r77Count()!=8)
            throw new IllegalStateException(
                "R77 native GL11 render callback not proven: accepted="+callbacks+
                " invalid="+invalid+" drawn="+drawn);
        closed=true;
        return callbacks;
    }
    public static synchronized int count() {return callbacks;}
    public static synchronized boolean invalid() {return invalid;}
    private R77WglRenderReceiver(){}
}
