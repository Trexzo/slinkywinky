package dev.slinkywinky;

import java.util.ArrayDeque;

/**
 * R347: REAL source-event JNI packet decision -> bounded primitive receipt
 * handoff to a foreign Netty observer. JNI is ALWAYS invoked on the Minecraft
 * client owner, NEVER on Netty. This is a read-only experiment: source decisions
 * MUST NOT be applied retroactively to packets already forwarded by R339.
 *
 * Receipts are generated from the same immutable SourcePacketReadOnlyQueue
 * Observation that supplies direction, kind, sequence and timestamp.
 */
public final class SourcePacketVerdictBridge implements AutoCloseable {
    public static final int CAPACITY=64;
    public static final long MAX_PACKET_AGE_NANOS=100_000_000L;
    public static final long MAX_DECISION_AGE_NANOS=100_000_000L;
    public static final int BACKTRACK_UNCHANGED=0;
    public static final int BACKTRACK_HOLD=1;
    public static final int BACKTRACK_FEEDBACK=2;
    public static final int LAG_RANGE_PASS=0;
    public static final int LAG_RANGE_HOLD=1;

    public static final class Decision {
        public final long channelToken,worldToken,playerToken,generation,tick;
        public final long publishedNanos,packetNanos;
        public final int packetSequence,direction,kind,entityId,sourceEvent;
        public final int nativeVerdict;
        public final boolean entityKnown,positionKnown;
        public final double x,y,z;
        private Decision(Epoch e,SourcePacketReadOnlyQueue.Observation o,
                         int event,int verdict,long now){
            channelToken=e.channel;worldToken=e.world;playerToken=e.player;
            generation=e.generation;tick=e.tick;publishedNanos=now;
            packetNanos=o.nanoTime;packetSequence=o.sequence;
            direction=o.direction;kind=o.kind;entityId=o.entityId;
            entityKnown=o.entityIdKnown;positionKnown=o.moving;
            x=o.x;y=o.y;z=o.z;sourceEvent=event;nativeVerdict=verdict;
        }
        /** Source proposal only. No live Netty traffic has been delayed. */
        public boolean hypotheticalHold(){
            return direction==SourcePacketReadOnlyQueue.IN?
                nativeVerdict==BACKTRACK_HOLD:
                nativeVerdict==LAG_RANGE_HOLD;
        }
    }
    private static final class Epoch {
        final long channel,world,player,generation,tick;
        Epoch(long channel,long world,long player,long gen,long tick){
            this.channel=channel;this.world=world;this.player=player;
            generation=gen;this.tick=tick;
        }
    }
    private final SourceRuntimePreview source;
    private final Thread owner=Thread.currentThread();
    private final ArrayDeque<Decision> fifo=new ArrayDeque<Decision>(CAPACITY);
    private Epoch epoch;
    private int lastPacketSequence;
    private boolean closed,failed;
    private long rejected,consumed,produced;

    public SourcePacketVerdictBridge(SourceRuntimePreview source){
        if(source==null)throw new IllegalArgumentException("Real JNI source required");
        this.source=source;
    }
    private void assertOwner(){
        if(Thread.currentThread()!=owner||closed)
            throw new IllegalStateException("R347 native source belongs to client owner");
    }
    private static void validateTick(SourceRuntimePreview.Receipt r,int opcode,
                                     SourceWorldReadOnlyHost.TickReceipt tick){
        if(r==null||r.opcode!=opcode||r.sourceEvent!=18||
           r.generation!=tick.generation||r.tick!=tick.tick||
           (r.at(3)!=0&&r.at(3)!=1))
            throw new IllegalArgumentException("R347 untrusted JNI native tick");
    }
    /**
     * Previous tick decisions cannot survive a new JNI source tick. A new
     * channel/world requires a NEW native generation; no generation rollback.
     */
    public synchronized void begin(long channelToken,
                                   SourceWorldReadOnlyHost.TickReceipt tick){
        assertOwner();
        if(tick==null||tick.generation<=0||tick.tick<=0||channelToken<=0)
            throw new IllegalArgumentException("R347 source epoch incomplete");
        validateTick(tick.backtrack,SourceRuntimePreview.BACKTRACK_TICK,tick);
        validateTick(tick.lagRange,SourceRuntimePreview.LAG_RANGE_TICK,tick);
        if(tick.backtrack.world!=tick.lagRange.world||
           tick.backtrack.player!=tick.lagRange.player||
           tick.backtrack.world<=0||tick.backtrack.player<=0||
           tick.backtrack.world==tick.backtrack.player)
            throw new IllegalArgumentException("Mixed native source world/player");
        final Epoch prev=epoch;
        if(prev!=null&&(tick.generation<prev.generation||
            (tick.generation==prev.generation&&(tick.tick<=prev.tick||
             channelToken!=prev.channel||
             tick.backtrack.world!=prev.world||
             tick.backtrack.player!=prev.player))))
            throw new IllegalStateException("R347 JNI epoch/channel replay");
        rejected+=fifo.size();fifo.clear();
        epoch=new Epoch(channelToken,tick.backtrack.world,
            tick.backtrack.player,tick.generation,tick.tick);
        lastPacketSequence=0;
        failed=false;
    }

    /**
     * Input Observation is already detached from the Netty Packet.
     * Same client-thread method computes its REAL JNI verdict, checks JNI
     * result shape, and records the primitive Decision atomically.
     */
    public Decision observe(SourcePacketReadOnlyQueue.Observation o,
                            long nowNanos){
        assertOwner();
        final Epoch active;
        // R347.1: keep the Netty reader monitor out of slow JNI. The
        // expensive source.observe(...) below is on the client owner ONLY.
        synchronized(this){
            if(epoch==null||failed||o==null)
                throw new IllegalStateException("R347 source epoch unavailable");
            if(o.sequence<=lastPacketSequence||o.sequence<1)
                throw new IllegalArgumentException("Packet sequence replay or regression");
            final long ageCheck=nowNanos-o.nanoTime;
            if(ageCheck<0||ageCheck>MAX_PACKET_AGE_NANOS){
                failClosed();
                throw new IllegalStateException("Packet too old/future for source verdict");
            }
            if(fifo.size()>=CAPACITY){
                failClosed();
                throw new IllegalStateException("Bounded Netty verdict mailbox overflow");
            }
            if(o.direction!=SourcePacketReadOnlyQueue.IN&&
               o.direction!=SourcePacketReadOnlyQueue.OUT){
                failClosed();
                throw new IllegalArgumentException("Malformed network direction");
            }
            active=epoch;
        }
        final long age=nowNanos-o.nanoTime;
        final int ms=(int)(age/1_000_000L);
        final int opcode,event;
        final int[] flags;
        final double[] fields;
        if(o.direction==SourcePacketReadOnlyQueue.IN){
            opcode=SourceRuntimePreview.BACKTRACK_PACKET;
            event=2;
            final int kind=SourcePacketReadOnlyQueue.sourceBacktrackKind(o);
            flags=new int[]{kind,o.sequence,o.entityId,1};
            fields=new double[]{ms};
        }else{
            opcode=SourceRuntimePreview.LAG_RANGE_PACKET;
            event=3;
            int k=o.kind==SourcePacketReadOnlyQueue.MOVEMENT?1:
                  o.kind==SourcePacketReadOnlyQueue.ATTACK?2:
                  o.kind==SourcePacketReadOnlyQueue.STOP_SPRINT?3:0;
            flags=new int[]{k,o.sequence,o.moving?1:0,1};
            fields=new double[]{ms,o.x,o.y,o.z};
        }
        try{
            SourceRuntimePreview.Receipt r=source.observe(
                active.world,active.player,active.generation,active.tick,
                opcode,flags,fields);
            int verdict=(int)r.at(3);
            if(r.world!=active.world||r.player!=active.player||
               r.generation!=active.generation||r.tick!=active.tick||
               r.opcode!=opcode||r.sourceEvent!=event||
               r.at(3)!=verdict||verdict<0||
               verdict>(event==2?BACKTRACK_FEEDBACK:LAG_RANGE_HOLD))
                throw new IllegalStateException("Wrong native source packet verdict");
            synchronized(this){
                // Netty can consume/revoke stale queued entries while JNI
                // works. Never republish after such a failure or epoch swap.
                if(closed||failed||epoch!=active)
                    throw new IllegalStateException("Native result outlived owner epoch");
                if(fifo.size()>=CAPACITY){
                    failClosed();
                    throw new IllegalStateException("Source verdict cap after JNI");
                }
                Decision d=new Decision(active,o,event,verdict,nowNanos);
                fifo.addLast(d);lastPacketSequence=o.sequence;++produced;
                return d;
            }
        }catch(RuntimeException|Error failure){
            synchronized(this){failClosed();}
            throw failure;
        }
    }

    /**
     * Foreign Netty read only, strict channel/world/generation + next packet
     * sequence match. A skip, stale receipt or queue overflow closes epoch
     * instead of silently assigning a verdict to a DIFFERENT Packet.
     */
    public synchronized Decision consume(long channelToken,long worldToken,
                                         long generation,int expectedSequence,
                                         long nowNanos){
        if(closed||failed||epoch==null||expectedSequence<=0||
           epoch.channel!=channelToken||epoch.world!=worldToken||
           epoch.generation!=generation)return null;
        final Decision head=fifo.peekFirst();
        if(head==null||head.packetSequence>expectedSequence)return null;
        final long age=nowNanos-head.publishedNanos;
        if(head.packetSequence<expectedSequence||age<0||
           age>MAX_DECISION_AGE_NANOS){
            failClosed();return null;
        }
        fifo.removeFirst();++consumed;
        return head;
    }
    private void failClosed(){
        rejected+=fifo.size();fifo.clear();failed=true;
    }
    public synchronized void revoke(){
        assertOwner();
        rejected+=fifo.size();fifo.clear();
        epoch=null;lastPacketSequence=0;failed=false;
    }
    public synchronized int pending(){return fifo.size();}
    public synchronized boolean healthy(){return !closed&&!failed&&epoch!=null;}
    public synchronized long produced(){return produced;}
    public synchronized long consumed(){return consumed;}
    public synchronized long rejected(){return rejected;}
    @Override public synchronized void close(){
        if(Thread.currentThread()!=owner)
            throw new IllegalStateException("R347 only client owner can retire JNI bridge");
        if(closed)return;
        rejected+=fifo.size();fifo.clear();
        epoch=null;closed=true;failed=true;
    }
}
