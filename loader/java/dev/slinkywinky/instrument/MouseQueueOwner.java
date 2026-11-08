package dev.slinkywinky.instrument;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.util.Arrays;

/**
 * R160: dormant Java 8 LWJGL2 mouse-queue ownership model.
 * Nothing in the production agent calls bind/arm/next. In particular,
 * intercepting Mouse.next() alone is UNSAFE on original ave.s()V, whose
 * vanilla mouse, keyboard and held-keybinding consumers remain reachable.
 */
public final class MouseQueueOwner {
    public interface PointerSink {
        // x/y are top-left CLIENT pixel coordinates, never screen coords.
        void onPointer(int phase,int x,int y);
    }
    public static final int DOWN=0, MOVE=1, UP=2;
    private static final int MAX_POLL=128;
    private static final int MAX_BUTTONS=8;
    private static final long FOCUS_AGE_NS=250000000L;
    private enum Mode { UNBOUND, VANILLA, MODAL, DRAINING, REFUSED }
    private static Mode mode=Mode.UNBOUND;
    private static Thread owner;
    private static Method nextMethod, buttonMethod, stateMethod,
                          xMethod, yMethod, wheelMethod;
    private static PointerSink sink;
    private static long lastEpoch, focusTime;
    private static int width,height;
    private static final boolean[] held=new boolean[MAX_BUTTONS];

    private MouseQueueOwner() {}
    private static boolean ownerThread() {
        return owner!=null && Thread.currentThread()==owner;
    }
    private static boolean allReleased() {
        for(boolean down:held)if(down)return false;
        return true;
    }
    private static boolean methodOk(Method m,Class<?> returns) {
        return Modifier.isPublic(m.getModifiers()) &&
            Modifier.isStatic(m.getModifiers()) &&
            returns.equals(m.getReturnType()) &&
            m.getParameterTypes().length==0;
    }
    /** Only a separate source-pinned classloader may authorize the bind. */
    public static synchronized boolean bind(Class<?> lwjglMouse,
            ClassLoader admittedGameLoader,boolean exactSourceAttested) {
        if(mode!=Mode.UNBOUND || !exactSourceAttested ||
           lwjglMouse==null || admittedGameLoader==null ||
           lwjglMouse.getClassLoader()!=admittedGameLoader ||
           !"org.lwjgl.input.Mouse".equals(lwjglMouse.getName()))
            return false;
        try {
            Method n=lwjglMouse.getMethod("next");
            Method b=lwjglMouse.getMethod("getEventButton");
            Method s=lwjglMouse.getMethod("getEventButtonState");
            Method x=lwjglMouse.getMethod("getEventX");
            Method y=lwjglMouse.getMethod("getEventY");
            Method wheel=lwjglMouse.getMethod("getEventDWheel");
            if(!methodOk(n,boolean.class)||!methodOk(b,int.class)||
               !methodOk(s,boolean.class)||!methodOk(x,int.class)||
               !methodOk(y,int.class)||!methodOk(wheel,int.class))
                return false;
            nextMethod=n;buttonMethod=b;stateMethod=s;
            xMethod=x;yMethod=y;wheelMethod=wheel;
            owner=Thread.currentThread();
            mode=Mode.VANILLA;
            return true;
        } catch(ReflectiveOperationException invalid) {return false;}
    }
    public static synchronized boolean observeForeground(boolean focused) {
        if(!ownerThread() || mode==Mode.UNBOUND || mode==Mode.REFUSED)
            return false;
        if(!focused) {
            focusTime=0;
            if(mode==Mode.MODAL || mode==Mode.DRAINING)mode=Mode.REFUSED;
            return false;
        }
        focusTime=System.nanoTime();
        return true;
    }
    /**
     * NOT a live input admission API: callers still need independently
     * verified and *restorable* suppression of every ave.s()V gameplay
     * consumer, not merely a mouse/keyboard queue replacement.
     */
    public static synchronized boolean arm(long epoch,
            boolean fullInputOwnershipAttested,
            boolean allPhysicalKeysAndButtonsReleased,
            boolean menuVisible,int clientWidth,int clientHeight,
            PointerSink callback) {
        if(!ownerThread() || mode!=Mode.VANILLA || epoch<=lastEpoch ||
           epoch<=0 || !fullInputOwnershipAttested ||
           !allPhysicalKeysAndButtonsReleased || !menuVisible ||
           callback==null || !allReleased() ||
           clientWidth<320 || clientWidth>8192 ||
           clientHeight<240 || clientHeight>8192 ||
           focusTime==0 || System.nanoTime()-focusTime>FOCUS_AGE_NS)
            return false;
        lastEpoch=epoch;
        width=clientWidth;height=clientHeight;
        sink=callback;
        mode=Mode.MODAL;
        return true;
    }
    /**
     * Callsite-shaped test relay. VANILLA preserves original next() and
     * event metadata. MODAL swallows all input, permitting only left-button
     * edges and movement to the first-party pointer sink.
     */
    public static synchronized boolean next() {
        if(!ownerThread() || mode==Mode.UNBOUND || mode==Mode.REFUSED)
            return false;
        if(mode==Mode.VANILLA) {
            try {return ((Boolean)nextMethod.invoke(null)).booleanValue();}
            catch(Throwable failure) {mode=Mode.REFUSED;return false;}
        }
        if(focusTime==0 || System.nanoTime()-focusTime>FOCUS_AGE_NS) {
            mode=Mode.REFUSED;return false;
        }
        try {
            int count=0;
            boolean exhausted=false;
            while(count<MAX_POLL) {
                if(!((Boolean)nextMethod.invoke(null)).booleanValue()) {
                    exhausted=true;break;
                }
                count++;
                int button=((Integer)buttonMethod.invoke(null)).intValue();
                boolean down=((Boolean)stateMethod.invoke(null)).booleanValue();
                int x=((Integer)xMethod.invoke(null)).intValue();
                int y=((Integer)yMethod.invoke(null)).intValue();
                int wheel=((Integer)wheelMethod.invoke(null)).intValue();
                // Mouse origin is bottom-left in LWJGL2; native menu origin
                // is top-left. Mismatched viewport is never guessed.
                if(button<-1 || button>=MAX_BUTTONS ||
                   x<0 || x>=width || y<0 || y>=height) {
                    mode=Mode.REFUSED;return false;
                }
                if(button>=0) {
                    boolean edge=held[button]!=down;
                    held[button]=down;
                    if(edge && mode==Mode.MODAL && button==0)
                        sink.onPointer(down?DOWN:UP,x,height-1-y);
                } else if(mode==Mode.MODAL && wheel==0 && held[0]) {
                    sink.onPointer(MOVE,x,height-1-y);
                }
                // Wheel, right-click and non-left presses are withheld.
            }
            if(mode==Mode.DRAINING && exhausted && allReleased()) {
                mode=Mode.VANILLA;
                sink=null;width=height=0;
            }
            return false;
        }catch(Throwable failure) {
            mode=Mode.REFUSED;return false;
        }
    }
    public static synchronized boolean close(boolean menuHidden,
                                               boolean keyboardQuarantined) {
        if(!ownerThread() || mode!=Mode.MODAL ||
           !menuHidden || !keyboardQuarantined)return false;
        mode=Mode.DRAINING;
        return true;
    }
    /** Refusal needs an independently verified original bytecode restore. */
    public static synchronized boolean unbind(boolean originalMouseRestored,
            boolean allPhysicalControlsReleased,boolean menuHidden,
            boolean keyboardRestored) {
        if(!ownerThread() || mode==Mode.MODAL ||
           !originalMouseRestored || !allPhysicalControlsReleased ||
           !menuHidden || !keyboardRestored)return false;
        try {
            if(((Boolean)nextMethod.invoke(null)).booleanValue())return false;
        } catch(Throwable failure) {mode=Mode.REFUSED;return false;}
        mode=Mode.UNBOUND;owner=null;nextMethod=null;
        buttonMethod=null;stateMethod=null;xMethod=null;yMethod=null;
        wheelMethod=null;sink=null;focusTime=0;width=height=0;
        Arrays.fill(held,false);
        return true;
    }
    public static synchronized String state() {
        return ownerThread()?mode.name():"UNAVAILABLE";
    }
    public static synchronized long epoch() {
        return ownerThread()?lastEpoch:0;
    }
}
