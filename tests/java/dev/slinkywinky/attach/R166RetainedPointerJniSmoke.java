package dev.slinkywinky.attach;

import java.util.concurrent.atomic.AtomicBoolean;

/**
 * R166: Java 8 -> disposable JNI WGL -> REAL first-party C++ retained
 * menu. The native library is built solely from tests and is NEVER copied
 * into the injectable agent distribution.
 */
public final class R166RetainedPointerJniSmoke {
    static native boolean begin();
    static native int[] toggleTarget();
    static native boolean pointer(int phase,int x,int y,long monotonic,
                                          boolean exclusive,boolean foreground,
                                          int viewportWidth,int viewportHeight);
    static native String profile();
    static native boolean redraw();
    static native boolean end();
    private static void check(boolean valid,String reason) {
        if(!valid)throw new AssertionError("R166 "+reason);
    }
    private static boolean aim(String source,boolean enabled) {
        return source!=null && source.startsWith(
            "# SlinkyWinky standalone profile; no secrets or account data\nversion=1\n") &&
            source.contains("\naim_enabled="+enabled+"\n");
    }
    private static boolean gesture(int phase,int x,int y,long epoch) {
        return pointer(phase,x,y,epoch,true,true,676,630);
    }
    public static void main(String[] args)throws Exception {
        check(args.length==1,"Disposable DLL path argument required");
        check(args[0].endsWith("slinkywinky_r166_pointer_test_jni.dll"),
              "Never load the production injector/JNI DLL");
        System.load(args[0]);
        check(!gesture(0,1,1,100),"Pointer before WGL admission accepted");
        check(profile()==null && toggleTarget()==null && !redraw(),
              "Unarmed native UI leaked state");
        check(!end(),"Unarmed close accepted");
        check(begin(),"Test-only isolated WGL session rejected");
        try {
            check(!begin(),"Second WGL session admitted");
            final int[] toggle=toggleTarget();
            check(toggle!=null && toggle.length==2 &&
                  toggle[0]>0 && toggle[0]<676 &&
                  toggle[1]>0 && toggle[1]<630,
                  "Exact native source hit target not returned");
            final int x=toggle[0],y=toggle[1];
            check(aim(profile(),false),"Default native aim not disabled");

            check(!pointer(0,x,y,2000,false,true,676,630),
                  "No synthetic exclusive dispatch was admitted");
            check(!gesture(2,x,y,2010),
                  "Release without admitted press mutated settings");
            check(!pointer(0,x,y,2100,true,false,676,630),
                  "Lost synthetic foreground accepted");
            check(!pointer(0,x,y,2200,true,true,677,630),
                  "Mismatched source viewport accepted");
            check(!pointer(0,x,y,-1,true,true,676,630),
                  "Negative monotonic timestamp admitted");
            check(!pointer(99,x,y,2400,true,true,676,630),
                  "Unknown pointer phase admitted");
            check(aim(profile(),false),"Rejected events changed native profile");

            final AtomicBoolean foreignRefused=new AtomicBoolean(false);
            Thread foreign=new Thread(new Runnable() {
                @Override public void run() {
                    foreignRefused.set(!pointer(0,x,y,2500,true,true,676,630) &&
                        profile()==null && toggleTarget()==null &&
                        !redraw() && !end());
                }
            });
            foreign.start();foreign.join();
            check(foreignRefused.get(),"Foreign Java thread changed WGL owner");
            // R186: rejected same-owner viewport invalidated the paint.
            check(!gesture(0,x,y,2800),"Stale viewport hit revived before repaint");
            check(redraw(),"R186 input repaint after refused viewport failed");

            check(gesture(0,x,y,3000),"Valid press was refused");
            check(gesture(2,x,y,3040),"Valid release was refused");
            check(aim(profile(),true),"Java gesture did not update native settings");
            check(redraw(),"Retained native OpenGL redraw or state restore failed");
            check(aim(profile(),true),"Redraw discarded retained native profile");

            check(gesture(0,x,y,3100),"Repeat press rejected unexpectedly");
            check(!gesture(2,x,y,3140),"Double click unexpectedly committed");
            check(aim(profile(),true),"Double-click changed configuration");

            check(gesture(0,x,y,4000),"Cancel setup press failed");
            check(gesture(3,x,y,4010),"Cancellation failed");
            check(!gesture(2,x,y,4020),"Cancelled click was committed");
            check(aim(profile(),true),"Cancelled click changed configuration");

            check(gesture(0,x,y,4200),"Stale viewport setup refused");
            check(!pointer(2,x,y,4220,true,true,677,630),
                  "Release from changed viewport committed");
            check(!gesture(2,x,y,4240),"Stale viewport left click held");
            check(aim(profile(),true),"Changed viewport changed settings");
            check(!gesture(0,x,y,4500),"Stale click after viewport loss admitted");
            check(redraw(),"R186 repaint after abandoned press failed");

            check(gesture(0,x,y,5000),"Move-out setup refused");
            check(!gesture(1,-20,-20,5010),"Movement outside retained GUI accepted");
            check(!gesture(2,x,y,5020),"Drag-out release committed");
            check(aim(profile(),true),"Drag-out changed native settings");

            check(gesture(0,x,y,6000),"Second valid press refused");
            check(gesture(2,x,y,6040),"Second valid release refused");
            check(aim(profile(),false),"Second valid gesture failed to switch native profile");
            check(redraw(),"Second redraw failed");
            System.out.println("R166_JAVA8_JNI_NATIVE_RETAINED_POINTER_PROFILE_PASS");
            System.out.println("R166_CROSS_THREAD_VIEWPORT_CANCEL_GESTURE_GUARDS_PASS");
            System.out.println("R166_DISPOSABLE_WGL_NO_MINECRAFT_HOOK_PASS");
        } finally {
            check(end(),"Isolated native session could not close");
        }
        check(!gesture(0,1,1,7000) && profile()==null &&
              toggleTarget()==null && !redraw() && !end(),
              "Disposed WGL session accepted additional Java input");
    }
}
