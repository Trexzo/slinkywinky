package dev.slinkywinky.attach;

import dev.slinkywinky.instrument.KeyboardQueueOwner;
import dev.slinkywinky.instrument.MouseQueueOwner;

/**
 * R165 Java 8 test-only transactional GUI input model.
 * No production source references this class or its synthetic ownership
 * acknowledgments. It NEVER installs a hook into a Minecraft process.
 *
 * Invariant: the R164 whole-method early-return flag is switched ON before
 * either original queue owner is armed; it remains ON until both queues have
 * drained, physical controls are independently released, and the screen is
 * hidden. Any uncertainty remains fail-closed, requiring explicit restoration.
 */
public final class R165ModalInputSession {
    public enum Phase { UNBOUND, PASSIVE, MODAL, DRAINING, LOCKOUT }
    private final R161InputCoverageLedger coverage=
        R161InputCoverageLedger.fixtureOnly();
    private Phase phase=Phase.UNBOUND;
    private Thread owner;
    private boolean mouseUnbound, keyboardUnbound;
    private long lastEpoch;

    private boolean owner() {
        return owner!=null && Thread.currentThread()==owner;
    }
    private static boolean state(String expected) {
        return expected.equals(MouseQueueOwner.state()) &&
               expected.equals(KeyboardQueueOwner.state());
    }
    private void lockout() {
        // Revoke BOTH queue owners when either one fails. Otherwise a
        // still-MODAL peer prevents even independently proven restoration.
        R164OfflineModalGate.setForFixture(true);
        if(owner()) {
            MouseQueueOwner.observeForeground(false);
            KeyboardQueueOwner.observeForeground(false);
        }
        phase=Phase.LOCKOUT;
    }
    public Phase phase() {
        if(owner==null && phase==Phase.UNBOUND)return Phase.UNBOUND;
        return owner()?phase:Phase.LOCKOUT;
    }
    public boolean quarantined() {
        return R164OfflineModalGate.shouldQuarantineInput();
    }
    public long epoch() {
        return owner()?lastEpoch:0L;
    }
    public boolean bind(Class<?> mouse,Class<?> keyboard,ClassLoader loader,
                        boolean sourceVerified) {
        if(phase!=Phase.UNBOUND || !sourceVerified ||
           loader==null || R164OfflineModalGate.shouldQuarantineInput())
            return false;
        owner=Thread.currentThread();
        if(!MouseQueueOwner.bind(mouse,loader,true)){
            owner=null;return false;
        }
        if(!KeyboardQueueOwner.bind(keyboard,loader,true)) {
            // Test only: input methods were NEVER transformed. This is
            // not a valid real Minecraft rollback attestation.
            if(!MouseQueueOwner.unbind(true,true,true,true)) {
                lockout();return false;
            }
            owner=null;return false;
        }
        lastEpoch=0;
        phase=Phase.PASSIVE;
        return true;
    }
    public boolean open(String sha,long epoch,int fullCoverage,
                        boolean wholeMethodCutInstalled,
                        boolean foregroundVerified,
                        boolean allPhysicalReleased,boolean menuVisible,
                        int width,int height,
                        MouseQueueOwner.PointerSink mouseSink,
                        KeyboardQueueOwner.KeySink keySink) {
        if(!owner() || phase!=Phase.PASSIVE || epoch<=lastEpoch ||
           epoch<=0 || !R161InputCoverageLedger.PINNED_AVE_SHA256.equals(sha) ||
           fullCoverage!=R161InputCoverageLedger.ALL ||
           !wholeMethodCutInstalled || !foregroundVerified ||
           !allPhysicalReleased || !menuVisible ||
           mouseSink==null || width<320 || height<240 ||
           width>8192 || height>8192 || !state("VANILLA"))
            return false;
        if(!MouseQueueOwner.observeForeground(true) ||
           !KeyboardQueueOwner.observeForeground(true))
            return false;
        // Critical ordering: never arm a queue while the vanilla method
        // would still execute its direct-key/held-binding consumers.
        R164OfflineModalGate.setForFixture(true);
        final boolean mouseArmed=MouseQueueOwner.arm(
            epoch,true,true,true,width,height,mouseSink);
        final boolean keyboardArmed=mouseArmed &&
            KeyboardQueueOwner.arm(epoch,true,true,keySink);
        final boolean proof=keyboardArmed && coverage.tryOpen(
            sha,epoch,fullCoverage,true,true);
        if(!proof) {
            // Compensate a partially armed mock session. Never turn the
            // gate OFF while any queue remains modal or holds an event.
            if(mouseArmed)MouseQueueOwner.close(true,true);
            if(keyboardArmed)KeyboardQueueOwner.close(true);
            if(mouseArmed)MouseQueueOwner.next();
            if(keyboardArmed)KeyboardQueueOwner.next();
            if(state("VANILLA")) {
                R164OfflineModalGate.setForFixture(false);
                phase=Phase.PASSIVE;
            }else lockout();
            return false;
        }
        lastEpoch=epoch;
        phase=Phase.MODAL;
        return true;
    }
    public boolean poll(boolean foreground) {
        if(!owner() || phase!=Phase.MODAL)return false;
        if(!foreground) {
            MouseQueueOwner.observeForeground(false);
            KeyboardQueueOwner.observeForeground(false);
            lockout();
            return false;
        }
        if(!MouseQueueOwner.observeForeground(true) ||
           !KeyboardQueueOwner.observeForeground(true)) {
            lockout();return false;
        }
        // Both queue wrappers consume their mock LWJGL2 events. Neither
        // may return them to vanilla while the test gate is active.
        MouseQueueOwner.next();
        KeyboardQueueOwner.next();
        if(!state("MODAL")) {
            lockout();return false;
        }
        return true;
    }
    public boolean beginClose(boolean menuHidden) {
        if(!owner() || phase!=Phase.MODAL || !menuHidden)return false;
        R164OfflineModalGate.setForFixture(true);
        boolean mouse=MouseQueueOwner.close(true,true);
        boolean keyboard=KeyboardQueueOwner.close(true);
        if(!mouse || !keyboard) {
            lockout();return false;
        }
        phase=Phase.DRAINING;
        return true;
    }
    public boolean drain(boolean foreground,boolean verifiedEmpty,
                         boolean allPhysicalReleased) {
        if(!owner() || phase!=Phase.DRAINING)return false;
        if(!foreground) {
            MouseQueueOwner.observeForeground(false);
            KeyboardQueueOwner.observeForeground(false);
            lockout();return false;
        }
        if(!MouseQueueOwner.observeForeground(true) ||
           !KeyboardQueueOwner.observeForeground(true)) {
            lockout();return false;
        }
        // Do not pass any closing/held-key release through to vanilla.
        if(!"VANILLA".equals(MouseQueueOwner.state()))MouseQueueOwner.next();
        if(!"VANILLA".equals(KeyboardQueueOwner.state()))KeyboardQueueOwner.next();
        if("REFUSED".equals(MouseQueueOwner.state()) ||
           "REFUSED".equals(KeyboardQueueOwner.state())) {
            lockout();return false;
        }
        if(!state("VANILLA") || !verifiedEmpty || !allPhysicalReleased)
            return false;
        if(!coverage.tryClose(R161InputCoverageLedger.ALL,
                              true,true,true)) {
            lockout();return false;
        }
        phase=Phase.PASSIVE;
        R164OfflineModalGate.setForFixture(false);
        return true;
    }
    /**
     * Fixture-only shutdown/uncertain-path recovery. The bytecode flags are
     * synthetic assertions; they must NEVER be interpreted as runtime proof
     * of original loaded-class retransformation.
     */
    public boolean restore(boolean originalMouseBytesVerified,
                           boolean originalKeyboardBytesVerified,
                           boolean allPhysicalReleased,boolean menuHidden) {
        if(!owner() ||
           (phase!=Phase.LOCKOUT && phase!=Phase.PASSIVE) ||
           !originalMouseBytesVerified || !originalKeyboardBytesVerified ||
           !allPhysicalReleased || !menuHidden)
            return false;
        if(!mouseUnbound)
            mouseUnbound=MouseQueueOwner.unbind(true,true,true,true);
        if(!keyboardUnbound)
            keyboardUnbound=KeyboardQueueOwner.unbind(true,true,true);
        if(!mouseUnbound || !keyboardUnbound) {
            lockout();return false;
        }
        if(phase==Phase.LOCKOUT &&
           !coverage.tryClose(R161InputCoverageLedger.ALL,
                              true,true,true)) {
            // The source can now be restored, but no input is emitted.
            lockout();return false;
        }
        R164OfflineModalGate.setForFixture(false);
        phase=Phase.UNBOUND;
        owner=null;
        mouseUnbound=false;keyboardUnbound=false;
        return true;
    }
}
