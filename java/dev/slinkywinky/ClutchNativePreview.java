package dev.slinkywinky;

/**
 * R239: one explicit owner-thread Java 8 -> native C++ Clutch planner call.
 * Read-only policy evaluation, never a placement/click/rotation/packet write.
 * The previous-block ray phase is deliberately NOT connected yet.
 */
public final class ClutchNativePreview implements AutoCloseable {
    static {System.loadLibrary("slinkywinky_jni");}
    private static native long nativeCreate();
    private static native double[] nativeEvaluate(long handle,long[] scope,
            double[] geometry,int[] candidates,int[] ground);
    private static native double[] nativePreparePrior(long handle,long[] scope,
            double[] geometry,int[] candidates,int[] ground,
            int[] previous,double[] previousAim);
    private static native double[] nativeFinishPrior(long handle,long tick,int[] hit);
    private static native void nativeClose(long handle);
    private final Thread owner=Thread.currentThread();
    private long handle;
    public static final class Decision {
        public final boolean admitted,found,faceUp;
        public final int sourceIndex,blockX,blockY,blockZ,visited;
        public final double angularCost;
        public final double aimX,aimY,aimZ,yawDelta,pitchDelta;
        // Preserved-prior remains unsupported until the two-phase ray gate.
        public final boolean preservedPrior;
        private Decision(double[] raw) {
            if(raw==null || raw.length!=15)
                throw new IllegalStateException("Malformed native Clutch receipt");
            for(double v:raw)if(!Double.isFinite(v))
                throw new IllegalStateException("Nonfinite native Clutch receipt");
            if(raw[0]!=1 || (raw[1]!=0 && raw[1]!=1) ||
               (raw[2]!=0 && raw[2]!=1) ||
               raw[3]<0 || raw[3]>=167 || raw[7]<0 || raw[7]>167 ||
               raw[3]!=Math.rint(raw[3]) || raw[7]!=Math.rint(raw[7]) ||
               raw[4]!=Math.rint(raw[4]) || raw[5]!=Math.rint(raw[5]) ||
               raw[6]!=Math.rint(raw[6]) ||
               Math.abs(raw[4])>=1000000 || Math.abs(raw[5])>=1000000 ||
               Math.abs(raw[6])>=1000000 || raw[8]<0 || raw[8]>1000 ||
               Math.abs(raw[9])>=1000000 || Math.abs(raw[10])>=1000000 ||
               Math.abs(raw[11])>=1000000 ||
               (raw[14]!=0 && raw[14]!=1) ||
               Math.abs(raw[12])>180.01 || Math.abs(raw[13])>500.01)
                throw new IllegalStateException("Invalid native Clutch proposal");
            admitted=true;found=raw[1]==1;faceUp=raw[2]==1;
            sourceIndex=(int)raw[3];
            blockX=(int)raw[4];blockY=(int)raw[5];blockZ=(int)raw[6];
            visited=(int)raw[7];angularCost=raw[8];
            aimX=raw[9];aimY=raw[10];aimZ=raw[11];
            yawDelta=raw[12];pitchDelta=raw[13];
            preservedPrior=raw[14]==1;
            if(preservedPrior && (!found || faceUp))
                throw new IllegalStateException("Invalid retained Clutch prior");
            if(!found && (faceUp || sourceIndex!=0 || blockX!=0 ||
                blockY!=0 || blockZ!=0 || angularCost!=0 ||
                aimX!=0 || aimY!=0 || aimZ!=0 ||
                yawDelta!=0 || pitchDelta!=0))
                throw new IllegalStateException("Unexpected missing-block data");
            if(found && (aimX<blockX-.001 || aimX>blockX+1.001 ||
                aimY<blockY-.001 || aimY>blockY+1.001 ||
                aimZ<blockZ-.001 || aimZ>blockZ+1.001 ||
                (faceUp && Math.abs(aimY-(blockY+1.0))>.001) ||
                Math.abs(Math.abs(yawDelta)+Math.abs(pitchDelta)-
                    angularCost)>.02))
                throw new IllegalStateException("Inconsistent native Clutch aim");
        }
    }
    public ClutchNativePreview() {
        handle=nativeCreate();
        if(handle<=0)throw new IllegalStateException("Native Clutch unavailable");
    }
    private void owner() {
        if(owner!=Thread.currentThread() || handle<=0)
            throw new IllegalStateException("Stale or foreign Clutch native owner");
    }
    public synchronized Decision observe(
            ClutchWorldSampler.Request request,
            ClutchWorldSampler.Snapshot snapshot,float yaw,float pitch,
            float maxAim,float randomisation,double clock) {
        owner();
        if(request==null || snapshot==null ||
           !snapshot.nativeMatches(request) ||
           snapshot.previousBlockId!=-1 ||
           !Float.isFinite(yaw) || !Float.isFinite(pitch) ||
           !Float.isFinite(maxAim) || !Float.isFinite(randomisation) ||
           !Double.isFinite(clock))
            throw new IllegalArgumentException("R239 requires source read-only snapshot without prior");
        final double[] lo=snapshot.nativeBoxLo(),hi=snapshot.nativeBoxHi(),
            eye=snapshot.nativeEye();
        if(lo==null || hi==null || eye==null)
            throw new IllegalArgumentException("Clutch geometry missing");
        final double[] geo={
            lo[0],lo[1],lo[2],hi[0],hi[1],hi[2],
            eye[0],eye[1],eye[2],
            yaw,pitch,request.minHeight,maxAim,randomisation,clock
        };
        // Native session lifetime is one ClutchWorldSampler identity.
        // Opaque tokens are constants within this session, not JVM addresses.
        final long[] scope={
            snapshot.generation,snapshot.tick,
            snapshot.nativeWorldToken(),snapshot.nativePlayerToken(),
            snapshot.baseX,snapshot.baseY,snapshot.baseZ,
            request.range,snapshot.groundBlocked?1:0,
            snapshot.previousBlockId
        };
        return new Decision(nativeEvaluate(handle,scope,geo,
            snapshot.candidateBlockIds(),snapshot.groundBlockIds()));
    }
    /**
     * R244: one client-thread Clutch prior transaction, source phase order.
     * JNI first computes an exact read-only prior target using a COPY of
     * source RNG. Only then may Java read the real Minecraft block ray.
     * The returned receipt never applies any gameplay action.
     */
    public synchronized Decision observePrior(
            ClutchWorldSampler sampler,ClutchWorldSampler.Port port,
            ClutchWorldSampler.Request request,
            ClutchWorldSampler.Snapshot snapshot,double[] previousAim,
            float yaw,float pitch,float maxAim,float randomisation,
            double clock) {
        owner();
        if(sampler==null || port==null || request==null ||
           snapshot==null || !snapshot.nativeMatchesWithPrior(request) ||
           previousAim==null || previousAim.length!=3 ||
           !Float.isFinite(yaw) || !Float.isFinite(pitch) ||
           !Float.isFinite(maxAim) || !Float.isFinite(randomisation) ||
           !Double.isFinite(clock))
            throw new IllegalArgumentException("Invalid prior Clutch observation");
        for(double v:previousAim)
            if(!Double.isFinite(v) || Math.abs(v)>=1000000)
                throw new IllegalArgumentException("Invalid prior Clutch aim");
        final double[] lo=snapshot.nativeBoxLo(),hi=snapshot.nativeBoxHi(),
            eye=snapshot.nativeEye();
        final double[] geometry={
            lo[0],lo[1],lo[2],hi[0],hi[1],hi[2],eye[0],eye[1],eye[2],
            yaw,pitch,request.minHeight,maxAim,randomisation,clock
        };
        final long[] scope={
            snapshot.generation,snapshot.tick,
            snapshot.nativeWorldToken(),snapshot.nativePlayerToken(),
            snapshot.baseX,snapshot.baseY,snapshot.baseZ,
            request.range,snapshot.groundBlocked?1:0,
            snapshot.previousBlockId
        };
        final int[] previous={
            request.previous.x,request.previous.y,request.previous.z
        };
        try {
            final double[] preflight=nativePreparePrior(handle,scope,
                geometry,snapshot.candidateBlockIds(),
                snapshot.groundBlockIds(),previous,previousAim.clone());
            if(preflight==null || preflight.length!=7 ||
               (preflight[0]!=0 && preflight[0]!=1))
                throw new IllegalStateException("Malformed native Clutch preflight");
            for(double v:preflight)
                if(!Double.isFinite(v) || Math.abs(v)>=1000000)
                    throw new IllegalStateException("Invalid Clutch ray geometry");
            int[] ray=null;
            if(preflight[0]==1) {
                if(snapshot.previousBlockId<=0 ||
                   preflight[1]!=eye[0] ||
                   preflight[2]!=eye[1] ||
                   preflight[3]!=eye[2])
                    throw new IllegalStateException("Unowned Clutch ray target");
                final double[] to={
                    preflight[4],preflight[5],preflight[6]
                };
                final ClutchWorldSampler.RayReceipt result=
                    sampler.priorRay(port,snapshot,to);
                if(result.tick!=snapshot.tick)
                    throw new IllegalStateException("Stale Clutch ray tick");
                ray=result.hit==null?new int[0]:
                    new int[]{result.hit.x,result.hit.y,result.hit.z};
            } else {
                for(int i=1;i<7;++i)
                    if(preflight[i]!=0)
                        throw new IllegalStateException(
                            "Unsolicited Clutch ray proposal");
            }
            return new Decision(nativeFinishPrior(handle,snapshot.tick,ray));
        } catch(RuntimeException failure) {
            // A failed phase-two ray must never be resumed with a newer
            // world snapshot. Quarantine this native owner immediately.
            close();
            throw failure;
        }
    }
    @Override public synchronized void close() {
        if(owner!=Thread.currentThread())
            throw new IllegalStateException("Foreign Clutch native close");
        if(handle==0)return;
        long old=handle;handle=0;nativeClose(old);
    }
}
