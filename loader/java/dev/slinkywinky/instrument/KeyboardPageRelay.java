package dev.slinkywinky.instrument;

/**
 * R118 dormant Java 8 bridge from an ALREADY ADMITTED KeyboardQueueOwner
 * key edge to the first-party native preview's page action contract.
 *
 * This class cannot poll input, bind LWJGL, start a hook, change game state,
 * or infer original-client ownership. It is intentionally never armed by
 * InstrumentationProbe. Only the mock-LWJGL test invokes it in R118.
 */
public final class KeyboardPageRelay implements KeyboardQueueOwner.KeySink {
    // Confirmed against actual LWJGL 2.9.4 org.lwjgl.input.Keyboard.
    public static final int KEY_PRIOR=201;
    public static final int KEY_NEXT=209;
    public interface PageSink {
        /** direction is -1 (previous) or +1 (next); no game commands. */
        void onPage(int direction,long epoch);
    }
    private final Thread owner;
    private final PageSink receiver;
    private final long epoch;
    private final boolean[] pressed=new boolean[2];
    private boolean disposed;

    public KeyboardPageRelay(long admittedEpoch,PageSink firstPartyReceiver) {
        if(admittedEpoch<=0 || firstPartyReceiver==null)
            throw new IllegalArgumentException("Missing owner epoch or receiver");
        owner=Thread.currentThread();
        receiver=firstPartyReceiver;
        epoch=admittedEpoch;
    }
    private boolean ownerThread() {
        return Thread.currentThread()==owner;
    }
    @Override public synchronized void onEdge(int lwjglCode,boolean down) {
        if(!ownerThread() || disposed)
            throw new IllegalStateException("Page sink owner revoked");
        final int index;
        if(lwjglCode==KEY_PRIOR)index=0;
        else if(lwjglCode==KEY_NEXT)index=1;
        else return; // all other keys remain owned by KeyboardQueueOwner
        final boolean rising=down && !pressed[index];
        pressed[index]=down;
        if(rising)
            receiver.onPage(index==0?-1:1,epoch);
    }
    /** Independent original-loop restore remains the native host's job. */
    public synchronized boolean dispose(boolean menuHidden,
            boolean controlsReleased,boolean originalLoopRestored) {
        if(!ownerThread() || disposed ||
           !menuHidden || !controlsReleased || !originalLoopRestored)
            return false;
        if(pressed[0] || pressed[1])return false;
        disposed=true;
        return true;
    }
    public synchronized boolean active() {
        return ownerThread() && !disposed;
    }
    public synchronized long epoch() {
        return ownerThread()?epoch:0;
    }
}
