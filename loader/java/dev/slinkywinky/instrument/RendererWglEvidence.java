package dev.slinkywinky.instrument;

import java.io.File;

/**
 * R78: passive WGL evidence by default. R91/R92/R93 require distinct,
 * explicitly opted-in finite overlay sessions on the admitted game thread.
 * R93 uses a deterministic show/hide/show schedule, never game input.
 * No WGL pointer values are emitted to logs or exposed as gameplay controls.
 */
public final class RendererWglEvidence {
    private static native long[] nativeCurrentWglSnapshot();
    private static native int nativeAuditRenderWindow(long context,long dc,int width,int height);
    // R172: passive physical cursor evidence only in original-only window
    // audit; NEVER forwarded to native retained GUI or original game keys.
    private static native int[] nativeObservePhysicalPointer(
        long context,long dc,int width,int height);
    private static native boolean nativeDrawReadOnlyOverlay(
        long context,long dc,int width,int height);
    private static native boolean nativeDrawCompactOverlay(
        long context,long dc,int width,int height);
    private static native boolean nativeOpenManualVisibility();
    private static native int nativePollManualVisibility(long context,long dc);
    private static native boolean nativeCloseManualVisibility();
    // R99: separate finite source-pinned retained first-party preview.
    private static native boolean nativeArmRetainedPreview();
    private static native boolean nativeDrawRetainedPreview(
        long context,long dc,int width,int height);
    private static native boolean nativeRetainedPreviewAction(int action);
    private static native int[] nativeRetainedPreviewSnapshot();
    // Owner-thread, pinned-WGL, read-only actual native configuration.
    // Never used to execute or enable any Minecraft gameplay action.
    private static native String nativeReadRetainedProfile(
        long context,long dc,int width,int height);
    private static native boolean nativeDiscardRetainedPreview();
    private static native boolean nativeOpenOperatorMenu();
    private static native int nativePollOperatorMenu(long context,long dc);
    private static native boolean nativeCloseOperatorMenu();

    private Thread owner;
    private int attempts;
    private int current;
    private int absent;
    private int invalid;
    private int changed;
    private long context;
    private long dc;
    private int width;
    private int height;
    private boolean pinned;
    private boolean poisoned;
    private final boolean overlayRequested;
    private final boolean multiFrame;
    private final boolean scriptedVisibility;
    private final boolean manualVisibility;
    private final boolean retainedPreview;
    private final boolean operatorMenu;
    private final boolean windowAudit;
    private int auditForeground;
    private int auditBackground;
    private int auditRefused;
    private int auditPointerInside,auditPointerOutside,auditPointerPressed;
    private int auditPointerRefused;
    // R173: source-pinned read-only reflection of ave.A().m from the
    // original game's admitted render thread; never sends GUI input.
    private OriginalGuiScreenProbe guiScreenProbe;
    private int auditGuiPresent,auditGuiAbsent,auditGuiRefused;
    private final int maxSamples;
    private boolean overlayDrawn;
    private boolean overlayFailed;
    private int overlayFrames;
    private int hiddenFrames;
    private int toggles;
    private int keyToggles;
    private int eventToggles;
    private boolean menuVisible;
    private boolean operatorStop;
    private boolean manualClosed;
    private boolean retainedVerified;
    private int acceptedOperatorEvents;
    private int rejectedOperatorEvents;
    private int lastOperatorTab=-1;
    private int lastOperatorFocus=-1;
    private int lastOperatorPreview=-1;

    private RendererWglEvidence(boolean requested,boolean multi,boolean scripted,
                                boolean manual,boolean retained,boolean operator,
                                boolean audit) {
        overlayRequested=requested;
        multiFrame=multi;
        scriptedVisibility=scripted;
        manualVisibility=manual;
        retainedPreview=retained;
        operatorMenu=operator;
        windowAudit=audit;
        maxSamples=(manual || operator)?2048:
                   ((multi || scripted || retained)?72:8);
    }

    // The binder is called ONLY after exact loaded original ave/bfk
    // preflight, while the finite R101 window-audit option is explicit.
    // This shape check is not itself source attestation or input ownership.
    public synchronized boolean bindOriginalGuiObserver(
            OriginalGuiScreenProbe probe) {
        if(!windowAudit || probe==null || guiScreenProbe!=null ||
           attempts!=0 || owner!=null)return false;
        guiScreenProbe=probe;
        return true;
    }

    // R158: narrow receipt schema. Do not parse arbitrary profile strings
    // into Java module actions or trust an unpinned render callback.
    private static boolean validProfileReceipt(String receipt,
                                               boolean autoBlock) {
        return receipt!=null && receipt.length()>80 &&
            receipt.length()<=4096 &&
            receipt.startsWith(
                "# SlinkyWinky standalone profile; no secrets or account data\nversion=1\n") &&
            receipt.contains("\nauto_clicker_enabled=") &&
            receipt.contains("\nsprint_enabled=") &&
            receipt.contains("\nbacktrack_ms=") &&
            receipt.contains("\nauto_block_enabled="+
                (autoBlock?"true":"false")+"\n") &&
            receipt.endsWith("\n");
    }

    // R172: Java8 pure format validator for read-only OS evidence.
    // A true result MUST NOT authorize click capture or GUI dispatch.
    static boolean validOriginalPointerReceipt(int[] point,int width,int height) {
        if(point==null || point.length!=4 ||
           width<320 || width>8192 || height<240 || height>8192 ||
           point[3]<0 || point[3]>31 || (point[2]!=0 && point[2]!=1) ||
           point[0]<-32768 || point[0]>32767 ||
           point[1]<-32768 || point[1]>32767)
            return false;
        final boolean inside=point[0]>=0 && point[1]>=0 &&
            point[0]<width && point[1]<height;
        return (point[2]==1)==inside;
    }

    /** R93: deterministic, input-free show/hide/show admission for 64 frames. */
    public static boolean scriptedVisibleAt(int sample) {
        return sample>=9 && sample<=72 && (sample<=28 || sample>=45);
    }
    public static boolean scriptedHiddenAt(int sample) {
        return sample>=29 && sample<=44;
    }

    /** Must be called only after R73 has admitted exact loaded original classes. */
    public static RendererWglEvidence open(String dllPath) throws Exception {
        return open(dllPath,false,false,false,false);
    }

    public static RendererWglEvidence open(String dllPath,boolean overlay) throws Exception {
        return open(dllPath,overlay,false,false,false);
    }

    public static RendererWglEvidence open(String dllPath,boolean overlay,
                                            boolean multi) throws Exception {
        return open(dllPath,overlay,multi,false,false);
    }

    public static RendererWglEvidence open(String dllPath,boolean overlay,
                                            boolean multi,boolean scripted) throws Exception {
        return open(dllPath,overlay,multi,scripted,false);
    }

    public static RendererWglEvidence open(String dllPath,boolean overlay,
                                            boolean multi,boolean scripted,
                                            boolean manual) throws Exception {
        return open(dllPath,overlay,multi,scripted,manual,false);
    }

    public static RendererWglEvidence open(String dllPath,boolean overlay,
                                            boolean multi,boolean scripted,
                                            boolean manual,boolean retained) throws Exception {
        return open(dllPath,overlay,multi,scripted,manual,retained,false);
    }
    public static RendererWglEvidence open(String dllPath,boolean overlay,
                                            boolean multi,boolean scripted,
                                            boolean manual,boolean retained,
                                            boolean operator) throws Exception {
        return open(dllPath,overlay,multi,scripted,manual,retained,operator,false);
    }
    public static RendererWglEvidence open(String dllPath,boolean overlay,
                                            boolean multi,boolean scripted,
                                            boolean manual,boolean retained,
                                            boolean operator,boolean audit) throws Exception {
        if(audit && (overlay || multi || scripted || manual || retained || operator))
            throw new IllegalArgumentException("R101 source-only window audit is exclusive and read-only");
        if(operator && (!overlay || multi || scripted || manual || retained))
            throw new IllegalArgumentException("R100 operator menu requires separate original-only consent");
        if(retained && (!overlay || multi || scripted || manual))
            throw new IllegalArgumentException("R99 finite retained preview is exclusive");
        if(manual && (multi || scripted || !overlay))
            throw new IllegalArgumentException("R94 manual visibility requires exclusive overlay mode");
        if(scripted && (multi || !overlay))
            throw new IllegalArgumentException("R93 scripted show/hide requires exclusive overlay mode");
        if(multi && !overlay)
            throw new IllegalArgumentException("R92 requires overlay admission");
        if(dllPath==null || !dllPath.endsWith("slinkywinky_jni.dll"))
            throw new IllegalArgumentException("R78 exact JNI DLL required");
        File dll=new File(dllPath).getCanonicalFile();
        if(!dll.isAbsolute() || !dll.isFile() ||
           !"slinkywinky_jni.dll".equalsIgnoreCase(dll.getName()))
            throw new IllegalArgumentException("R78 JNI DLL missing");
        System.load(dll.getAbsolutePath());
        if(manual && !nativeOpenManualVisibility())
            throw new IllegalStateException("R94 owner-PID control gate admission failed");
        if(retained && !nativeArmRetainedPreview())
            throw new IllegalStateException("R99 owner-render retained admission failed");
        if(operator && !nativeOpenOperatorMenu())
            throw new IllegalStateException("R100 PID/birth operator-menu admission failed");
        return new RendererWglEvidence(overlay,multi,scripted,manual,retained,operator,audit);
    }

    /**
     * The caller is responsible for confirming the original renderer
     * callsite before observation. Passive sampling is read-only; optional
     * bounded drawing uses separately verified state-restoring native JNI.
     */
    public synchronized void sample() {
        if(poisoned || attempts>=maxSamples)return;
        attempts++;
        if(owner==null)owner=Thread.currentThread();
        else if(Thread.currentThread()!=owner) {
            poisoned=true;
            invalid++;
            return;
        }
        try {
            final long[] v=nativeCurrentWglSnapshot();
            if(v==null || v.length!=6) {
                poisoned=true;
                invalid++;
                return;
            }
            if(v[0]==0L || v[1]==0L) {
                absent++;
                return;
            }
            if(v[2]!=0L || v[3]!=0L ||
               v[4]<320L || v[4]>8192L || v[5]<240L || v[5]>8192L) {
                invalid++;
                return;
            }
            if(!pinned) {
                pinned=true;
                context=v[0];
                dc=v[1];
                width=(int)v[4];
                height=(int)v[5];
            } else if(context!=v[0] || dc!=v[1] ||
                      width!=(int)v[4] || height!=(int)v[5]) {
                changed++;
            } else {
                // Same opaque handles and same exact viewport.
            }
            current++;
            if(windowAudit) {
                // The window is identified from THIS original renderer's
                // WGL HDC; no global hook, focus change or gameplay input.
                final int identity=nativeAuditRenderWindow(context,dc,width,height);
                if(identity>=0 && identity<=1) {
                    final int observed=guiScreenProbe==null?
                        OriginalGuiScreenProbe.REFUSED:guiScreenProbe.observe();
                    if(observed==OriginalGuiScreenProbe.SCREEN_PRESENT)
                        auditGuiPresent++;
                    else if(observed==OriginalGuiScreenProbe.NO_SCREEN)
                        auditGuiAbsent++;
                    else {
                        auditGuiRefused++;poisoned=true;invalid++;return;
                    }
                }
                if(identity==1) {
                    auditForeground++;
                    // Read ONLY the foreground original HWND and actual WGL
                    // render owner. Do not synthesize mouse down/up, hook
                    // LWJGL, or pass these coordinates to a menu dispatcher.
                    final int[] point=nativeObservePhysicalPointer(
                        context,dc,width,height);
                    if(!validOriginalPointerReceipt(point,width,height)) {
                        auditPointerRefused++;poisoned=true;invalid++;return;
                    }
                    if(point[2]==1)auditPointerInside++;
                    else auditPointerOutside++;
                    if(point[3]!=0)auditPointerPressed++;
                } else if(identity==0)auditBackground++;
                else {auditRefused++;poisoned=true;invalid++;}
                return;
            }
            if(manualVisibility) {
                if(attempts>8 && preflightStable()) {
                    final int action=nativePollManualVisibility(context,dc);
                    if(action<0 || action>3) {
                        poisoned=true;
                        invalid++;
                        return;
                    }
                    if(action==2) {
                        // The operator stop is consumed on this verified
                        // game-render thread. No UI draw occurs on stop.
                        operatorStop=true;
                        hiddenFrames++;
                        return;
                    }
                    if(action==1 || action==3) {
                        menuVisible=!menuVisible;
                        toggles++;
                        if(action==3)keyToggles++;
                        else eventToggles++;
                    }
                    if(menuVisible) {
                        if(!nativeDrawCompactOverlay(context,dc,width,height)) {
                            poisoned=true;
                            overlayFailed=true;
                            invalid++;
                        } else {
                            overlayDrawn=true;
                            overlayFrames++;
                        }
                    } else hiddenFrames++;
                }
                return;
            }
            // R100: actions originate ONLY from acknowledged external
            // PID/birth-scoped named events. The receiver polls on the
            // already verified source-pinned original WGL render owner.
            // No Minecraft keys, clicks, packets or gameplay module dispatch.
            if(operatorMenu && attempts>8 && preflightStable()) {
                if(!nativeDrawRetainedPreview(context,dc,width,height)) {
                    poisoned=true;overlayFailed=true;invalid++;return;
                }
                overlayDrawn=true;
                overlayFrames++;
                final int action=nativePollOperatorMenu(context,dc);
                if(action<0 || action>17 || action==18) {
                    poisoned=true;invalid++;return;
                }
                if(action==9)operatorStop=true;
                else if(action>=1 && action<=8)acceptedOperatorEvents++;
                else if(action>=10 && action<=17)rejectedOperatorEvents++;
                final int[] state=nativeRetainedPreviewSnapshot();
                if(state==null || state.length!=6 || state[0]!=overlayFrames ||
                   state[3]!=acceptedOperatorEvents ||
                   state[4]!=rejectedOperatorEvents ||
                   state[1]<0 || state[1]>5 || state[2]<-1 || state[2]>=65 ||
                   (state[5]!=0 && state[5]!=1) ||
                   acceptedOperatorEvents>15 || rejectedOperatorEvents>15) {
                    poisoned=true;invalid++;return;
                }
                if(action!=0 || overlayFrames==1) {
                    final String receipt=nativeReadRetainedProfile(
                        context,dc,width,height);
                    if(!validProfileReceipt(receipt,state[5]==1)) {
                        poisoned=true;invalid++;return;
                    }
                }
                lastOperatorTab=state[1];
                lastOperatorFocus=state[2];
                lastOperatorPreview=state[5];
                return;
            }
            // R99 is a separate finite retained menu preview, with four
            // internally scripted *local profile-only* navigation actions.
            // NO OS keys, mouse or original Minecraft gameplay handlers.
            if(retainedPreview && attempts>8 && preflightStable()) {
                if(!nativeDrawRetainedPreview(context,dc,width,height)) {
                    poisoned=true;overlayFailed=true;invalid++;return;
                }
                overlayDrawn=true;
                overlayFrames++;
                final int nextAction=overlayFrames==1?2:
                    (overlayFrames==2?4:(overlayFrames==3?5:
                     (overlayFrames==4?0:-1)));
                if(nextAction>=0 && !nativeRetainedPreviewAction(nextAction)) {
                    poisoned=true;overlayFailed=true;invalid++;return;
                }
                final int[] state=nativeRetainedPreviewSnapshot();
                if(state==null || state.length!=6 || state[0]!=overlayFrames ||
                   state[3]!=Math.min(overlayFrames,4) || state[4]!=0 ||
                   state[1]!=(overlayFrames>=4?1:0) ||
                   state[2]!=(overlayFrames>=4?17:(overlayFrames>=1?1:0)) ||
                   state[5]!=(overlayFrames>=3?1:0)) {
                    poisoned=true;overlayFailed=true;invalid++;return;
                }
                if(nextAction>=0 || overlayFrames==5) {
                    final String receipt=nativeReadRetainedProfile(
                        context,dc,width,height);
                    if(!validProfileReceipt(receipt,state[5]==1)) {
                        poisoned=true;invalid++;return;
                    }
                }
                if(overlayFrames>=5)retainedVerified=true;
                return;
            }
            // The visual R91 experiment is a distinct explicit mode. Exactly
            // ONE draw may occur, after all eight source/callsite/owner/WGL
            // snapshots have passed the established passive R78 gate.
            if(overlayRequested && !retainedPreview && !operatorMenu &&
               ((multiFrame && attempts>8 && preflightStable()) ||
                (scriptedVisibility && attempts>8 && preflightStable()) ||
                (!multiFrame && !scriptedVisibility &&
                 attempts==8 && preflightStable()))) {
                if(scriptedVisibility && scriptedHiddenAt(attempts)) {
                    // HIDDEN means no native draw call, no GUI input hook,
                    // no state writes or owner changes. The original game
                    // renders its own frame without any Slinky overlay.
                    hiddenFrames++;
                } else {
                    boolean drawn=false;
                    try {
                        drawn=scriptedVisibility?
                            nativeDrawCompactOverlay(context,dc,width,height):
                            nativeDrawReadOnlyOverlay(context,dc,width,height);
                    } catch(Throwable drawing) {
                        drawn=false;
                    }
                    if(!drawn) {
                        overlayFailed=true;
                        poisoned=true;
                        invalid++;
                    } else {
                        overlayDrawn=true;
                        overlayFrames++;
                    }
                }
            }
        } catch(Throwable error) {
            poisoned=true;
            invalid++;
        }
    }

    public synchronized boolean overlayRequested(){return overlayRequested;}
    public synchronized boolean multiFrame(){return multiFrame;}
    public synchronized boolean scriptedVisibility(){return scriptedVisibility;}
    public synchronized boolean manualVisibility(){return manualVisibility;}
    public synchronized boolean retainedPreview(){return retainedPreview;}
    public synchronized boolean operatorMenu(){return operatorMenu;}
    public synchronized boolean windowAudit(){return windowAudit;}
    public synchronized boolean windowIdentityVerified(){
        return windowAudit && attempts==8 && current==8 &&
               auditForeground+auditBackground==8 && auditRefused==0 &&
               auditPointerRefused==0 &&
               auditPointerInside+auditPointerOutside==auditForeground &&
               auditGuiPresent+auditGuiAbsent==8 && auditGuiRefused==0 &&
               !poisoned && fullyStable();
    }
    // No raw coordinates or button codes enter logs or public GUI state.
    // These counters cannot establish exclusive Minecraft input ownership.
    public synchronized int observedForegroundPointerSamples(){
        return windowAudit?auditPointerInside+auditPointerOutside:0;
    }
    public synchronized int refusedForegroundPointerSamples(){
        return windowAudit?auditPointerRefused:0;
    }
    public synchronized boolean originalGuiReadOnlyVerified(){
        return windowIdentityVerified() &&
               auditGuiPresent+auditGuiAbsent==8 && auditGuiRefused==0;
    }
    public synchronized int observedOriginalGuiPresent(){
        return windowAudit?auditGuiPresent:0;
    }
    public synchronized int observedOriginalGuiAbsent(){
        return windowAudit?auditGuiAbsent:0;
    }
    public synchronized int refusedOriginalGuiObservations(){
        return windowAudit?auditGuiRefused:0;
    }
    public synchronized boolean operatorStopped(){return operatorMenu && operatorStop;}
    public synchronized boolean operatorFailed(){return operatorMenu &&
        (poisoned || overlayFailed);}
    public synchronized boolean operatorVerified(){
        return operatorMenu && operatorStop && !poisoned && !overlayFailed &&
               overlayFrames>=5 && acceptedOperatorEvents==4 &&
               rejectedOperatorEvents==0 && lastOperatorTab==1 &&
               lastOperatorFocus==17 && lastOperatorPreview==1;
    }
    public synchronized boolean retainedVerified(){
        return retainedPreview && retainedVerified && overlayFrames==64 &&
               !poisoned && !overlayFailed;
    }
    public synchronized boolean manualStopped(){return operatorStop;}
    public synchronized boolean manualFailed(){return poisoned || overlayFailed;}
    public synchronized int toggles(){return toggles;}
    public synchronized int keyToggles(){return keyToggles;}
    public synchronized int eventToggles(){return eventToggles;}
    public synchronized boolean closeManual() {
        if(operatorMenu) {
            if(manualClosed)return false;
            manualClosed=true;
            return nativeCloseOperatorMenu();
        }
        if(retainedPreview) {
            if(manualClosed)return false;
            manualClosed=true;
            // Called only after the callback fence drains all WGL owners;
            // per-frame native WGL hosts are already closed on render.
            return nativeDiscardRetainedPreview();
        }
        if(!manualVisibility)return true;
        if(manualClosed)return false;
        manualClosed=true;
        return nativeCloseManualVisibility();
    }
    public synchronized int overlayFrames(){return overlayFrames;}
    public synchronized int hiddenFrames(){return hiddenFrames;}
    public synchronized boolean overlayDrawn(){
        return overlayDrawn && !overlayFailed &&
               (operatorMenu ? operatorVerified() :
                (retainedPreview ? retainedVerified() :
                (manualVisibility ?
                    (operatorStop && toggles>=2 && toggles==keyToggles+eventToggles &&
                     overlayFrames>0 && hiddenFrames>0 &&
                     overlayFrames+hiddenFrames==attempts-8) :
                    (overlayFrames==(scriptedVisibility?48:(multiFrame?64:1)) &&
                     (!scriptedVisibility || hiddenFrames==16)))));
    }
    public synchronized int attempts(){return attempts;}
    public synchronized int present(){return current;}
    public synchronized int absent(){return absent;}
    public synchronized int invalid(){return invalid;}
    public synchronized int changed(){return changed;}
    private boolean preflightStable() {
        return attempts>=8 && current==attempts && absent==0 &&
               invalid==0 && changed==0 && pinned && !poisoned;
    }
    public synchronized boolean fullyStable() {
        return ((manualVisibility || operatorMenu) ?
                (operatorStop || attempts==maxSamples) :
                attempts==maxSamples) && preflightStable();
    }
    public synchronized String result() {
        return "attempts="+attempts+" current="+current+" absent="+absent+
               " invalid="+invalid+" changed="+changed+
               " stable="+fullyStable()+
               (windowAudit ?
                " window=exact-original owner=render-thread foreground="+auditForeground+
                " background="+auditBackground+" refused="+auditRefused+
                " width="+width+" height="+height :
               (operatorMenu ?
                " shown="+overlayFrames+" actions="+acceptedOperatorEvents+
                " refused="+rejectedOperatorEvents+" tab="+lastOperatorTab+
                " focus="+lastOperatorFocus+" preview="+lastOperatorPreview+
                " stop="+(operatorStop?"operator":"limit") :
               (retainedPreview ?
                " frames="+overlayFrames+" retained="+retainedVerified() +
                " actions=4 tab=1 focus=17" :
               (manualVisibility ?
                " shown="+overlayFrames+" hidden="+hiddenFrames+
                " toggles="+toggles+" stop="+(operatorStop?"operator":"limit"):
                (scriptedVisibility?
                 " shown="+overlayFrames+" hidden="+hiddenFrames:
                 (multiFrame?" frames="+overlayFrames:""))))));
    }
}
