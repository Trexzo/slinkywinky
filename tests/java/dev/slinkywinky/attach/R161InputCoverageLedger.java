package dev.slinkywinky.attach;

/**
 * R161 fixture-only inventory of all observed vanilla Minecraft 1.8.9
 * input consumers. A one-queue rewrite is NOT a positive admission.
 * Never package in the agent or use as real-world provenance.
 */
public final class R161InputCoverageLedger {
    public static final String PINNED_AVE_SHA256 =
        "430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82";
    public static final int MOUSE_QUEUE = 1<<0;
    public static final int MOUSE_BUTTONS = 1<<1;
    public static final int MOUSE_WHEEL = 1<<2;
    public static final int KEYBOARD_QUEUE = 1<<3;
    public static final int RAW_KEY_STATE = 1<<4;
    public static final int PRE_KEY_STATE = 1<<5;
    public static final int POST_KEY_STATE = 1<<6;
    public static final int PRE_KEY_PRESSES = 1<<7;
    public static final int POST_KEY_PRESSES = 1<<8;
    public static final int HELD_BINDING_POLL = 1<<9;
    public static final int PRESS_BINDING_POLL = 1<<10;
    public static final int GUI_DISPATCH = 1<<11;
    public static final int PHYSICAL_RELEASE = 1<<12;
    public static final int FOREGROUND = 1<<13;
    public static final int ROLLBACK_BYTES = 1<<14;
    public static final int DRAIN_BOTH = 1<<15;
    public static final int ALL = 0xffff;
    private long lastEpoch;
    private boolean opened;

    /**
     * Test-only positive path. A fabricated bitmask cannot attest a live
     * original JVM; this asserts that a future proof must cover EVERY path.
     */
    public boolean tryOpen(String sourceSha,long epoch,int covered,
                           boolean sameClientThread,
                           boolean keyboardAndMouseOwnersBound) {
        if(opened || epoch<=lastEpoch || epoch<=0 ||
           !PINNED_AVE_SHA256.equals(sourceSha) || covered!=ALL ||
           !sameClientThread || !keyboardAndMouseOwnersBound)
            return false;
        lastEpoch=epoch;
        opened=true;
        return true;
    }

    /** No pass-through after partial rollback, stale events, or lost owner. */
    public boolean tryClose(int covered,boolean mouseVanilla,
                            boolean keyboardVanilla,boolean queuesEmpty) {
        if(!opened || covered!=ALL || !mouseVanilla ||
           !keyboardVanilla || !queuesEmpty)return false;
        opened=false;
        return true;
    }
    public boolean isOpen() {return opened;}
    public long epoch() {return lastEpoch;}
    private R161InputCoverageLedger() {}
    public static R161InputCoverageLedger fixtureOnly() {
        return new R161InputCoverageLedger();
    }
}
