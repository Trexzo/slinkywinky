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
    /** Render-time immutable ray input; no game objects or action sink. */
    public interface RenderPort extends Port {
        RenderFrame captureRender(double deltaSeconds,float partialTicks);
    }
    public static final class RenderFrame {
        final double[] values;
        final boolean swordHeld;
        public RenderFrame(double[] values,boolean swordHeld){
            if(values==null||values.length!=20)
                throw new IllegalArgumentException("Expected 20 Timer Range render inputs");
            this.values=values.clone();
            this.swordHeld=swordHeld;
            for(double v:this.values)
                if(!Double.isFinite(v))
                    throw new IllegalArgumentException("Nonfinite source render position");
            if(this.values[0]<0||this.values[0]>60||
               this.values[1]<0||this.values[1]>1)
                throw new IllegalArgumentException("Source render delta/partial invalid");
        }
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
    private SourceRuntimePreview.Receipt lastRender;
    private SourcePacketReadOnlyQueue packetIngress;
    private int lastPacketSequence;
    private long packetObservationCount;
    public static final class PacketReceipt {
        public final long generation,tick,observed;
        public final int incoming,outgoing,inventory,callbacks;
        private PacketReceipt(long g,long t,long observed,int inbound,
                              int outbound,int inventory,int callbacks){
            generation=g;tick=t;this.observed=observed;
            incoming=inbound;outgoing=outbound;this.inventory=inventory;
            this.callbacks=callbacks;
        }
    }
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
    public SourceRuntimePreview.Receipt lastRender(){owner();return lastRender;}
    private void invalidate(){
        last=null;lastRender=null;packetIngress=null;
        lastPacketSequence=0;packetObservationCount=0;
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
    /**
     * R339: replay bounded primitive Netty observations ONLY on the admitted
     * game thread, after a verified world/client tick. Packet I/O has
     * already passed through Minecraft; these are hypothetical policies.
     * No native packet is withheld, replayed, cancelled or transmitted.
     */
    public PacketReceipt observePackets(Port port,SourcePacketReadOnlyQueue ingress){
        owner();
        if(inCall)throw new IllegalStateException("Reentrant packet source observer");
        if(port==null||ingress==null)
            throw new IllegalArgumentException("Missing packet Port/ingress");
        inCall=true;
        try{
            if(!port.admittedClientThread())
                throw new IllegalStateException("Network replay on foreign thread");
            if(world==null||player==null||last==null)return null;
            if(port.worldIdentity()!=world||port.playerIdentity()!=player){
                invalidate();return null;
            }
            if(packetIngress!=null&&packetIngress!=ingress)
                throw new IllegalStateException("Network channel changed within owner generation");
            if(!ingress.healthy())
                throw new IllegalStateException("Incomplete / overflowed Netty event stream");
            if(packetIngress==null)packetIngress=ingress;
            final SourcePacketReadOnlyQueue.Batch batch=ingress.drain(128);
            int incoming=0,outgoing=0,inventory=0,callbacks=0;
            for(SourcePacketReadOnlyQueue.Observation o:batch.rows){
                if(!port.admittedClientThread()||
                   port.worldIdentity()!=world||port.playerIdentity()!=player)
                    throw new IllegalStateException("World switched mid packet drain");
                if(o.sequence<=lastPacketSequence)
                    throw new IllegalStateException("Packet observation order regression");
                lastPacketSequence=o.sequence;
                if(o.direction==SourcePacketReadOnlyQueue.IN){
                    // S19 status exposes an entity only via a World lookup.
                    // Unknown ID must never simulate known target feedback.
                    int kind=0;
                    if(o.kind==SourcePacketReadOnlyQueue.ENTITY_STATUS&&o.entityIdKnown)
                        kind=1;
                    else if(o.kind==SourcePacketReadOnlyQueue.UPDATE_HEALTH)
                        kind=2;
                    else if(o.kind==SourcePacketReadOnlyQueue.SOUND)kind=3;
                    else if(o.kind==SourcePacketReadOnlyQueue.VELOCITY)kind=4;
                    else if(o.kind==SourcePacketReadOnlyQueue.TELEPORT)kind=5;
                    SourceRuntimePreview.Receipt r=nativePreview.observe(
                        worldNumber,playerNumber,generation,tick,
                        SourceRuntimePreview.BACKTRACK_PACKET,
                        new int[]{kind,o.sequence,o.entityId,1},
                        new double[]{0});
                    if(r.sourceEvent!=2)throw new IllegalStateException("Wrong source ingress phase");
                    ++incoming;++callbacks;
                }else if(o.direction==SourcePacketReadOnlyQueue.OUT){
                    int kind=o.kind==SourcePacketReadOnlyQueue.MOVEMENT?1:
                        o.kind==SourcePacketReadOnlyQueue.ATTACK?2:
                        o.kind==SourcePacketReadOnlyQueue.STOP_SPRINT?3:0;
                    SourceRuntimePreview.Receipt r=nativePreview.observe(
                        worldNumber,playerNumber,generation,tick,
                        SourceRuntimePreview.LAG_RANGE_PACKET,
                        new int[]{kind,o.sequence,o.moving?1:0,1},
                        new double[]{0,o.x,o.y,o.z});
                    if(r.sourceEvent!=3)throw new IllegalStateException("Wrong source egress phase");
                    ++outgoing;++callbacks;
                    final int inv=o.kind==SourcePacketReadOnlyQueue.INV_OPEN?1:
                        o.kind==SourcePacketReadOnlyQueue.INV_CLICK?2:
                        o.kind==SourcePacketReadOnlyQueue.INV_CLOSE?3:-1;
                    if(inv>=0){
                        SourceRuntimePreview.Receipt gi=nativePreview.observe(
                            worldNumber,playerNumber,generation,tick,
                            SourceRuntimePreview.INV_WALK_PACKET,
                            new int[]{inv},new double[0]);
                        if(gi.sourceEvent!=23)throw new IllegalStateException("Wrong inventory phase");
                        ++inventory;++callbacks;
                    }
                }else throw new IllegalStateException("Malformed packet direction");
                ++packetObservationCount;
            }
            return new PacketReceipt(generation,tick,packetObservationCount,
                                     incoming,outgoing,inventory,callbacks);
        }catch(RuntimeException | Error failed){
            failClosed();throw failed;
        }finally{inCall=false;}
    }

    /** Source frame delivery is read-only, bound to the prior admitted tick. */
    public SourceRuntimePreview.Receipt observeRender(RenderPort port,
                                                       double dt,float partial){
        owner();
        if(inCall)throw new IllegalStateException("Reentrant render source observer");
        if(port==null)throw new IllegalArgumentException("Missing render port");
        inCall=true;
        try{
            if(!port.admittedClientThread())
                throw new IllegalStateException("Foreign render observer");
            if(world==null||player==null||last==null)return null;
            if(port.worldIdentity()!=world||port.playerIdentity()!=player){
                invalidate();
                return null;
            }
            RenderFrame render=port.captureRender(dt,partial);
            if(render==null)return null;
            if(!port.admittedClientThread()||
               port.worldIdentity()!=world||port.playerIdentity()!=player)
                throw new IllegalStateException("Source render owner switched midframe");
            lastRender=nativePreview.observe(worldNumber,playerNumber,generation,tick,
                SourceRuntimePreview.TIMER_RANGE_FRAME,
                new int[]{render.swordHeld?1:0},render.values);
            return lastRender;
        }catch(RuntimeException | Error failure){
            failClosed();
            throw failure;
        }finally{inCall=false;}
    }
    @Override public void close(){
        if(Thread.currentThread()!=thread)
            throw new IllegalStateException("Foreign Minecraft source close");
        if(closed)return;
        if(inCall)throw new IllegalStateException("Cannot close during source delivery");
        failClosed();
    }
}
