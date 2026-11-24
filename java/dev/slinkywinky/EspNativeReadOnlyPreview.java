package dev.slinkywinky;

/**
 * R301: transport an admitted R300 world receipt to the actual first-party
 * R297/R298/R299 C++ controllers via JNI. Count-only verification; no GL,
 * Minecraft mutation, event-priority equivalence or live module switches.
 */
public final class EspNativeReadOnlyPreview implements AutoCloseable {
    private static final int VERSION=1;
    static {System.loadLibrary("slinkywinky_jni");}
    private static native long nativeCreate();
    private static native void nativeEnableSourcePhases(long handle);
    private static native long[] nativeSourceEvent(long handle,long[] identity,int type);
    private static native long[] nativeObserve(long handle,long[] identity,
        double[] self,double[] tiles,int[] tileIds,double[] drops,int[] dropFacts,
        double[] players,float[] health,int[] playerFacts,float[] ranges);
    private static native void nativeClose(long handle);
    private static native double[] nativeChestPlan(long handle,long[] identity,double[] camera);
    private static native double[] nativeEntityPlan(long handle,long[] identity,
                                                    double[] camera,double partialTicks);
    private static native double[] nativePlayerScreenRows(long handle,long[] identity,
                                                         double[] camera,double partialTicks);

    /** Source-derived native center translations. Diagnostic colors are host-specific. */
    public static final class ChestDraw {
        public final double centerX,centerY,centerZ;
        public final int blockId;
        public final boolean filled;
        private ChestDraw(double x,double y,double z,int id,boolean filled) {
            centerX=x;centerY=y;centerZ=z;blockId=id;this.filled=filled;
        }
    }
    public static final class ChestPlan {
        public final long worldToken,playerToken,generation,tick;
        public final float scale,filledAlpha,lineWidth;
        public final java.util.List<ChestDraw> draws;
        private ChestPlan(Result receipt,float scale,float alpha,float line,
                          java.util.List<ChestDraw> draws) {
            worldToken=receipt.worldToken;playerToken=receipt.playerToken;
            generation=receipt.generation;tick=receipt.tick;
            this.scale=scale;filledAlpha=alpha;lineWidth=line;
            this.draws=java.util.Collections.unmodifiableList(
                new java.util.ArrayList<ChestDraw>(draws));
        }
    }
    public static final class Result {
        public final long worldToken,playerToken,generation,tick;
        public final int rawTiles,rawDrops,rawPlayers,chestRows,itemRows,playerRows;
        private Result(EspWorldSnapshotSampler.Receipt receipt,long[] n){
            worldToken=receipt.worldToken;playerToken=receipt.playerToken;
            generation=receipt.generation;tick=receipt.tick;
            rawTiles=(int)n[5];rawDrops=(int)n[6];rawPlayers=(int)n[7];
            chestRows=(int)n[8];itemRows=(int)n[9];playerRows=(int)n[10];
        }
        private Result(Result previous,long[] n){
            worldToken=previous.worldToken;playerToken=previous.playerToken;
            generation=previous.generation;tick=previous.tick;
            rawTiles=previous.rawTiles;rawDrops=previous.rawDrops;
            rawPlayers=previous.rawPlayers;
            chestRows=(int)n[0];itemRows=(int)n[1];playerRows=(int)n[2];
        }
    }
    private final Thread thread=Thread.currentThread();
    private final boolean phased;
    private Result lastSource;
    private long handle;
    public EspNativeReadOnlyPreview(){this(false);}
    public EspNativeReadOnlyPreview(boolean sourcePhases){
        phased=sourcePhases;
        handle=nativeCreate();
        if(handle<=0)throw new IllegalStateException("R301 native unavailable");
        if(sourcePhases){
            try{nativeEnableSourcePhases(handle);}
            catch(RuntimeException | LinkageError failure){
                nativeClose(handle);handle=0;throw failure;
            }
        }
    }
    private void owner(){
        if(handle<=0||Thread.currentThread()!=thread)
            throw new IllegalStateException("R301 foreign/closed native observer");
    }
    /** Diagnostic-only ranges. Not assertions about user/original GUI settings. */
    public Result observe(EspWorldSnapshotSampler.Receipt receipt,
                          float chestRange,float itemRange,float playerRange){
        owner();
        if(receipt==null||receipt.ownerPosition==null||
           receipt.worldToken<=0||receipt.playerToken<=0||
           receipt.worldToken==receipt.playerToken||
           receipt.generation<=0||receipt.tick<0||
           receipt.tiles.size()>EspWorldSnapshotSampler.MAX_TILES||
           receipt.drops.size()>EspWorldSnapshotSampler.MAX_ENTITIES||
           receipt.players.size()>EspWorldSnapshotSampler.MAX_PLAYERS)
            throw new IllegalArgumentException("R301 malformed owner receipt");
        int ct=receipt.tiles.size(),cd=receipt.drops.size(),cp=receipt.players.size();
        long[] id={receipt.worldToken,receipt.playerToken,
                   receipt.generation,receipt.tick};
        double[] self={receipt.ownerPosition.x,receipt.ownerPosition.y,
                       receipt.ownerPosition.z};
        double[] tiles=new double[3*ct];int[] tileIds=new int[ct];
        for(int i=0;i<ct;i++){
            EspWorldSnapshotSampler.Tile t=receipt.tiles.get(i);
            tiles[3*i]=t.position.x;tiles[3*i+1]=t.position.y;
            tiles[3*i+2]=t.position.z;tileIds[i]=t.blockId;
        }
        double[] drops=new double[6*cd];int[] dropFacts=new int[5*cd];
        for(int i=0;i<cd;i++){
            EspWorldSnapshotSampler.Drop d=receipt.drops.get(i);
            drops[6*i]=d.previous.x;drops[6*i+1]=d.previous.y;
            drops[6*i+2]=d.previous.z;
            drops[6*i+3]=d.position.x;drops[6*i+4]=d.position.y;
            drops[6*i+5]=d.position.z;
            dropFacts[5*i]=d.itemId;dropFacts[5*i+1]=d.stackSize;
            dropFacts[5*i+2]=d.dead?1:0;
            dropFacts[5*i+3]=d.stackPresent?1:0;
            dropFacts[5*i+4]=d.itemPresent?1:0;
        }
        double[] players=new double[6*cp];float[] health=new float[3*cp];
        int[] playerFacts=new int[3*cp];
        for(int i=0;i<cp;i++){
            EspWorldSnapshotSampler.Player p=receipt.players.get(i);
            players[6*i]=p.previous.x;players[6*i+1]=p.previous.y;
            players[6*i+2]=p.previous.z;
            players[6*i+3]=p.position.x;players[6*i+4]=p.position.y;
            players[6*i+5]=p.position.z;
            health[3*i]=p.health;health[3*i+1]=p.maxHealth;
            health[3*i+2]=p.absorption;
            playerFacts[3*i]=p.self?1:0;playerFacts[3*i+1]=p.dead?1:0;
            playerFacts[3*i+2]=p.hurtTime;
        }
        long[] n=nativeObserve(handle,id,self,tiles,tileIds,drops,dropFacts,
                               players,health,playerFacts,
                               new float[]{chestRange,itemRange,playerRange});
        if(n==null||n.length!=11||n[0]!=VERSION||n[1]!=id[0]||
           n[2]!=id[1]||n[3]!=id[2]||n[4]!=id[3]||
           n[5]!=ct||n[6]!=cd||n[7]!=cp||
           (phased ? (n[8]!=-1||n[9]!=-1||n[10]!=-1) :
             (n[8]<0||n[8]>100||n[9]<0||n[9]>100||
              n[10]<0||n[10]>100)))
            throw new IllegalStateException("R301/R305 mismatched native receipt");
        Result result=new Result(receipt,n);
        if(phased)lastSource=result;
        return result;
    }
    /**
     * R305: independently dispatch source event-0 Chest collection,
     * source event-18 Item+Player collection, event-6 world-render admission,
     * event-7 screen-render admission, and event-26 unload cleanup.
     * No JNI call is made by legacy (unphased) clients.
     */
    public Result sourceEvent(Result previous,int event){
        owner();
        if(!phased||previous==null||previous!=lastSource||
           !(event==0||event==18||event==6||event==7||event==26))
            throw new IllegalStateException("R305 wrong source event/owner receipt");
        final long[] id={previous.worldToken,previous.playerToken,
                         previous.generation,previous.tick};
        long[] rows=nativeSourceEvent(handle,id,event);
        if(rows==null||rows.length!=3||rows[0]<0||rows[0]>100||
           ((event==0) ? (rows[1]!=-1||rows[2]!=-1) :
             (rows[1]<0||rows[1]>100||rows[2]<0||rows[2]>100))||
           (event==26&&(rows[0]!=0||rows[1]!=0||rows[2]!=0)))
            throw new IllegalStateException("R305 malformed phase receipt");
        Result next=new Result(previous,rows);
        lastSource=event==26?null:next;
        return next;
    }
    /**
     * R303: actual R297 C++ source controller's event-6 draw-plan.
     * Camera values are the Forge render-manager viewerPos fields, not
     * invented GL projections. JNI requires the EXACT most recent native
     * owner receipt; stale ticks/changed worlds cannot request draw rows.
     */
    public ChestPlan chestPlan(Result receipt,double cameraX,double cameraY,double cameraZ) {
        owner();
        if(receipt==null||receipt.worldToken<=0||receipt.playerToken<=0||
           receipt.generation<=0||receipt.tick<0||
           !Double.isFinite(cameraX)||!Double.isFinite(cameraY)||
           !Double.isFinite(cameraZ))
            throw new IllegalArgumentException("R303 malformed camera/receipt");
        final long[] id={receipt.worldToken,receipt.playerToken,receipt.generation,receipt.tick};
        final double[] encoded=nativeChestPlan(handle,id,new double[]{cameraX,cameraY,cameraZ});
        if(encoded==null||encoded.length<5||encoded[0]!=1.0||
           encoded[1]<0.0||encoded[1]>100.0||
           encoded[1]!=(int)encoded[1]||
           encoded.length!=5+5*(int)encoded[1]||
           (int)encoded[1]!=receipt.chestRows||
           encoded[2]!=0.5||Math.abs(encoded[3]-0.4)>0.000001||
           !Double.isFinite(encoded[4])||encoded[4]<=0||encoded[4]>16)
            throw new IllegalStateException("R303 invalid native source plan");
        java.util.List<ChestDraw> rows=new java.util.ArrayList<ChestDraw>();
        for(int i=0;i<(int)encoded[1];i++){
            int base=5+5*i;
            final double x=encoded[base],y=encoded[base+1],z=encoded[base+2];
            final double kind=encoded[base+3],filled=encoded[base+4];
            if(!Double.isFinite(x)||!Double.isFinite(y)||!Double.isFinite(z)||
               !(kind==54||kind==130||kind==146||kind==138)||
               !(filled==0.0||filled==1.0))
                throw new IllegalStateException("R303 malformed native row");
            rows.add(new ChestDraw(x,y,z,(int)kind,filled==1.0));
        }
        return new ChestPlan(receipt,(float)encoded[2],
                             (float)encoded[3],(float)encoded[4],rows);
    }


    /** R304: native ItemEspController collection order and merged quantity. */
    public static final class ItemDraw {
        public final double x,y,z;
        public final int itemId,quantity;
        private ItemDraw(double x,double y,double z,int id,int count){
            this.x=x;this.y=y;this.z=z;itemId=id;quantity=count;
        }
    }
    /** R304: native PlayerEspController farthest-first order and normalized health. */
    public static final class PlayerDraw {
        public final double x,y,z;
        public final float health,absorption;
        public final int hurtTime;
        private PlayerDraw(double x,double y,double z,float health,float absorption,int hurt){
            this.x=x;this.y=y;this.z=z;this.health=health;
            this.absorption=absorption;hurtTime=hurt;
        }
    }
    public static final class EntityPlan {
        public final long worldToken,playerToken,generation,tick;
        public final java.util.List<ItemDraw> items;
        public final java.util.List<PlayerDraw> players;
        private EntityPlan(Result receipt,java.util.List<ItemDraw> items,
                           java.util.List<PlayerDraw> players){
            worldToken=receipt.worldToken;playerToken=receipt.playerToken;
            generation=receipt.generation;tick=receipt.tick;
            this.items=java.util.Collections.unmodifiableList(
                new java.util.ArrayList<ItemDraw>(items));
            this.players=java.util.Collections.unmodifiableList(
                new java.util.ArrayList<PlayerDraw>(players));
        }
    }

    /**
     * R304: source controller results, not original render semantics.
     * All positions are interpolated from native previous/current rows, and
     * translated against the Forge render camera. Item quantity zero
     * represents a row folded into a nearby same-ID source count.
     */
    public EntityPlan entityPlan(Result receipt,double cameraX,double cameraY,
                                 double cameraZ,double partialTicks){
        owner();
        if(receipt==null||receipt.worldToken<=0||receipt.playerToken<=0||
           receipt.generation<=0||receipt.tick<0||
           !Double.isFinite(cameraX)||!Double.isFinite(cameraY)||
           !Double.isFinite(cameraZ)||!Double.isFinite(partialTicks)||
           partialTicks<0||partialTicks>1)
            throw new IllegalArgumentException("R304 malformed frame");
        long[] id={receipt.worldToken,receipt.playerToken,receipt.generation,receipt.tick};
        double[] out=nativeEntityPlan(handle,id,new double[]{cameraX,cameraY,cameraZ},
                                      partialTicks);
        if(out==null||out.length<3||out[0]!=1||out[1]<0||out[1]>100||
           out[2]<0||out[2]>100||out[1]!=(int)out[1]||
           out[2]!=(int)out[2]||out.length!=3+5*(int)out[1]+6*(int)out[2]||
           out[1]!=receipt.itemRows||out[2]!=receipt.playerRows)
            throw new IllegalStateException("R304 native row framing mismatch");
        java.util.List<ItemDraw> items=new java.util.ArrayList<ItemDraw>();
        for(int i=0;i<(int)out[1];i++){
            int k=3+5*i;
            if(!Double.isFinite(out[k])||!Double.isFinite(out[k+1])||
               !Double.isFinite(out[k+2])||out[k+3]<0||
               out[k+3]>(double)Integer.MAX_VALUE||
               out[k+3]!=(int)out[k+3]||out[k+4]<0||out[k+4]>99||
               out[k+4]!=(int)out[k+4])
                throw new IllegalStateException("R304 invalid item row");
            items.add(new ItemDraw(out[k],out[k+1],out[k+2],
                                   (int)out[k+3],(int)out[k+4]));
        }
        java.util.List<PlayerDraw> players=new java.util.ArrayList<PlayerDraw>();
        int offset=3+5*(int)out[1];
        for(int i=0;i<(int)out[2];i++){
            int k=offset+6*i;
            if(!Double.isFinite(out[k])||!Double.isFinite(out[k+1])||
               !Double.isFinite(out[k+2])||!Double.isFinite(out[k+3])||
               !Double.isFinite(out[k+4])||out[k+3]<0||out[k+3]>1||
               out[k+4]<0||out[k+4]>1||out[k+5]<0||
               out[k+5]>Integer.MAX_VALUE||out[k+5]!=(int)out[k+5])
                throw new IllegalStateException("R304 invalid player row");
            players.add(new PlayerDraw(out[k],out[k+1],out[k+2],
                       (float)out[k+3],(float)out[k+4],(int)out[k+5]));
        }
        return new EntityPlan(receipt,items,players);
    }

    /** R306: event-7 native PlayerESP screen rows in source farthest-first order. */
    public static final class ScreenPlayer {
        public final double x,y,z;
        public final float health,absorption;
        public final int hurtTime;
        private ScreenPlayer(double x,double y,double z,
                             float health,float absorption,int hurtTime){
            this.x=x;this.y=y;this.z=z;this.health=health;
            this.absorption=absorption;this.hurtTime=hurtTime;
        }
    }
    public static final class ScreenPlan {
        public final long worldToken,playerToken,generation,tick;
        public final java.util.List<ScreenPlayer> players;
        private ScreenPlan(Result source,java.util.List<ScreenPlayer> data){
            worldToken=source.worldToken;playerToken=source.playerToken;
            generation=source.generation;tick=source.tick;
            players=java.util.Collections.unmodifiableList(
                new java.util.ArrayList<ScreenPlayer>(data));
        }
    }
    /**
     * R306: only the original source event-7 phase can return native screen
     * rows. The native bridge still owns source sorting, normalized health,
     * relationship filtering and provenance. No coordinates are projected
     * inside JNI; actual Forge OpenGL matrices are supplied to a separate,
     * independently tested projection adapter.
     */
    public ScreenPlan playerScreenPlan(Result source,double cameraX,
                                       double cameraY,double cameraZ,double partialTicks){
        owner();
        if(!phased||source==null||source!=lastSource||
           source.playerRows<0||source.playerRows>100||
           !Double.isFinite(cameraX)||!Double.isFinite(cameraY)||
           !Double.isFinite(cameraZ)||!Double.isFinite(partialTicks)||
           partialTicks<0||partialTicks>1)
            throw new IllegalArgumentException("R306 invalid source screen request");
        long[] id={source.worldToken,source.playerToken,source.generation,source.tick};
        double[] out=nativePlayerScreenRows(handle,id,
            new double[]{cameraX,cameraY,cameraZ},partialTicks);
        if(out==null||out.length<2||out[0]!=1||out[1]<0||
           out[1]>100||out[1]!=(int)out[1]||
           out.length!=2+6*(int)out[1]||out[1]!=source.playerRows)
            throw new IllegalStateException("R306 source screen packet mismatch");
        java.util.List<ScreenPlayer> players=new java.util.ArrayList<ScreenPlayer>();
        for(int i=0;i<(int)out[1];i++){
            final int k=2+6*i;
            if(!Double.isFinite(out[k])||!Double.isFinite(out[k+1])||
               !Double.isFinite(out[k+2])||!Double.isFinite(out[k+3])||
               !Double.isFinite(out[k+4])||out[k+3]<0||out[k+3]>1||
               out[k+4]<0||out[k+4]>1||out[k+5]<0||
               out[k+5]>Integer.MAX_VALUE||out[k+5]!=(int)out[k+5])
                throw new IllegalStateException("R306 invalid native source screen row");
            players.add(new ScreenPlayer(out[k],out[k+1],out[k+2],
                                         (float)out[k+3],(float)out[k+4],(int)out[k+5]));
        }
        return new ScreenPlan(source,players);
    }

    @Override public void close(){
        owner();nativeClose(handle);handle=0;lastSource=null;
    }
}
