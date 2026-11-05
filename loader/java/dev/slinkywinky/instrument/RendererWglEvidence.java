package dev.slinkywinky.instrument;

import java.io.File;

/**
 * R78: finite passive WGL evidence, never a graphics renderer.
 * Native call may ONLY read current WGL handles and GL_VIEWPORT.
 * No pointer values are emitted to logs or exposed as gameplay controls.
 */
public final class RendererWglEvidence {
    private static native long[] nativeCurrentWglSnapshot();
    private static native boolean nativeDrawReadOnlyOverlay(
        long context,long dc,int width,int height);

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
    private boolean overlayDrawn;
    private boolean overlayFailed;

    private RendererWglEvidence(boolean requested) {overlayRequested=requested;}

    /** Must be called only after R73 has admitted exact loaded original classes. */
    public static RendererWglEvidence open(String dllPath) throws Exception {
        return open(dllPath,false);
    }

    public static RendererWglEvidence open(String dllPath,boolean overlay) throws Exception {
        if(dllPath==null || !dllPath.endsWith("slinkywinky_jni.dll"))
            throw new IllegalArgumentException("R78 exact JNI DLL required");
        File dll=new File(dllPath).getCanonicalFile();
        if(!dll.isAbsolute() || !dll.isFile() ||
           !"slinkywinky_jni.dll".equalsIgnoreCase(dll.getName()))
            throw new IllegalArgumentException("R78 JNI DLL missing");
        System.load(dll.getAbsolutePath());
        return new RendererWglEvidence(overlay);
    }

    /**
     * The caller is responsible for confirming the original renderer
     * callsite before observation. Never draws or mutates WGL/GL state.
     */
    public synchronized void sample() {
        if(poisoned || attempts>=8)return;
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
            // The visual R91 experiment is a distinct explicit mode. Exactly
            // ONE draw may occur, after all eight source/callsite/owner/WGL
            // snapshots have passed the established passive R78 gate.
            if(overlayRequested && attempts==8 && fullyStable()) {
                try {
                    overlayDrawn=nativeDrawReadOnlyOverlay(
                        context,dc,width,height);
                } catch(Throwable drawing) {
                    overlayDrawn=false;
                }
                if(!overlayDrawn) {
                    overlayFailed=true;
                    poisoned=true;
                    invalid++;
                }
            }
        } catch(Throwable error) {
            poisoned=true;
            invalid++;
        }
    }

    public synchronized boolean overlayRequested(){return overlayRequested;}
    public synchronized boolean overlayDrawn(){return overlayDrawn && !overlayFailed;}
    public synchronized int attempts(){return attempts;}
    public synchronized int present(){return current;}
    public synchronized int absent(){return absent;}
    public synchronized int invalid(){return invalid;}
    public synchronized int changed(){return changed;}
    public synchronized boolean fullyStable() {
        return attempts==8 && current==8 && absent==0 &&
               invalid==0 && changed==0 && pinned && !poisoned;
    }
    public synchronized String result() {
        return "attempts="+attempts+" current="+current+" absent="+absent+
               " invalid="+invalid+" changed="+changed+
               " stable="+fullyStable();
    }
}
