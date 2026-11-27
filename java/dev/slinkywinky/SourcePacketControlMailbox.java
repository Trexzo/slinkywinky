package dev.slinkywinky;

import java.util.concurrent.atomic.AtomicReference;

/**
 * R346: a ONE-SLOT volatile immutable JNI-to-Netty policy observation ticket.
 * Single Minecraft client-thread publisher; foreign Netty event-loop readers.
 *
 * Native source tick 'active' is NOT a native packet hold verdict; tickets are
 * non-actionable candidates. No packet/world/entity/Channel/JNI references.
 * No packet-owner implementation is wired to this mailbox in the live mod.
 */
public final class SourcePacketControlMailbox implements AutoCloseable {
    public static final long MAX_TICKET_AGE_NANOS=100_000_000L;
    public static final class Ticket {
        public final long channelToken,worldToken,playerToken,generation,tick;
        public final long sequence,publishedNanos;
        public final int targetEntityId;
        public final boolean backtrackCandidate,lagRangeCandidate;
        private Ticket(long channel,long world,long player,long generation,
                       long tick,long seq,long at,int target,
                       boolean backtrack,boolean lag){
            channelToken=channel;worldToken=world;playerToken=player;
            this.generation=generation;this.tick=tick;sequence=seq;
            publishedNanos=at;targetEntityId=target;
            backtrackCandidate=backtrack;lagRangeCandidate=lag;
        }
        /** Source tick ACTIVE, not original Slinky packet hold approval. */
        public boolean hasCandidate(){return backtrackCandidate||lagRangeCandidate;}
    }
    private final Thread publisher=Thread.currentThread();
    private final AtomicReference<Ticket> latest=new AtomicReference<Ticket>();
    private volatile boolean closed;
    private long serial,lastGeneration,lastTick,lastPublished=Long.MIN_VALUE;
    private void owner(){
        if(closed||publisher!=Thread.currentThread())
            throw new IllegalStateException("R346 publisher must be live client-thread owner");
    }
    private static void source(SourceRuntimePreview.Receipt r,int opcode,
                               long world,long player,long gen,long tick){
        if(r==null||r.opcode!=opcode||r.world!=world||r.player!=player||
           r.generation!=gen||r.tick!=tick||
           (r.at(3)!=0&&r.at(3)!=1))
            throw new IllegalStateException("Untrusted native source tick policy receipt");
    }
    public Ticket publish(long channelToken,int entityId,
               SourceWorldReadOnlyHost.TickReceipt tickReceipt,long nowNanos){
        owner();
        if(tickReceipt==null||tickReceipt.backtrack==null||
           tickReceipt.lagRange==null||tickReceipt.generation<=0||
           tickReceipt.tick<=0||channelToken<=0||entityId<0)
            throw new IllegalArgumentException("Invalid R346 client packet policy identity");
        final SourceRuntimePreview.Receipt b=tickReceipt.backtrack;
        final SourceRuntimePreview.Receipt l=tickReceipt.lagRange;
        final long world=b.world,player=b.player,gen=tickReceipt.generation;
        final long tick=tickReceipt.tick;
        if(world<=0||player<=0||world==player||b.generation!=gen||
           b.tick!=tick||gen<lastGeneration||
           (gen==lastGeneration&&tick<=lastTick)||
           (gen==lastGeneration&&nowNanos<lastPublished)||
           serial==Long.MAX_VALUE)
            throw new IllegalStateException("R346 stale/replayed JNI source frame");
        source(b,SourceRuntimePreview.BACKTRACK_TICK,
               world,player,gen,tick);
        source(l,SourceRuntimePreview.LAG_RANGE_TICK,
               world,player,gen,tick);
        boolean canBacktrack=b.at(3)==1&&entityId>0;
        boolean canLagRange=l.at(3)==1;
        final Ticket ticket=new Ticket(channelToken,world,player,gen,
              tick,++serial,nowNanos,entityId,canBacktrack,canLagRange);
        lastGeneration=gen;lastTick=tick;lastPublished=nowNanos;
        latest.set(ticket);
        return ticket;
    }
    /**
     * May be called on Netty. Exact channel/world/generation token check;
     * 'now' strictly bounded to 100ms after client's source tick.
     * Returns only a candidate, never authority to hold a packet.
     */
    public Ticket inspect(long channelToken,long worldToken,long generation,
                          long nowNanos){
        if(closed||channelToken<=0||worldToken<=0||generation<=0)return null;
        final Ticket ticket=latest.get();
        if(ticket==null||ticket.channelToken!=channelToken||
           ticket.worldToken!=worldToken||ticket.generation!=generation)
            return null;
        final long age=nowNanos-ticket.publishedNanos;
        if(age<0||age>MAX_TICKET_AGE_NANOS||closed||latest.get()!=ticket)
            return null;
        return ticket;
    }
    /** Invalidate pending candidates on disconnect and world replacement. */
    public void revoke(){
        owner();
        latest.set(null);
    }
    @Override public void close(){
        if(publisher!=Thread.currentThread())
            throw new IllegalStateException("R346 mailbox must close on client owner");
        if(closed)return;
        closed=true;
        latest.set(null);
    }
}
