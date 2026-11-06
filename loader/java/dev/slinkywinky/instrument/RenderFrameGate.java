package dev.slinkywinky.instrument;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * R75 bounded, render-thread-only observation gate.
 * Pure Java: never touches Minecraft state, graphics APIs, JNI or inputs.
 */
public final class RenderFrameGate {
    private final int limit;
    private final CountDownLatch finished=new CountDownLatch(1);
    private Thread owner;
    private int accepted;
    private boolean invalid;
    private boolean stopped;

    public RenderFrameGate(int limit) {
        this(limit,false);
    }

    /** R92 permits a single explicit 8-preflight + 64-draw epoch. */
    public RenderFrameGate(int limit,boolean multiOverlay) {
        this(limit,multiOverlay,false);
    }

    /** R94: opt-in, finite manual menu epoch; hard upper cap 2048 callbacks. */
    public RenderFrameGate(int limit,boolean multiOverlay,boolean manual) {
        if(manual ? (multiOverlay || limit!=2048) :
           (multiOverlay ? limit!=72 : (limit<1 || limit>8)))
            throw new IllegalArgumentException(
                "Only R75 1..8, R92 72, or R94 2048 explicitly admitted frames");
        this.limit=limit;
    }

    /** Only a real ave.av -> bfk.a call stack is eligible. */
    public static boolean hasOriginalCallsite(StackTraceElement[] stack) {
        if(stack==null)return false;
        for(int i=0;i+1<stack.length;i++) {
            StackTraceElement child=stack[i],parent=stack[i+1];
            if("bfk".equals(child.getClassName()) &&
               "a".equals(child.getMethodName()) &&
               "ave".equals(parent.getClassName()) &&
               "av".equals(parent.getMethodName()))
                return true;
        }
        return false;
    }

    /** Nonblocking and bounded. The first eligible callback claims thread ownership. */
    public synchronized void sample(Thread thread,float partial,long frameTime,
                                    StackTraceElement[] stack) {
        if(stopped)return;
        if(thread==null || !hasOriginalCallsite(stack) ||
           Float.isNaN(partial) || Float.isInfinite(partial) ||
           partial<0f || partial>1f || frameTime<0 ||
           (owner!=null && thread!=owner)) {
            invalid=true;
            stopped=true;
            finished.countDown();
            return;
        }
        if(owner==null)owner=thread;
        accepted++;
        if(accepted>=limit) {
            stopped=true;
            finished.countDown();
        }
    }

    public boolean await(long timeout,TimeUnit unit) throws InterruptedException {
        return finished.await(timeout,unit);
    }

    public synchronized void stop() {
        stopped=true;
        finished.countDown();
    }

    public synchronized int accepted() {return accepted;}
    public synchronized boolean invalid(){return invalid;}
    public synchronized String ownerThread() {
        return owner==null?"unobserved":owner.getName()+"#"+owner.getId();
    }
}
