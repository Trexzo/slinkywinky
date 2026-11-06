package dev.slinkywinky.instrument;

import java.lang.reflect.Method;
import java.lang.reflect.Modifier;

/**
 * R110 first-party, disabled-by-default Java 8 LWJGL2 queue gate.
 * This class is NEVER installed at a Minecraft callsite by R110.
 * Without source admission it cannot open modal mode.
 *
 * A future verified transformer may use next() in place of Keyboard.next().
 * All other vanilla event accessors must remain unchanged.
 */
public final class KeyboardQueueOwner {
    public interface KeySink {
        void onEdge(int lwjglKey, boolean down);
    }
    private enum Mode { UNBOUND, VANILLA, MODAL, DRAINING, REFUSED }
    private static final int MAX_EVENTS_PER_POLL=128;
    private static final long FOCUS_MAX_AGE_NS=250000000L;
    private static Mode mode=Mode.UNBOUND;
    private static Thread owner;
    private static Method nextMethod, keyMethod, stateMethod;
    private static KeySink sink;
    private static long lastEpoch, focusTime;
    private static final boolean[] held=new boolean[256];

    private KeyboardQueueOwner() {}
    private static boolean ownerThread() {
        return owner!=null && Thread.currentThread()==owner;
    }
    private static boolean allReleased() {
        for(boolean pressed:held)if(pressed)return false;
        return true;
    }
    private static boolean staticMethod(Method method,Class<?> returns) {
        return Modifier.isPublic(method.getModifiers()) &&
               Modifier.isStatic(method.getModifiers()) &&
               returns.equals(method.getReturnType()) &&
               method.getParameterTypes().length==0;
    }
    /**
     * Bind only to an independently admitted original-game LWJGL classloader.
     * An arbitrary boolean here is NOT a live source/foreground attestation.
     */
    public static synchronized boolean bind(Class<?> lwjglKeyboard,
            ClassLoader admittedGameLoader,boolean exactSourceAttested) {
        if(mode!=Mode.UNBOUND || !exactSourceAttested ||
           lwjglKeyboard==null || admittedGameLoader==null ||
           lwjglKeyboard.getClassLoader()!=admittedGameLoader ||
           !"org.lwjgl.input.Keyboard".equals(lwjglKeyboard.getName()))
            return false;
        try {
            Method n=lwjglKeyboard.getMethod("next");
            Method k=lwjglKeyboard.getMethod("getEventKey");
            Method s=lwjglKeyboard.getMethod("getEventKeyState");
            if(!staticMethod(n,boolean.class) ||
               !staticMethod(k,int.class) ||
               !staticMethod(s,boolean.class))return false;
            owner=Thread.currentThread();
            nextMethod=n;keyMethod=k;stateMethod=s;
            mode=Mode.VANILLA;
            return true;
        }catch(ReflectiveOperationException badSource) {return false;}
    }
    /** Each modal poll needs a fresh source-pinned foreground heartbeat. */
    public static synchronized boolean observeForeground(boolean exactFocus) {
        if(!ownerThread() || mode==Mode.UNBOUND || mode==Mode.REFUSED)return false;
        if(!exactFocus) {
            if(mode==Mode.MODAL || mode==Mode.DRAINING)mode=Mode.REFUSED;
            focusTime=0;
            return false;
        }
        focusTime=System.nanoTime();
        return true;
    }
    public static synchronized boolean arm(long epoch,
            boolean exclusiveDispatchAttested,boolean allPhysicalKeysReleased,
            KeySink admittedSink) {
        if(!ownerThread() || mode!=Mode.VANILLA ||
           epoch<=lastEpoch || epoch<=0 || admittedSink==null ||
           !exclusiveDispatchAttested || !allPhysicalKeysReleased ||
           !allReleased() || focusTime==0 ||
           System.nanoTime()-focusTime>FOCUS_MAX_AGE_NS)
            return false;
        lastEpoch=epoch;
        sink=admittedSink;
        mode=Mode.MODAL;
        return true;
    }
    /**
     * EXACT signature required by R109's single Keyboard.next() callsite.
     * Never proxies another keyboard method or constructs a game key event.
     * If no owner is admitted, deliberately refuses rather than assuming
     * a GUI may consume input.
     */
    public static synchronized boolean next() {
        if(!ownerThread() || mode==Mode.UNBOUND || mode==Mode.REFUSED)
            return false;
        if(mode==Mode.VANILLA) {
            try {return ((Boolean)nextMethod.invoke(null)).booleanValue();}
            catch(Throwable invalidSource) {mode=Mode.REFUSED;return false;}
        }
        if(focusTime==0 || System.nanoTime()-focusTime>FOCUS_MAX_AGE_NS) {
            mode=Mode.REFUSED;
            return false;
        }
        try {
            int consumed=0;
            boolean exhausted=false;
            while(consumed<MAX_EVENTS_PER_POLL) {
                if(!((Boolean)nextMethod.invoke(null)).booleanValue()) {
                    exhausted=true;break;
                }
                consumed++;
                int key=((Integer)keyMethod.invoke(null)).intValue();
                boolean down=((Boolean)stateMethod.invoke(null)).booleanValue();
                if(key<0 || key>=held.length) {
                    mode=Mode.REFUSED;return false;
                }
                // Queue events with key 0 are character-only and remain
                // withheld; no local text entry is enabled in R110.
                if(key==0)continue;
                boolean edge=held[key]!=down;
                held[key]=down;
                if(edge && mode==Mode.MODAL) {
                    sink.onEdge(key,down); // only local preview observer
                }
            }
            // Require observing a fully empty queue AFTER all modal-owned
            // releases. Do not forward the closing key or its key-up.
            if(mode==Mode.DRAINING && exhausted && allReleased()) {
                mode=Mode.VANILLA;
                sink=null;
            }
            return false;
        }catch(Throwable failure) {
            mode=Mode.REFUSED;return false;
        }
    }
    /** The host has already hidden the overlay; drain held keys to release. */
    public static synchronized boolean close(boolean verifiedMenuHidden) {
        if(!ownerThread() || mode!=Mode.MODAL || !verifiedMenuHidden)return false;
        mode=Mode.DRAINING;
        return true;
    }
    /** Only an independently proven bytecode restore can remove a refusal. */
    public static synchronized boolean unbind(boolean originalClassRestored,
            boolean allPhysicalKeysReleased,boolean menuHidden) {
        if(!ownerThread() || mode==Mode.MODAL ||
           !originalClassRestored || !allPhysicalKeysReleased || !menuHidden)
            return false;
        // Never reopen vanilla with old GUI-owned events still queued.
        // A nonempty queue is quarantined one event at a time and requires
        // another independently attested release attempt.
        try {
            if(((Boolean)nextMethod.invoke(null)).booleanValue())return false;
        }catch(Throwable uncertainty) {mode=Mode.REFUSED;return false;}
        mode=Mode.UNBOUND;
        nextMethod=null;keyMethod=null;stateMethod=null;
        sink=null;owner=null;focusTime=0;
        java.util.Arrays.fill(held,false);
        return true;
    }
    public static synchronized String state() {
        return ownerThread()?mode.name():"UNAVAILABLE";
    }
    public static synchronized long epoch() {
        return ownerThread()?lastEpoch:0;
    }
}
