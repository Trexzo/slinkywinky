package dev.slinkywinky.forge;

import io.netty.channel.ChannelDuplexHandler;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelPromise;
import io.netty.util.ReferenceCountUtil;
import java.nio.channels.ClosedChannelException;
import java.util.ArrayDeque;
import net.minecraft.network.Packet;
import net.minecraft.network.play.client.C03PacketPlayer;

/**
 * R345: OUTBOUND real Netty Packet + ORIGINAL ChannelPromise owner, JUnit only.
 * NOT part of the Forge production JAR or live client pipeline.
 *
 * One event-loop owns all retained packet/promise pairs. No JNI, Minecraft
 * World or entity references. No application of native verdicts in runtime.
 */
final class ForgeOutboundPacketHoldOwner extends ChannelDuplexHandler {
    static final int MAX_PENDING=64;
    static final long MAX_AGE_NANOS=150_000_000L;
    interface Clock {long nowNanos();}
    static final class Metrics {
        final long held,delivered,abandoned,failOpen,expired;
        final boolean armed,retired;
        final int pending;
        Metrics(ForgeOutboundPacketHoldOwner o){
            held=o.held;delivered=o.delivered;abandoned=o.abandoned;
            failOpen=o.failOpen;expired=o.expired;
            armed=o.armed;retired=o.retired;pending=o.pending.size();
        }
    }
    private static final class Pending {
        final Packet packet;
        final ChannelPromise promise;
        final long nanos;
        Pending(Packet packet,ChannelPromise promise,long at){
            this.packet=packet;this.promise=promise;nanos=at;
        }
    }
    private final Thread ownerThread=Thread.currentThread();
    private final Clock clock;
    private final ArrayDeque<Pending> pending=new ArrayDeque<Pending>(MAX_PENDING);
    private ChannelHandlerContext context;
    private boolean armed,retired,draining,flushRequested;
    private long held,delivered,abandoned,failOpen,expired,previousNanos;
    ForgeOutboundPacketHoldOwner(Clock clock){
        if(clock==null)throw new IllegalArgumentException("R345 owner clock missing");
        this.clock=clock;
    }
    private void owner(){
        if(Thread.currentThread()!=ownerThread)
            throw new IllegalStateException("R345 outbound owner must run on Netty loop");
        if(context!=null&&!context.executor().inEventLoop())
            throw new IllegalStateException("R345 foreign Netty loop");
    }
    @Override public void handlerAdded(ChannelHandlerContext ctx){
        owner();
        if(context!=null)throw new IllegalStateException("R345 duplicate install");
        context=ctx;
    }
    void arm(){
        owner();
        if(retired||context==null||!context.channel().isActive())
            throw new IllegalStateException("R345 owner detached or not active");
        armed=true;
    }
    void disarm(){
        owner();
        if(retired)return;
        armed=false;
        releaseAll();
    }
    private long timestamp(){
        long t=clock.nowNanos();
        if(previousNanos!=0&&t<previousNanos)
            throw new IllegalStateException("R345 source clock regression");
        previousNanos=t;
        return t;
    }
    /** Must run on the SAME Netty event loop. Real promises retained intact. */
    void releaseDue(){
        owner();
        if(retired||pending.isEmpty())return;
        final long t;
        try{t=timestamp();}
        catch(RuntimeException rollback){
            failOpen++;armed=false;releaseAll();return;
        }
        final long age=t-pending.peekFirst().nanos;
        if(age<0||age>=MAX_AGE_NANOS){
            expired+=age>=MAX_AGE_NANOS?pending.size():0;
            failOpen++;armed=false;releaseAll();
        }
    }
    @Override public void write(ChannelHandlerContext ctx,Object msg,ChannelPromise promise){
        owner();
        if(retired||!armed||!(msg instanceof Packet)){
            if(!pending.isEmpty())releaseAll();
            ctx.write(msg,promise);
            return;
        }
        final boolean movement=msg instanceof C03PacketPlayer;
        if(pending.isEmpty()&&!movement){
            ctx.write(msg,promise);return;
        }
        final long time;
        try{time=timestamp();}
        catch(RuntimeException rollback){
            failOpen++;armed=false;releaseAll();
            ctx.write(msg,promise);return;
        }
        if(!pending.isEmpty()){
            long age=time-pending.peekFirst().nanos;
            if(age<0||age>=MAX_AGE_NANOS){
                expired+=age>=MAX_AGE_NANOS?pending.size():0;
                failOpen++;armed=false;releaseAll();
                ctx.write(msg,promise);return;
            }
        }
        if(pending.size()==MAX_PENDING){
            failOpen++;armed=false;releaseAll();
            ctx.write(msg,promise);return;
        }
        pending.addLast(new Pending((Packet)msg,promise,time));
        ++held;
    }
    @Override public void flush(ChannelHandlerContext ctx){
        owner();
        if(!pending.isEmpty()){
            // A flush must not accidentally bypass an earlier held write.
            flushRequested=true;
            return;
        }
        ctx.flush();
    }
    private void releaseAll(){
        owner();
        if(context==null||pending.isEmpty())return;
        if(draining)throw new IllegalStateException("R345 recursive flush/release");
        draining=true;
        try{
            while(!pending.isEmpty()){
                Pending p=pending.removeFirst();
                // EXACT original ChannelPromise and Packet refs.
                context.write(p.packet,p.promise);
                ++delivered;
            }
            if(flushRequested){
                flushRequested=false;
                context.flush();
            }
        }finally{draining=false;}
    }
    private void failUndeliverable(){
        owner();
        armed=false;
        while(!pending.isEmpty()){
            Pending p=pending.removeFirst();
            ReferenceCountUtil.release(p.packet);
            p.promise.tryFailure(new ClosedChannelException());
            ++abandoned;
        }
        flushRequested=false;
    }
    @Override public void close(ChannelHandlerContext ctx,ChannelPromise promise){
        owner();
        retired=true;failUndeliverable();
        ctx.close(promise);
    }
    @Override public void disconnect(ChannelHandlerContext ctx,ChannelPromise promise){
        owner();
        retired=true;failUndeliverable();
        ctx.disconnect(promise);
    }
    @Override public void channelInactive(ChannelHandlerContext ctx)throws Exception {
        owner();
        retired=true;failUndeliverable();
        ctx.fireChannelInactive();
    }
    @Override public void handlerRemoved(ChannelHandlerContext ctx)throws Exception {
        owner();
        if(!retired&&ctx.channel().isActive())
            releaseAll();
        else failUndeliverable();
        armed=false;retired=true;context=null;
    }
    Metrics metrics(){owner();return new Metrics(this);}
}
