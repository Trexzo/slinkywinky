package dev.slinkywinky;

/** R335: Java 8 owner-thread-only JNI view of four candidate native modules.
 * This class observes read-only results and performs NO Minecraft actions. */
public final class SourceRuntimePreview implements AutoCloseable {
    // R359: eager static loading prevented a verified original Java8 Attach
    // agent from using the DLL it already loaded through System.load(path).
    // Only the explicit admitted agent path may bypass default library loading.
    private static boolean nativeReady,externallyPreloaded;
    public static synchronized void admitPreloadedLibrary(){
        if(nativeReady&&!externallyPreloaded)
            throw new IllegalStateException("R359 native library already owns a loader");
        externallyPreloaded=true;
    }
    private static synchronized void ensureNativeReady(){
        if(!nativeReady){
            if(!externallyPreloaded)System.loadLibrary("slinkywinky_jni");
            nativeReady=true;
        }
    }
    private static native long nativeOpen();
    private static native double[] nativeObserve(long handle,long[] identity,
                                                  int opcode,int[] bits,double[] fields);
    private static native void nativeClose(long handle);
    private static native double[] nativeSourceBoxes(
        long handle,long[] identity,double[] coordinates);
    /** Two original source event6 callbacks; read-only proposals. */
    public static final class SourceBox {
        public final boolean draw,rotate,filled;
        public final double x,y,z,scaleX,scaleY,scaleZ,rotationY,lineWidth;
        public final double red,green,blue,fillAlpha;
        private SourceBox(double[] row,int offset){
            if(row==null||row.length!=30||offset!=0&&offset!=15)
                throw new IllegalStateException("Malformed e6 box receipt");
            for(int i=0;i<15;++i)
                if(!Double.isFinite(row[offset+i]))
                    throw new IllegalStateException("Nonfinite e6 source geometry");
            if((row[offset]!=0&&row[offset]!=1)||
               (row[offset+7]!=0&&row[offset+7]!=1)||
               (row[offset+9]!=0&&row[offset+9]!=1)||
               row[offset+4]<=0||row[offset+5]<=0||row[offset+6]<=0||
               row[offset+10]<=0||row[offset+10]>64)
                throw new IllegalStateException("Invalid e6 geometry flags or dimensions");
            draw=row[offset]!=0;rotate=row[offset+7]!=0;filled=row[offset+9]!=0;
            x=row[offset+1];y=row[offset+2];z=row[offset+3];
            scaleX=row[offset+4];scaleY=row[offset+5];scaleZ=row[offset+6];
            rotationY=row[offset+8];lineWidth=row[offset+10];
            red=row[offset+11];green=row[offset+12];blue=row[offset+13];
            fillAlpha=row[offset+14];
            if(red<0||red>1||green<0||green>1||blue<0||blue>1||
               fillAlpha<0||fillAlpha>1)
                throw new IllegalStateException("Invalid e6 color/alpha");
        }
    }
    public static final class SourceBoxes {
        public final long generation,tick;
        public final SourceBox backtrack,lagRange;
        private SourceBoxes(long generation,long tick,double[] rows){
            this.generation=generation;this.tick=tick;
            backtrack=new SourceBox(rows,0);
            lagRange=new SourceBox(rows,15);
        }
    }
    public synchronized SourceBoxes observeBoxes(long world,long player,
                                  long generation,long tick,double[] coordinates){
        assertOwner();
        if(world<=0||player<=0||world==player||generation<=0||tick<0||
           coordinates==null||coordinates.length!=12)
            throw new IllegalArgumentException("Malformed source e6 world/box identity");
        for(double d:coordinates)
            if(!Double.isFinite(d)||Math.abs(d)>10000000)
                throw new IllegalArgumentException("Nonfinite source render field");
        if(coordinates[9]<0||coordinates[9]>1)
            throw new IllegalArgumentException("Invalid e6 partial tick");
        return new SourceBoxes(generation,tick,nativeSourceBoxes(handle,
            new long[]{world,player,generation,tick},coordinates.clone()));
    }

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
        INV_WALK_FLUSH=8,TIMER_RANGE_PACKET=9,LAG_RANGE_ATTACK=10;
    private static final int[] BIT_LENGTHS={12,4,12,4,15,1,12,1,2,0,2};
    private static final int[] FIELD_LENGTHS={7,1,6,4,7,20,1,0,0,0,0};
    private static final int[] SOURCE_EVENTS={18,2,18,3,0,3,25,23,17,1,15};
    private final Thread owner=Thread.currentThread();
    private long handle;
    public SourceRuntimePreview(){
        ensureNativeReady();
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
