package dev.slinkywinky.attach;

import java.util.concurrent.atomic.AtomicInteger;

/**
 * R76 actual Java 8 -> JNI -> WGL fixture acceptance.
 * Dedicated native library creates its own invisible window/context.
 * There is NO Minecraft Attach. The optional R77 fixture draws only in its\n * own disposable WGL window.
 */
public final class R76WglJvmSmoke {
    private static native boolean nativeBegin();
    private static native int nativeSample();
    private static native boolean nativeDrawPreview();
    private static native int nativeCount();
    private static native boolean nativeClearCurrent();
    private static native boolean nativeRestoreCurrent();
    private static native boolean nativeMismatchViewport();
    private static native boolean nativeUseAlternate();
    private static native boolean nativeReturnOriginal();
    private static native boolean nativeFinish();

    /** R77: permitted only for a disposable, system-classloader Java 8 fixture.
     * JNI calls here are never part of the shipped Minecraft instrumentation JAR.
     */
    private static void r77Gate() {
        if(!"1".equals(System.getenv("SLINKYWINKY_R76_WGL_FIXTURE")) ||
           !"1".equals(System.getenv("SLINKYWINKY_R77_DISPOSABLE_FIXTURE")))
            throw new IllegalStateException("R77 disposable JNI gate absent");
    }
    static void r77Load() {
        r77Gate();
        System.loadLibrary("slinkywinky_r76_wgl_fixture");
    }
    static boolean r77Begin() {r77Gate();return nativeBegin();}
    static int r77Sample() {r77Gate();return nativeSample();}
    static int r77Count() {r77Gate();return nativeCount();}
    static boolean r77Draw() {r77Gate();return nativeDrawPreview();}
    static boolean r77Finish() {r77Gate();return nativeFinish();}

    /** R78: isolated native WGL fixture controls, not shipped to the game. */
    private static void r78Gate() {
        if(!"1".equals(System.getenv("SLINKYWINKY_R76_WGL_FIXTURE")) ||
           !"1".equals(System.getenv("SLINKYWINKY_R78_WGL_FIXTURE")))
            throw new IllegalStateException("R78 disposable WGL fixture gate absent");
    }
    static void r78Load() {r78Gate();System.loadLibrary("slinkywinky_r76_wgl_fixture");}
    static boolean r78Begin() {r78Gate();return nativeBegin();}
    static boolean r78Finish() {r78Gate();return nativeFinish();}
    static boolean r78Clear() {r78Gate();return nativeClearCurrent();}
    static boolean r78Restore() {r78Gate();return nativeRestoreCurrent();}
    static boolean r78ChangeViewport() {r78Gate();return nativeMismatchViewport();}
    static boolean r78Switch() {r78Gate();return nativeUseAlternate();}
    static boolean r78Return() {r78Gate();return nativeReturnOriginal();}

    private static void require(boolean ok,String why) {
        if(!ok)throw new AssertionError(why);
    }
    public static void main(String[] args) throws Exception {
        if(!"1".equals(System.getenv("SLINKYWINKY_R76_WGL_FIXTURE")))
            throw new IllegalStateException("Disposable WGL fixture opt-in missing");
        System.loadLibrary("slinkywinky_r76_wgl_fixture");
        require(nativeSample()==0,"No-context sample unexpectedly accepted");
        require(!nativeDrawPreview(),"Native menu rendered without WGL context");
        require(nativeBegin(),"Unable to create isolated Java8 owned WGL context");
        require(!nativeBegin(),"Double WGL admission accepted");
        for(int i=1;i<=8;i++)
            require(nativeSample()==(i==8?2:1),"Unexpected finite WGL sample "+i);
        require(nativeSample()==0 && nativeCount()==8,
                "WGL sample cap failed closed");
        AtomicInteger foreignMenu=new AtomicInteger(-1);
        Thread foreignDraw=new Thread(new Runnable() {
            @Override public void run() {
                foreignMenu.set(nativeDrawPreview()?1:0);
            }
        },"R76-offthread-menu-denial");
        foreignDraw.start();foreignDraw.join(5000L);
        require(!foreignDraw.isAlive() && foreignMenu.get()==0,
                "GL11 menu accepted a non-owner Java render thread");
        require(nativeDrawPreview(),
                "JNI native GL11 menu draw failed to restore admitted WGL state");
        System.out.println("R76_JAVA8_JNI_GL11_MENU_RENDER_OWNER_AND_STATE_PASS");
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
