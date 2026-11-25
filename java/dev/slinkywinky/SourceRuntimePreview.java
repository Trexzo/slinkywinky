package dev.slinkywinky;

/** R335: Java 8 owner-thread-only JNI view of four candidate native modules.
 * This class observes read-only results and performs NO Minecraft actions. */
public final class SourceRuntimePreview implements AutoCloseable {
    static {System.loadLibrary("slinkywinky_jni");}
    private static native long nativeOpen();
    private static native double[] nativeObserve(long handle,long[] identity,
                                                  int opcode,int[] bits,double[] fields);
    private static native void nativeClose(long handle);
    private static native long[] nativeControl(long handle,long[] identity,
                                               int moduleId,boolean enabled);
    public static final int BACKTRACK=0,LAG_RANGE=1,TIMER_RANGE=2,INV_WALK=3;
    /** One-shot, read-only native disable cleanup. No world mutation. */
    public static final class ControlReceipt {
        public final int moduleId;
        public final boolean requestedEnabled,activeEnabled;
        public final boolean restoreTimerSpeed,releaseUseKey;
        public final long queuedInventoryClicks,worldGeneration;
        private ControlReceipt(int module,boolean enabled,long generation,long[] r){
            if(r==null||r.length!=7||r[0]!=module||
               r[1]!=(enabled?1:0)||r[2]!=(enabled?1:0)||
               r[3]<0||r[3]>1||r[4]<0||r[4]>1||
               r[5]<0||r[5]>32||r[6]!=generation)
                throw new IllegalStateException("Malformed source toggle receipt");
            moduleId=module;requestedEnabled=enabled;activeEnabled=r[2]!=0;
            restoreTimerSpeed=r[3]!=0;releaseUseKey=r[4]!=0;
            queuedInventoryClicks=r[5];worldGeneration=r[6];
        }
    }
    public synchronized ControlReceipt setEnabled(long world,long player,
                                                   long generation,long tick,
                                                   int module,boolean enabled){
        assertOwner();
        if(world<=0||player<=0||world==player||generation<=0||tick<0||
           module<0||module>3)
            throw new IllegalArgumentException("Invalid source toggle identity");
        long[] identity={world,player,generation,tick};
        return new ControlReceipt(module,enabled,generation,
            nativeControl(handle,identity,module,enabled));
    }
    public static final int BACKTRACK_TICK=0,BACKTRACK_PACKET=1,
        LAG_RANGE_TICK=2,LAG_RANGE_PACKET=3,TIMER_RANGE_TICK=4,
        TIMER_RANGE_FRAME=5,INV_WALK_TICK=6,INV_WALK_PACKET=7,
        INV_WALK_FLUSH=8;
    private static final int[] BIT_LENGTHS={12,4,12,4,15,1,12,1,2};
    private static final int[] FIELD_LENGTHS={7,1,6,4,7,20,1,0,0};
    private static final int[] SOURCE_EVENTS={18,2,18,3,0,3,25,23,17};
    private final Thread owner=Thread.currentThread();
    private long handle;
    public SourceRuntimePreview(){
        handle=nativeOpen();
        if(handle<=0)throw new IllegalStateException("Native source preview unavailable");
    }
    public static final class Receipt {
        public final long world,player,generation,tick;
        public final int opcode,sourceEvent;
        private final double[] values;
        private Receipt(long[] identity,int op,double[] nativeResult){
            world=identity[0];player=identity[1];
            generation=identity[2];tick=identity[3];
            opcode=op;sourceEvent=SOURCE_EVENTS[op];
            if(nativeResult==null||nativeResult.length!=8||
               nativeResult[0]!=1||nativeResult[1]!=sourceEvent||
               nativeResult[2]!=1)
                throw new IllegalStateException("Malformed source JNI receipt");
            for(double v:nativeResult)
                if(Double.isNaN(v)||Double.isInfinite(v))
                    throw new IllegalStateException("Nonfinite native source receipt");
            values=nativeResult.clone();
        }
        public double at(int index){return values[index];}
        public double[] copy(){return values.clone();}
    }
    private void assertOwner(){
        if(handle<=0||Thread.currentThread()!=owner)
            throw new IllegalStateException("Closed or foreign source preview");
    }
    public synchronized Receipt observe(long world,long player,long generation,long tick,
                                        int opcode,int[] flags,double[] fields){
        assertOwner();
        if(world<=0||player<=0||world==player||generation<=0||tick<0||
           opcode<0||opcode>=BIT_LENGTHS.length||
           flags==null||fields==null||
           flags.length!=BIT_LENGTHS[opcode]||
           fields.length!=FIELD_LENGTHS[opcode])
            throw new IllegalArgumentException("Malformed source observation dimensions");
        for(double value:fields)
            if(Double.isNaN(value)||Double.isInfinite(value))
                throw new IllegalArgumentException("Nonfinite source observation");
        long[] identity={world,player,generation,tick};
        return new Receipt(identity,opcode,
            nativeObserve(handle,identity,opcode,flags.clone(),fields.clone()));
    }
    @Override public synchronized void close(){
        assertOwner();
        nativeClose(handle);
        handle=0;
    }
}
