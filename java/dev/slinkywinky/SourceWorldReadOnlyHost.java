package dev.slinkywinky;

/**
 * R337: an actual client-thread observation owner for the four original-table
 * source modules. Port is implemented by Forge189; no game classes here.
 * All decisions stay read-only and have no packet, timer or keyboard sink.
 */
public final class SourceWorldReadOnlyHost implements AutoCloseable {
    public interface Port {
        boolean admittedClientThread();
        Object worldIdentity();
        Object playerIdentity();
        Frame capture();
    }
    /** Four simultaneous, copied source controller snapshots.
     * Ordered BACKTRACK_TICK, LAG_RANGE_TICK, TIMER_RANGE_TICK, INV_WALK_TICK. */
    public static final class Frame {
        private static final int[] BITS={12,12,15,12};
        private static final int[] FIELDS={7,6,7,1};
        final int[][] bits=new int[4][];
        final double[][] fields=new double[4][];
        public Frame(int[][] b,double[][] f){
            if(b==null||f==null||b.length!=4||f.length!=4)
                throw new IllegalArgumentException("Four source observations required");
            for(int i=0;i<4;++i){
                if(b[i]==null||f[i]==null||
                   b[i].length!=BITS[i]||f[i].length!=FIELDS[i])
                    throw new IllegalArgumentException("Unbounded source observation shape");
                bits[i]=b[i].clone();
                fields[i]=f[i].clone();
                for(double v:fields[i])
                    if(!Double.isFinite(v))
                        throw new IllegalArgumentException("Nonfinite world position or setting");
            }
        }
    }
    public static final class TickReceipt {
        public final long generation,tick;
        public final SourceRuntimePreview.Receipt backtrack,lagRange,timerRange,invWalk;
        private TickReceipt(long generation,long tick,SourceRuntimePreview.Receipt[] r){
            this.generation=generation;this.tick=tick;
            backtrack=r[0];lagRange=r[1];timerRange=r[2];invWalk=r[3];
        }
    }
    private final Thread thread=Thread.currentThread();
    private final SourceRuntimePreview nativePreview;
    private Object world,player;
    private long generation=0,tick=0,worldNumber=0,playerNumber=0,tokenSequence=100;
    private boolean closed,inCall;
    private TickReceipt last;
    public SourceWorldReadOnlyHost(){
        this(new SourceRuntimePreview());
    }
    public SourceWorldReadOnlyHost(SourceRuntimePreview nativePreview){
        if(nativePreview==null)throw new IllegalArgumentException("No source JNI owner");
        this.nativePreview=nativePreview;
    }
    private void owner(){
        if(thread!=Thread.currentThread()||closed)
            throw new IllegalStateException("Closed or foreign Minecraft source observer");
    }
    public long generation(){owner();return generation;}
    public TickReceipt last(){owner();return last;}
    private void invalidate(){
        last=null;
        world=player=null;tick=0;
        worldNumber=playerNumber=0;
    }
    public void retire(){
        owner();
        if(inCall)throw new IllegalStateException("Cannot retire during source delivery");
        invalidate();
    }
    private void failClosed(){
        try{nativePreview.close();}
        finally{closed=true;invalidate();}
    }
    /**
     * Source tick order is fixed. Forge's ClientTick.END does NOT establish
     * equivalence to the original native Slinky event slot 18/0/25.
     */
    public TickReceipt observe(Port port){
        owner();
        if(inCall)throw new IllegalStateException("Reentrant source observation");
        if(port==null)throw new IllegalArgumentException("Missing world Port");
        inCall=true;
        try{
            if(!port.admittedClientThread())
                throw new IllegalStateException("Not a local 1.8.9 client thread");
            final Object readWorld=port.worldIdentity();
            final Object readPlayer=port.playerIdentity();
            if(readWorld==null||readPlayer==null){
                invalidate();
                return null;
            }
            Frame frame=port.capture();
            if(frame==null||readWorld!=port.worldIdentity()||
               readPlayer!=port.playerIdentity()||!port.admittedClientThread())
                throw new IllegalStateException("World replaced during source snapshot");
            if(readWorld!=world||readPlayer!=player){
                if(generation==Long.MAX_VALUE||tokenSequence>Long.MAX_VALUE-2)
                    throw new IllegalStateException("Source generation exhausted");
                ++generation;
                world=readWorld;player=readPlayer;tick=0;
                worldNumber=++tokenSequence;playerNumber=++tokenSequence;
            }
            if(tick==Long.MAX_VALUE)
                throw new IllegalStateException("Source world tick exhausted");
            ++tick;
            final int[] names={
                SourceRuntimePreview.BACKTRACK_TICK,
                SourceRuntimePreview.LAG_RANGE_TICK,
                SourceRuntimePreview.TIMER_RANGE_TICK,
                SourceRuntimePreview.INV_WALK_TICK
            };
            SourceRuntimePreview.Receipt[] result=new SourceRuntimePreview.Receipt[4];
            for(int i=0;i<4;++i){
                result[i]=nativePreview.observe(worldNumber,playerNumber,
                    generation,tick,names[i],frame.bits[i],frame.fields[i]);
            }
            last=new TickReceipt(generation,tick,result);
            return last;
        }catch(RuntimeException | Error failure){
            failClosed();
            throw failure;
        }finally{
            inCall=false;
        }
    }
    @Override public void close(){
        owner();
        if(inCall)throw new IllegalStateException("Cannot close during source delivery");
        failClosed();
    }
}
