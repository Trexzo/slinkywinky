package dev.slinkywinky.instrument;

/**
 * R174: OBSERVATION-ONLY, single-render-thread classifier for original-window
 * pointer samples and original Minecraft GuiScreen presence.
 *
 * This cannot own or suppress Minecraft input. No action, event, release,
 * movement, keybind, or GUI click is ever emitted from this class.
 * A "candidate" is explicitly a POTENTIAL VANILLA DOUBLE-DISPATCH RISK,
 * never authorization to send that click to the retained overlay.
 */
public final class OriginalPointerArbitrationAudit {
    public static final int REFUSED=-1;
    public static final int BACKGROUND=0;
    public static final int NO_GUI=1;
    public static final int RELEASED=2;
    public static final int QUARANTINED=3;
    public static final int UNSAFE_PRESS=4;
    public static final int UNSAFE_RELEASE=5;

    private Thread owner;
    private int width,height;
    private boolean viewportPinned,stopped;
    private boolean zeroSeen,heldCandidate;
    private int lastMask;
    private int frames,foreground,background;
    private int candidatePresses,candidateReleases,cancelled;
    private int quarantined,refused;

    // A deliberately permanent deny: the original game's other consumers
    // and full original loaded-class restoration are NOT yet controlled.
    public boolean mayDeliverMenuClick(){return false;}
    public boolean exclusiveOriginalInputOwner(){return false;}

    public synchronized int observe(int originalScreen,boolean exactForeground,
                                    int[] pointer,int clientWidth,int clientHeight){
        if(owner==null)owner=Thread.currentThread();
        if(owner!=Thread.currentThread()){
            refused++;
            return REFUSED;
        }
        if(stopped || originalScreen<OriginalGuiScreenProbe.NO_SCREEN ||
           originalScreen>OriginalGuiScreenProbe.SCREEN_PRESENT ||
           clientWidth<320 || clientWidth>8192 ||
           clientHeight<240 || clientHeight>8192){
            refused++;stopped=true;return REFUSED;
        }
        if(!viewportPinned){
            width=clientWidth;height=clientHeight;viewportPinned=true;
        }else if(width!=clientWidth || height!=clientHeight){
            // An owner/viewport transition demands an entirely new audit.
            if(heldCandidate)cancelled++;
            heldCandidate=false;zeroSeen=false;
            stopped=true;refused++;return REFUSED;
        }
        if(!exactForeground){
            if(pointer!=null){
                refused++;stopped=true;return REFUSED;
            }
            // Losing foreground revokes the entire pending gesture. The
            // next foreground sample MUST observe all buttons released.
            if(heldCandidate)cancelled++;
            heldCandidate=false;zeroSeen=false;lastMask=0;
            background++;frames++;
            return BACKGROUND;
        }
        if(!RendererWglEvidence.validOriginalPointerReceipt(
                pointer,clientWidth,clientHeight)){
            if(heldCandidate)cancelled++;
            heldCandidate=false;zeroSeen=false;
            stopped=true;refused++;
            return REFUSED;
        }
        foreground++;frames++;
        final int mask=pointer[3];
        final boolean left=(mask&1)!=0;
        final boolean previouslyLeft=(lastMask&1)!=0;
        final boolean inside=pointer[2]==1;
        final boolean gameGui=originalScreen==
            OriginalGuiScreenProbe.SCREEN_PRESENT;
        lastMask=mask;

        // All buttons up must be sampled before any rising-edge estimate.
        // Button polling can miss edges; no mode grants event ownership.
        if(mask==0){
            if(heldCandidate && previouslyLeft){
                heldCandidate=false;zeroSeen=true;
                candidateReleases++;
                return UNSAFE_RELEASE;
            }
            heldCandidate=false;zeroSeen=true;
            return gameGui?RELEASED:NO_GUI;
        }
        if(!left || mask!=1 || !inside || !gameGui){
            if(heldCandidate)cancelled++;
            heldCandidate=false;
            zeroSeen=false;
            quarantined++;
            return gameGui?QUARANTINED:NO_GUI;
        }
        if(!zeroSeen){
            // Initially held or resumed from a focus loss: no new press
            // can be inferred, much less delivered to a retained menu.
            quarantined++;
            return QUARANTINED;
        }
        if(!previouslyLeft && !heldCandidate){
            heldCandidate=true;
            candidatePresses++;
            // Vanilla mouse readers outside ave.s()V remain live.
            return UNSAFE_PRESS;
        }
        return heldCandidate?UNSAFE_PRESS:QUARANTINED;
    }

    public synchronized boolean validSummary(int expectedFrames){
        // RendererFrameDiagnostic calls this after stopAndDrain() on its
        // cleanup thread. No input is read here; synchronized summary
        // is safe across threads, unlike observe() which pins the owner.
        return owner!=null && !stopped &&
            frames==expectedFrames && refused==0 &&
            foreground+background==expectedFrames &&
            candidateReleases<=candidatePresses &&
            !mayDeliverMenuClick();
    }
    public synchronized int samples(){return frames;}
    public synchronized int possibleVanillaOverlapPresses(){return candidatePresses;}
    public synchronized int possibleVanillaOverlapReleases(){return candidateReleases;}
    public synchronized int cancelledGestures(){return cancelled;}
    public synchronized int refusedSamples(){return refused;}
    public synchronized int quarantinedSamples(){return quarantined;}
}
