package dev.slinkywinky.instrument;

/**
 * R79: single-lock admission-to-observation fence for original renderer frames.
 *
 * A native observer may run ONLY after RenderFrameGate has accepted that
 * exact thread, callsite and input. stopAndDrain() blocks until the current
 * observer returns, then permanently refuses late or racing callbacks.
 * No JNI, game state, GL or instrumentation is used by this class itself.
 */
public final class RenderCallbackFence {
    private final RenderFrameGate gate;
    private final Runnable observer;
    private boolean closed;
    private boolean observerFailed;

    public RenderCallbackFence(RenderFrameGate gate,Runnable observer) {
        if(gate==null)throw new IllegalArgumentException("Render gate required");
        this.gate=gate;
        this.observer=observer;
    }

    /** This entire transition is atomic with stopAndDrain(). */
    public synchronized void sample(Thread thread,float partialTicks,long frameTime,
                                    StackTraceElement[] callsite) {
        if(closed || observerFailed)return;
        try {
            final int prior=gate.accepted();
            // Owner thread and original ave.av -> bfk.a stack MUST be validated
            // before any native WGL query can be made.
            gate.sample(thread,partialTicks,frameTime,callsite);
            if(observer!=null && !gate.invalid() &&
               gate.accepted()==prior+1)
                observer.run();
        } catch(Throwable unexpected) {
            observerFailed=true;
            gate.stop(); // Never throw exceptions into Mojang rendering.
        }
    }

    /** Call before retransformation. Joins any in-flight JNI observation. */
    public synchronized void stopAndDrain() {
        closed=true;
        gate.stop();
    }

    /** Unexpected callback failures must invalidate successful completion. */
    public synchronized void abort() {
        observerFailed=true;
        stopAndDrain();
    }

    public synchronized boolean failed() {return observerFailed;}
    public synchronized boolean closed() {return closed;}
}
