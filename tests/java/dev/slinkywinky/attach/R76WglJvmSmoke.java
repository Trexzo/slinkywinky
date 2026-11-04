package dev.slinkywinky.attach;

import java.util.concurrent.atomic.AtomicInteger;

/**
 * R76 actual Java 8 -> JNI -> WGL fixture acceptance.
 * Dedicated native library creates its own invisible window/context.
 * There is NO Minecraft Attach and NO OpenGL menu drawing.
 */
public final class R76WglJvmSmoke {
    private static native boolean nativeBegin();
    private static native int nativeSample();
    private static native int nativeCount();
    private static native boolean nativeClearCurrent();
    private static native boolean nativeRestoreCurrent();
    private static native boolean nativeMismatchViewport();
    private static native boolean nativeUseAlternate();
    private static native boolean nativeReturnOriginal();
    private static native boolean nativeFinish();

    private static void require(boolean ok,String why) {
        if(!ok)throw new AssertionError(why);
    }
    public static void main(String[] args) throws Exception {
        if(!"1".equals(System.getenv("SLINKYWINKY_R76_WGL_FIXTURE")))
            throw new IllegalStateException("Disposable WGL fixture opt-in missing");
        System.loadLibrary("slinkywinky_r76_wgl_fixture");
        require(nativeSample()==0,"No-context sample unexpectedly accepted");
        require(nativeBegin(),"Unable to create isolated Java8 owned WGL context");
        require(!nativeBegin(),"Double WGL admission accepted");
        for(int i=1;i<=8;i++)
            require(nativeSample()==(i==8?2:1),"Unexpected finite WGL sample "+i);
        require(nativeSample()==0 && nativeCount()==8,
                "WGL sample cap failed closed");
        require(nativeFinish(),"Clean bounded context was not released");
        System.out.println("R76_JAVA8_JNI_WGL_CONTEXT_EIGHT_FRAME_PASS");

        require(nativeBegin(),"Second disposable WGL epoch refused");
        AtomicInteger offThread=new AtomicInteger(-1);
        Thread foreign=new Thread(new Runnable() {
            @Override public void run() {
                offThread.set(nativeSample());
            }
        },"R76-no-current-foreign-thread");
        foreign.start();
        foreign.join(5000L);
        require(!foreign.isAlive() && offThread.get()==0,
                "Java foreign thread was admitted to WGL owner");
        require(nativeSample()==0 && nativeCount()==0,
                "Off-thread incident must invalidate context epoch");
        require(!nativeFinish(),"Invalid WGL epoch unexpectedly clean");
        System.out.println("R76_JNI_FOREIGN_RENDER_THREAD_FAIL_CLOSED_PASS");

        require(nativeBegin(),"Context-loss fixture unavailable");
        require(nativeSample()==1,"Valid WGL first sample denied");
        require(nativeClearCurrent(),"Unable to simulate lost WGL context");
        require(nativeSample()==0 && nativeCount()==1,
                "No-context sampling did not reject");
        require(nativeRestoreCurrent(),"Fixture context restoration unavailable");
        require(nativeSample()==0,
                "Lost context was silently re-admitted in the same epoch");
        require(!nativeFinish(),"Context-loss epoch incorrectly marked clean");
        System.out.println("R76_WGL_CONTEXT_LOSS_STICKY_REFUSAL_PASS");

        require(nativeBegin(),"Context-switch fixture unavailable");
        require(nativeSample()==1,"Initial admitted context sample denied");
        require(nativeUseAlternate(),"Unable to switch to disposable alternate WGL context");
        require(nativeSample()==0 && nativeCount()==1,
                "Different WGL context admitted to original owner epoch");
        require(nativeReturnOriginal(),"Unable to return to original owner WGL context");
        require(nativeSample()==0,"New WGL context invalidation incorrectly cleared");
        require(!nativeFinish(),"Changed WGL context epoch marked clean");
        System.out.println("R76_ALTERNATE_WGL_CONTEXT_SWITCH_FAIL_CLOSED_PASS");

        require(nativeBegin(),"Viewport fixture unavailable");
        require(nativeMismatchViewport(),"Could not exercise viewport mutation");
        require(nativeSample()==0 && nativeCount()==0,
                "Changed viewport unexpectedly admitted");
        require(!nativeFinish(),"Changed viewport context treated as clean");
        System.out.println("R76_WGL_VIEWPORT_MISMATCH_FAIL_CLOSED_PASS");

        require(nativeSample()==0,"Destroyed WGL context still accepted frames");
        System.out.println("R76_JAVA8_WGL_DISPOSABLE_CONTEXT_RELEASE_PASS");
        System.out.println("R76_NO_MINECRAFT_OR_NATIVE_MENU_DRAWING");
    }
    private R76WglJvmSmoke(){}
}
