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
    private static native void nativeClose(long handle);
    private final Thread owner=Thread.currentThread();
    private long handle;
    public static final class Decision {
        public final boolean admitted,found,faceUp;
        public final int sourceIndex,blockX,blockY,blockZ,visited;
        public final double angularCost;
        private Decision(double[] raw) {
            if(raw==null || raw.length!=9)
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
               Math.abs(raw[6])>=1000000 || raw[8]<0 || raw[8]>1000)
                throw new IllegalStateException("Invalid native Clutch proposal");
            admitted=true;found=raw[1]==1;faceUp=raw[2]==1;
            sourceIndex=(int)raw[3];
            blockX=(int)raw[4];blockY=(int)raw[5];blockZ=(int)raw[6];
            visited=(int)raw[7];angularCost=raw[8];
            if(!found && (faceUp || sourceIndex!=0 || blockX!=0 ||
                blockY!=0 || blockZ!=0 || angularCost!=0))
                throw new IllegalStateException("Unexpected missing-block data");
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
    @Override public synchronized void close() {
        if(owner!=Thread.currentThread())
            throw new IllegalStateException("Foreign Clutch native close");
        if(handle==0)return;
        long old=handle;handle=0;nativeClose(old);
    }
}
