package dev.slinkywinky.instrument;

import dev.slinkywinky.SourcePacketReadOnlyQueue;

/**
 * R365 only: ephemeral packet argument -> source primitive FIFO.
 * Producer may be any original NetworkManager sender thread. Draining and
 * close belong to the game client thread. Does not hook or block a packet.
 */
public final class Vanilla189OutboundPrimitiveQueue implements AutoCloseable {
    private final Thread owner=Thread.currentThread();
    private final Vanilla189OutboundPacketSampler sampler;
    private final SourcePacketReadOnlyQueue ingress=new SourcePacketReadOnlyQueue();
    private volatile boolean closed;
    private volatile boolean failed;
    private long admittedBatches;

    public Vanilla189OutboundPrimitiveQueue(Class<?> packetClass,Class<?> movement)
            throws ReflectiveOperationException {
        sampler=new Vanilla189OutboundPacketSampler(packetClass,movement);
    }
    public Vanilla189OutboundPrimitiveQueue(Class<?> packetClass,Class<?> movement,
            Class<?> useEntity,Class<?> entityAction,Class<?> actionEnum)
            throws ReflectiveOperationException {
        sampler=new Vanilla189OutboundPacketSampler(packetClass,movement,
            useEntity,entityAction,actionEnum);
    }
    /** ALWAYS called before original send; never changes the send result. */
    public boolean observe(Object packet,long nowNanos) {
        if(closed||failed)return false;
        try{
            if(nowNanos<=0)throw new IllegalArgumentException("Invalid packet clock");
            Vanilla189OutboundPacketSampler.Sample s=sampler.sample(packet);
            if(closed)return false;
            final boolean offered=ingress.offer(
                SourcePacketReadOnlyQueue.OUT,s.kind,0,false,s.positionKnown,
                s.x,s.y,s.z,nowNanos);
            if(!offered)failed=true;
            return offered;
        }catch(Throwable failure){
            failed=true;ingress.poison();
            return false; // original send ALWAYS proceeds unchanged
        }
    }
    public SourcePacketReadOnlyQueue.Batch drain(int max){
        if(closed||Thread.currentThread()!=owner)
            throw new IllegalStateException("R365 Netty snapshot requires client owner");
        if(failed||!ingress.healthy())
            throw new IllegalStateException("R365 incomplete original packet stream");
        final SourcePacketReadOnlyQueue.Batch batch=ingress.drain(max);
        ++admittedBatches;
        return batch;
    }
    public boolean healthy(){return !closed&&!failed&&ingress.healthy();}
    public int pending(){return ingress.pending();}
    public long batches(){
        if(Thread.currentThread()!=owner)
            throw new IllegalStateException("R365 counter read on foreign thread");
        return admittedBatches;
    }
    @Override public void close(){
        if(Thread.currentThread()!=owner)
            throw new IllegalStateException("R365 only original client owner may retire");
        if(closed)return;
        closed=true;ingress.close();sampler.close();
    }
}
