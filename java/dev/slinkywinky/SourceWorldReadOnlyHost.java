package dev.slinkywinky;

import java.util.Collections;
import java.util.HashSet;
import java.util.List;

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
    /**
     * R353: conservative singleplayer snapshot proving current-tick attack
     * timestamp (not an exact original event15 phase hook). Optional.
     */
    public interface AttackPort extends Port {
        boolean sourceLagRangeAttackNow();
    }
    /** Render-time immutable ray input; no game objects or action sink. */
    public interface RenderPort extends Port {
        RenderFrame captureRender(double deltaSeconds,float partialTicks);
    }
    public interface BoxPort extends RenderPort {
        BoxFrame captureBoxes(float partialTicks);
    }
    /** R342 only client-applied serverPos fixed-point coordinates. */
    public interface HistoryPort extends BoxPort {
        RemoteTarget captureRemoteTarget();
        // Java8-compatible default retains all earlier owner/JNI fixtures.
        default List<RemoteTarget> captureRemoteTargets(){
            RemoteTarget single=captureRemoteTarget();
            return single==null?
                Collections.<RemoteTarget>emptyList():
                Collections.singletonList(single);
        }
        BoxFrame captureHistoricalBoxes(float partialTicks,
                     SourceRemotePositionHistory.Historical history);
    }
    public static final class RemoteTarget {
        public final int entityId,serverX,serverY,serverZ;
        public final long uuidHigh,uuidLow;
        public RemoteTarget(int id,long hi,long lo,int x,int y,int z){
            entityId=id;uuidHigh=hi;uuidLow=lo;
            serverX=x;serverY=y;serverZ=z;
        }
    }
    public static final class BoxFrame {
        private final double[] coordinates;
        public BoxFrame(double[] coordinates){
            if(coordinates==null||coordinates.length!=12)
                throw new IllegalArgumentException("Source e6 needs 12 immutable values");
            this.coordinates=coordinates.clone();
            for(double v:this.coordinates)
                if(!Double.isFinite(v)||Math.abs(v)>10000000)
                    throw new IllegalArgumentException("Invalid source e6 coordinate");
            if(this.coordinates[9]<0||this.coordinates[9]>1)
                throw new IllegalArgumentException("Invalid source e6 partial tick");
        }
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
        /** Optional authentic JNI e15 receipt from a conservative tick proxy. */
        public final SourceRuntimePreview.Receipt lagRangeAttack;
        private TickReceipt(long generation,long tick,SourceRuntimePreview.Receipt[] r,
                            SourceRuntimePreview.Receipt attack){
            this.generation=generation;this.tick=tick;
            backtrack=r[0];lagRange=r[1];timerRange=r[2];invWalk=r[3];
            lagRangeAttack=attack;
        }
    }
    private final Thread thread=Thread.currentThread();
    private final SourceRuntimePreview nativePreview;
    private Object world,player;
    private long generation=0,tick=0,worldNumber=0,playerNumber=0,tokenSequence=100;
    private boolean closed,inCall;
    private TickReceipt last;
    private SourceRuntimePreview.Receipt lastRender;
    private SourceRuntimePreview.SourceBoxes lastBoxes;
    private SourceRemotePositionHistory.Historical lastHistoryBox;
    private final SourceRemotePositionHistory positionHistory=
        new SourceRemotePositionHistory();
    private SourcePacketReadOnlyQueue packetIngress;
    private int lastPacketSequence;
    private long packetObservationCount;
    public static final class PacketReceipt {
        public final long generation,tick,observed;
        public final int incoming,outgoing,inventory,callbacks;
        /** REAL native JNI packet *proposals*. All packets already forwarded. */
        public final int backtrackHold,backtrackFeedback,lagRangeHold,timerHold;
        private PacketReceipt(long g,long t,long observed,int inbound,
                              int outbound,int inventory,int callbacks,
                              int btHold,int btFeedback,int lagHold,int tmHold){
            generation=g;tick=t;this.observed=observed;
            incoming=inbound;outgoing=outbound;this.inventory=inventory;
            this.callbacks=callbacks;
            backtrackHold=btHold;backtrackFeedback=btFeedback;
            lagRangeHold=lagHold;timerHold=tmHold;
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
    public SourceRuntimePreview.SourceBoxes lastBoxes(){owner();return lastBoxes;}
    public SourceRemotePositionHistory history(){owner();return positionHistory;}
    public SourceRemotePositionHistory.Historical lastHistoryBox(){
        owner();return lastHistoryBox;
    }
    private void invalidate(){
        // On disconnect or direct world replacement, also close the
        // previously admitted primitive packet ingress. Late Netty offers
        // then fail closed while real packet delivery remains untouched.
        if(packetIngress!=null)packetIngress.close();
        last=null;lastRender=null;lastBoxes=null;lastHistoryBox=null;
        packetIngress=null;
        positionHistory.resetWorld();
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
        finally{closed=true;invalidate();positionHistory.retire();}
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
                // A direct ClientTick world/player replacement must retire
                // packet channel identity, historical UUID/positions and
                // old JNI receipts, EVEN if no render/packet callback ran
                // between the two worlds. Otherwise old entity-ID history
                // can silently survive a dimension/world transition.
                invalidate();
                ++generation;
                world=readWorld;player=readPlayer;tick=0;
                worldNumber=++tokenSequence;playerNumber=++tokenSequence;
            }
            if(tick==Long.MAX_VALUE)
                throw new IllegalStateException("Source world tick exhausted");
            ++tick;
            SourceRuntimePreview.Receipt attack=null;
            // Original candidate Lag Range event15 must precede event18.
            // A Forge ClientTick.END read is NOT exact source phase timing.
            if(port instanceof AttackPort&&((AttackPort)port).sourceLagRangeAttackNow()){
                if(!port.admittedClientThread()||port.worldIdentity()!=world||
                   port.playerIdentity()!=player)
                    throw new IllegalStateException("R353 attack observation changed owner");
                attack=nativePreview.observe(worldNumber,playerNumber,generation,tick,
                    SourceRuntimePreview.LAG_RANGE_ATTACK,new int[]{1,1},new double[0]);
                if(attack.sourceEvent!=15||attack.at(2)!=1||attack.at(3)!=1)
                    throw new IllegalStateException("R353 source attack receipt invalid");
            }
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
            if(port instanceof HistoryPort){
                final List<RemoteTarget> samples=
                    ((HistoryPort)port).captureRemoteTargets();
                if(samples==null||
                   samples.size()>SourceRemotePositionHistory.MAX_ENTITIES)
                    throw new IllegalStateException("Unbounded remote history batch");
                final HashSet<Integer> unique=new HashSet<Integer>();
                final long sampleTime=System.nanoTime();
                for(RemoteTarget sample:samples){
                    if(sample==null||!unique.add(sample.entityId))
                        throw new IllegalStateException("Duplicate/null remote entity batch");
                    if(!port.admittedClientThread()||
                       port.worldIdentity()!=world||port.playerIdentity()!=player)
                        throw new IllegalStateException("History target world changed");
                    positionHistory.applied(sample.entityId,
                        sample.uuidHigh,sample.uuidLow,sample.serverX,
                        sample.serverY,sample.serverZ,sampleTime);
                }
            }
            last=new TickReceipt(generation,tick,result,attack);
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
            int btHeld=0,btFeedback=0,lagHeld=0,timerHeld=0;
            for(SourcePacketReadOnlyQueue.Observation o:batch.rows){
                if(!port.admittedClientThread()||
                   port.worldIdentity()!=world||port.playerIdentity()!=player)
                    throw new IllegalStateException("World switched mid packet drain");
                if(o.sequence<=lastPacketSequence)
                    throw new IllegalStateException("Packet observation order regression");
                lastPacketSequence=o.sequence;
                // Observe only primitive facts, validate them against future
                // client-applied serverPos samples; no Packet/Entity refs.
                positionHistory.packet(o);
                if(o.direction==SourcePacketReadOnlyQueue.IN){
                    // S19 status exposes an entity only via a World lookup.
                    // Unknown ID must never simulate known target feedback.
                    final int kind=SourcePacketReadOnlyQueue.sourceBacktrackKind(o);
                    SourceRuntimePreview.Receipt r=nativePreview.observe(
                        worldNumber,playerNumber,generation,tick,
                        SourceRuntimePreview.BACKTRACK_PACKET,
                        new int[]{kind,o.sequence,o.entityId,1},
                        new double[]{0});
                    if(r.sourceEvent!=2)throw new IllegalStateException("Wrong source ingress phase");
                    if(r.at(3)==1)++btHeld;
                    else if(r.at(3)==2)++btFeedback;
                    else if(r.at(3)!=0)throw new IllegalStateException(
                        "Invalid native Backtrack packet proposal");
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
                    if(r.at(3)==1)++lagHeld;
                    else if(r.at(3)!=0)throw new IllegalStateException(
                        "Invalid native Lag Range packet proposal");
                    ++outgoing;++callbacks;
                    // Timer Range's original event1 is invoked for every
                    // outgoing packet. The result is NOT an on-wire verdict:
                    // this packet was already forwarded unchanged by Netty.
                    SourceRuntimePreview.Receipt timerPacket=nativePreview.observe(
                        worldNumber,playerNumber,generation,tick,
                        SourceRuntimePreview.TIMER_RANGE_PACKET,
                        new int[0],new double[0]);
                    if(timerPacket.sourceEvent!=1)
                        throw new IllegalStateException("Wrong source Timer Range packet phase");
                    if(timerPacket.at(3)==1)++timerHeld;
                    else if(timerPacket.at(3)!=0)throw new IllegalStateException(
                        "Invalid native Timer Range packet proposal");
                    ++callbacks;
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
                incoming,outgoing,inventory,callbacks,
                btHeld,btFeedback,lagHeld,timerHeld);
        }catch(RuntimeException | Error failed){
            failClosed();throw failed;
        }finally{inCall=false;}
    }

    /**
     * R342: native source event6 receives only an authenticated *past*
     * vanilla serverPos history: >=80ms old and <=600ms old. Distinct
     * from the live-target R341 geometry, and NOT original packet delay.
     */
    public SourceRuntimePreview.SourceBoxes observeHistoricalBoxes(
                                                HistoryPort port,float partial){
        owner();
        if(inCall)throw new IllegalStateException("Reentrant history render");
        if(port==null)throw new IllegalArgumentException("Missing history render port");
        inCall=true;
        try{
            if(!port.admittedClientThread())
                throw new IllegalStateException("Foreign history render owner");
            if(world==null||player==null||last==null)return null;
            if(port.worldIdentity()!=world||port.playerIdentity()!=player){
                invalidate();return null;
            }
            RemoteTarget target=port.captureRemoteTarget();
            if(target==null)return null;
            final SourceRemotePositionHistory.Historical historic=
                positionHistory.historical(target.entityId,
                    target.uuidHigh,target.uuidLow,System.nanoTime(),
                    SourceRemotePositionHistory.MIN_AGE_NANOS,
                    SourceRemotePositionHistory.MAX_AGE_NANOS);
            if(historic==null)return null;
            BoxFrame boxes=port.captureHistoricalBoxes(partial,historic);
            if(boxes==null)return null;
            if(!port.admittedClientThread()||
               port.worldIdentity()!=world||port.playerIdentity()!=player)
                throw new IllegalStateException("History render world switched");
            lastBoxes=nativePreview.observeBoxes(worldNumber,playerNumber,
                generation,tick,boxes.coordinates);
            lastHistoryBox=historic;
            return lastBoxes;
        }catch(RuntimeException | Error failure){
            failClosed();throw failure;
        }finally{inCall=false;}
    }

    /** R341 source event6 immutable shared-camera render geometry. */
    public SourceRuntimePreview.SourceBoxes observeBoxes(BoxPort port,float partial){
        owner();
        if(inCall)throw new IllegalStateException("Reentrant source box request");
        if(port==null)throw new IllegalArgumentException("Missing source e6 Port");
        inCall=true;
        try{
            if(!port.admittedClientThread())
                throw new IllegalStateException("Foreign source e6 render thread");
            if(world==null||player==null||last==null)return null;
            if(port.worldIdentity()!=world||port.playerIdentity()!=player){
                invalidate();
                return null;
            }
            final BoxFrame boxes=port.captureBoxes(partial);
            if(boxes==null)return null;
            if(!port.admittedClientThread()||
               port.worldIdentity()!=world||port.playerIdentity()!=player)
                throw new IllegalStateException("Source e6 world switched midread");
            lastBoxes=nativePreview.observeBoxes(worldNumber,playerNumber,
                generation,tick,boxes.coordinates);
            return lastBoxes;
        }catch(RuntimeException | Error failure){
            failClosed();throw failure;
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
