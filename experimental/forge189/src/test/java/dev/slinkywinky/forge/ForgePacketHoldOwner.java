package dev.slinkywinky.forge;

import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelInboundHandlerAdapter;
import io.netty.util.ReferenceCountUtil;
import java.util.ArrayDeque;
import net.minecraft.network.Packet;
import net.minecraft.network.play.server.S18PacketEntityTeleport;

/**
 * R344: isolated actual Netty Packet ownership/release experiment.
 *
 * DELIBERATELY NOT REGISTERED by ForgePreview or any live NetworkManager:
 * the default R339 tap remains 100% pass-through. A future adapter must
 * establish genuine original event timing and source policy ownership.
 *
 * A single Netty event-loop thread owns this handler, every real Packet
 * object, and the delivery order. No client/world/GL/JNI references.
 * Test harness controls admission and expiry; there is no background timer.
 */
final class ForgePacketHoldOwner extends ChannelInboundHandlerAdapter {
    static final int MAX_PENDING=64;
    static final long MAX_AGE_NANOS=150_000_000L;

    interface Clock {long nowNanos();}
    static final class Metrics {
        final long accepted,replayed,abandoned,failOpen,expired;
        final int pending;
        final boolean armed,retired;
        Metrics(ForgePacketHoldOwner o){
            accepted=o.accepted;replayed=o.replayed;
            abandoned=o.abandoned;failOpen=o.failOpen;expired=o.expired;
            pending=o.pending.size();armed=o.armed;retired=o.retired;
        }
    }
    private static final class Pending {
        final Packet packet;
        final long receivedNanos;
        Pending(Packet packet,long at){this.packet=packet;receivedNanos=at;}
    }

    private final Thread eventOwner=Thread.currentThread();
    private final Clock clock;
    private final ArrayDeque<Pending> pending=new ArrayDeque<Pending>(MAX_PENDING);
    private ChannelHandlerContext context;
    private boolean armed,retired,forwarding;
    private int selectedEntityId;
    private long accepted,replayed,abandoned,failOpen,expired;
    private long previousClock;

    ForgePacketHoldOwner(Clock clock) {
        if(clock==null)throw new IllegalArgumentException("Packet clock required");
        this.clock=clock;
    }
    private void owner(){
        if(Thread.currentThread()!=eventOwner)
            throw new IllegalStateException("R344 Netty packet ownership is single-event-loop only");
        if(context!=null&&!context.executor().inEventLoop())
            throw new IllegalStateException("R344 callback on foreign event loop");
    }
    private long now(){
        final long value=clock.nowNanos();
        if(previousClock!=0&&value<previousClock)
            throw new IllegalStateException("R344 packet clock regressed");
        previousClock=value;
        return value;
    }
    @Override public void handlerAdded(ChannelHandlerContext ctx){
        owner();
        if(context!=null)throw new IllegalStateException("R344 packet handler reused");
        context=ctx;
    }
    /** Admission permitted only for an explicitly selected entity ID. */
    void arm(int entityId){
        owner();
        if(context==null||retired||entityId<=0)
            throw new IllegalStateException("R344 cannot arm a retired/unattached packet owner");
        if(!context.channel().isActive())
            throw new IllegalStateException("R344 cannot arm inactive network");
        if(armed&&entityId!=selectedEntityId)releaseAll();
        armed=true;selectedEntityId=entityId;
    }
    void disarm(){
        owner();
        if(retired)return;
        armed=false;selectedEntityId=0;
        releaseAll();
    }
    private void forward(Packet msg,ChannelHandlerContext ctx){
        // A Packet is delivered exactly once and only from this Netty loop.
        ctx.fireChannelRead(msg);
    }
    private void releaseAll(){
        owner();
        if(context==null||pending.isEmpty())return;
        if(forwarding)throw new IllegalStateException("R344 reentrant release");
        forwarding=true;
        try{
            while(!pending.isEmpty()){
                final Pending p=pending.removeFirst();
                forward(p.packet,context);
                ++replayed;
            }
        }finally{forwarding=false;}
    }
    /** Explicit test/event-loop drive. Without a trusted timer, no auto-delay. */
    void releaseDue(){
        owner();
        if(retired)return;
        final long time=now();
        if(!pending.isEmpty()){
            final long age=time-pending.peekFirst().receivedNanos;
            if(age<0) {
                armed=false;selectedEntityId=0;
                releaseAll();++failOpen;return;
            }
            if(age>=MAX_AGE_NANOS){
                expired+=pending.size();
                armed=false;selectedEntityId=0;
                releaseAll();
            }
        }
    }
    Metrics metrics(){owner();return new Metrics(this);}
    @Override public void channelRead(ChannelHandlerContext ctx,Object msg){
        owner();
        if(retired||!armed||!(msg instanceof Packet)){
            if(!pending.isEmpty())releaseAll();
            ctx.fireChannelRead(msg);
            return;
        }
        // Every held reference must be the exact original Minecraft Packet
        // (never a ByteBuf), and every subsequent Packet is queued to avoid
        // silently passing another packet ahead of the held one.
        final Packet packet=(Packet)msg;
        final boolean isSelectedTeleport=
            msg instanceof S18PacketEntityTeleport &&
            ((S18PacketEntityTeleport)msg).getEntityId()==selectedEntityId;
        if(pending.isEmpty()&&!isSelectedTeleport){
            ctx.fireChannelRead(msg);return;
        }
        final long time;
        try{time=now();}
        catch(RuntimeException badClock){
            armed=false;selectedEntityId=0;
            releaseAll();++failOpen;
            ctx.fireChannelRead(msg);return;
        }
        if(!pending.isEmpty()){
            long age=time-pending.peekFirst().receivedNanos;
            if(age<0||age>=MAX_AGE_NANOS){
                armed=false;selectedEntityId=0;
                expired+=age>=MAX_AGE_NANOS?pending.size():0;
                releaseAll();++failOpen;
                ctx.fireChannelRead(msg);return;
            }
        }
        if(pending.size()==MAX_PENDING){
            armed=false;selectedEntityId=0;
            releaseAll();++failOpen;
            ctx.fireChannelRead(msg);return;
        }
        pending.addLast(new Pending(packet,time));
        ++accepted;
    }
    /** A disconnected Channel cannot safely replay into a dead game world. */
    @Override public void channelInactive(ChannelHandlerContext ctx)throws Exception {
        owner();
        retired=true;armed=false;selectedEntityId=0;
        discardRetained();
        ctx.fireChannelInactive();
    }
    @Override public void handlerRemoved(ChannelHandlerContext ctx)throws Exception {
        owner();
        if(!retired&&ctx.channel().isActive())
            releaseAll();
        else discardRetained();
        retired=true;armed=false;selectedEntityId=0;
        context=null;
    }
    private void discardRetained(){
        while(!pending.isEmpty()){
            Packet msg=pending.removeFirst().packet;
            ReferenceCountUtil.release(msg);
            ++abandoned;
        }
    }
}
