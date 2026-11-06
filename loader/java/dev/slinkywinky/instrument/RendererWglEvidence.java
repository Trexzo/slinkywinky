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
    private static native boolean nativeDrawReadOnlyOverlay(
        long context,long dc,int width,int height);
    private static native boolean nativeDrawCompactOverlay(
        long context,long dc,int width,int height);
    private static native boolean nativeOpenManualVisibility();
    private static native int nativePollManualVisibility(long context,long dc);
    private static native boolean nativeCloseManualVisibility();

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
    private final int maxSamples;
    private boolean overlayDrawn;
    private boolean overlayFailed;
    private int overlayFrames;
    private int hiddenFrames;
    private int toggles;
    private boolean menuVisible;
    private boolean operatorStop;
    private boolean manualClosed;

    private RendererWglEvidence(boolean requested,boolean multi,boolean scripted,
                                boolean manual) {
        overlayRequested=requested;
        multiFrame=multi;
        scriptedVisibility=scripted;
        manualVisibility=manual;
        maxSamples=manual?2048:((multi || scripted)?72:8);
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
        return new RendererWglEvidence(overlay,multi,scripted,manual);
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
            if(manualVisibility) {
                if(attempts>8 && preflightStable()) {
                    final int action=nativePollManualVisibility(context,dc);
                    if(action<0 || action>2) {
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
                    if(action==1) {
                        menuVisible=!menuVisible;
                        toggles++;
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
            // The visual R91 experiment is a distinct explicit mode. Exactly
            // ONE draw may occur, after all eight source/callsite/owner/WGL
            // snapshots have passed the established passive R78 gate.
            if(overlayRequested &&
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
    public synchronized boolean manualStopped(){return operatorStop;}
    public synchronized boolean manualFailed(){return poisoned || overlayFailed;}
    public synchronized int toggles(){return toggles;}
    public synchronized boolean closeManual() {
        if(!manualVisibility)return true;
        if(manualClosed)return false;
        manualClosed=true;
        return nativeCloseManualVisibility();
    }
    public synchronized int overlayFrames(){return overlayFrames;}
    public synchronized int hiddenFrames(){return hiddenFrames;}
    public synchronized boolean overlayDrawn(){
        return overlayDrawn && !overlayFailed &&
               (manualVisibility ?
                    (operatorStop && toggles>=2 && overlayFrames>0 && hiddenFrames>0 &&
                     overlayFrames+hiddenFrames==attempts-8) :
                    (overlayFrames==(scriptedVisibility?48:(multiFrame?64:1)) &&
                     (!scriptedVisibility || hiddenFrames==16)));
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
        return (manualVisibility ? (operatorStop || attempts==maxSamples) :
                attempts==maxSamples) && preflightStable();
    }
    public synchronized String result() {
        return "attempts="+attempts+" current="+current+" absent="+absent+
               " invalid="+invalid+" changed="+changed+
               " stable="+fullyStable()+
               (manualVisibility ?
                " shown="+overlayFrames+" hidden="+hiddenFrames+
                " toggles="+toggles+" stop="+(operatorStop?"operator":"limit"):
                (scriptedVisibility?
                 " shown="+overlayFrames+" hidden="+hiddenFrames:
                 (multiFrame?" frames="+overlayFrames:"")));
    }
}
