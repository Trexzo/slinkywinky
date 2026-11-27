package dev.slinkywinky.forge;

import io.netty.channel.ChannelPromise;
import io.netty.channel.embedded.EmbeddedChannel;
import net.minecraft.network.Packet;
import net.minecraft.network.play.client.C03PacketPlayer;
import net.minecraft.network.play.client.C16PacketClientStatus;
import org.junit.Assert;
import org.junit.Test;

/** R345: actual Netty outbound packet and original promise ownership. */
public final class ForgeOutboundPacketHoldOwnerTest {
    private static final class ManualClock implements ForgeOutboundPacketHoldOwner.Clock {
        long time=1_000_000_000L;
        public long nowNanos(){return time;}
    }
    private static Packet movement(){return new C03PacketPlayer(true);}
    private static Packet inventory(){
        return new C16PacketClientStatus(
            C16PacketClientStatus.EnumState.OPEN_INVENTORY_ACHIEVEMENT);
    }
    private static ChannelPromise write(EmbeddedChannel c,Object packet){
        ChannelPromise p=c.newPromise();
        c.pipeline().writeAndFlush(packet,p);
        c.runPendingTasks();
        return p;
    }
    @Test public void defaultUnarmedForwardsAndCompletesPromise(){
        ManualClock clock=new ManualClock();
        ForgeOutboundPacketHoldOwner owner=new ForgeOutboundPacketHoldOwner(clock);
        EmbeddedChannel net=new EmbeddedChannel(owner);
        try{
            Packet packet=movement();
            ChannelPromise promise=write(net,packet);
            Assert.assertTrue(promise.isSuccess());
            Assert.assertSame(packet,net.readOutbound());
            Assert.assertNull(net.readOutbound());
            Assert.assertFalse(owner.metrics().armed);
            Assert.assertEquals(0,owner.metrics().pending);
        }finally{net.finish();}
    }
    @Test public void originalTwoPacketsAndPromisesRetainedUntilDisarm(){
        ManualClock clock=new ManualClock();
        ForgeOutboundPacketHoldOwner owner=new ForgeOutboundPacketHoldOwner(clock);
        EmbeddedChannel net=new EmbeddedChannel(owner);
        try{
            owner.arm();
            Packet p1=movement(),p2=inventory();
            ChannelPromise first=write(net,p1);
            ChannelPromise second=write(net,p2);
            Assert.assertFalse(first.isDone());
            Assert.assertFalse(second.isDone());
            Assert.assertNull("A real outgoing Packet is withheld",net.readOutbound());
            Assert.assertEquals(2,owner.metrics().pending);
            owner.disarm();
            net.runPendingTasks();
            Assert.assertTrue("Exact first promise completed",first.isSuccess());
            Assert.assertTrue("Exact second promise completed",second.isSuccess());
            Assert.assertSame(p1,net.readOutbound());
            Assert.assertSame(p2,net.readOutbound());
            Assert.assertNull("No double send",net.readOutbound());
            Assert.assertEquals(2,owner.metrics().delivered);
        }finally{net.finish();}
    }
    @Test public void nonMovementPassesWhenNoHeldWrite(){
        ManualClock clock=new ManualClock();
        ForgeOutboundPacketHoldOwner owner=new ForgeOutboundPacketHoldOwner(clock);
        EmbeddedChannel net=new EmbeddedChannel(owner);
        try{
            owner.arm();
            Packet other=inventory();
            ChannelPromise p=write(net,other);
            Assert.assertTrue(p.isSuccess());
            Assert.assertSame(other,net.readOutbound());
            Assert.assertEquals(0,owner.metrics().held);
        }finally{net.finish();}
    }
    @Test public void monotonicTimeoutReplaysFIFOAndEndsAdmission(){
        ManualClock clock=new ManualClock();
        ForgeOutboundPacketHoldOwner owner=new ForgeOutboundPacketHoldOwner(clock);
        EmbeddedChannel net=new EmbeddedChannel(owner);
        try{
            owner.arm();
            Packet first=movement();
            ChannelPromise p=write(net,first);
            clock.time+=ForgeOutboundPacketHoldOwner.MAX_AGE_NANOS/2;
            Packet second=inventory();
            ChannelPromise q=write(net,second);
            owner.releaseDue();
            Assert.assertFalse(p.isDone());
            Assert.assertFalse(q.isDone());
            clock.time+=ForgeOutboundPacketHoldOwner.MAX_AGE_NANOS/2+1;
            owner.releaseDue();
            net.runPendingTasks();
            Assert.assertTrue(p.isSuccess());
            Assert.assertTrue(q.isSuccess());
            Assert.assertSame(first,net.readOutbound());
            Assert.assertSame(second,net.readOutbound());
            Assert.assertFalse(owner.metrics().armed);
            Assert.assertEquals(2,owner.metrics().expired);
        }finally{net.finish();}
    }
    @Test public void overflowFlushesBeforeNewWriteAndFulfillsAllPromises(){
        ManualClock clock=new ManualClock();
        ForgeOutboundPacketHoldOwner owner=new ForgeOutboundPacketHoldOwner(clock);
        EmbeddedChannel net=new EmbeddedChannel(owner);
        try{
            owner.arm();
            final int size=ForgeOutboundPacketHoldOwner.MAX_PENDING+1;
            Packet[] packet=new Packet[size];
            ChannelPromise[] promise=new ChannelPromise[size];
            for(int i=0;i<size;++i){
                packet[i]=i==0?movement():inventory();
                promise[i]=write(net,packet[i]);
                if(i<size-1)Assert.assertFalse(promise[i].isDone());
            }
            Assert.assertEquals(1,owner.metrics().failOpen);
            Assert.assertEquals(0,owner.metrics().pending);
            Assert.assertFalse(owner.metrics().armed);
            Assert.assertEquals(size-1,owner.metrics().delivered);
            for(int i=0;i<size;++i){
                Assert.assertTrue("All original promises completed",promise[i].isSuccess());
                Assert.assertSame("All messages delivered FIFO",packet[i],net.readOutbound());
            }
            Assert.assertNull(net.readOutbound());
        }finally{net.finish();}
    }
    @Test public void liveHandlerRemovalReplaysPacketAndOriginalPromise(){
        ManualClock clock=new ManualClock();
        ForgeOutboundPacketHoldOwner owner=new ForgeOutboundPacketHoldOwner(clock);
        EmbeddedChannel net=new EmbeddedChannel(owner);
        try{
            owner.arm();
            Packet packet=movement();
            ChannelPromise promise=write(net,packet);
            Assert.assertFalse(promise.isDone());
            net.pipeline().remove(owner);
            net.runPendingTasks();
            Assert.assertTrue(promise.isSuccess());
            Assert.assertSame(packet,net.readOutbound());
            Assert.assertTrue(owner.metrics().retired);
        }finally{net.finish();}
    }
    @Test public void closeFailsPendingPromiseWithoutWritingToDeadChannel(){
        ManualClock clock=new ManualClock();
        ForgeOutboundPacketHoldOwner owner=new ForgeOutboundPacketHoldOwner(clock);
        EmbeddedChannel net=new EmbeddedChannel(owner);
        owner.arm();
        Packet packet=movement();
        ChannelPromise promise=write(net,packet);
        Assert.assertFalse(promise.isDone());
        net.close();
        net.runPendingTasks();
        Assert.assertTrue("Pending promise completed with failure",promise.isDone());
        Assert.assertFalse(promise.isSuccess());
        Assert.assertEquals(1,owner.metrics().abandoned);
        Assert.assertEquals(0,owner.metrics().pending);
        Assert.assertNull("No write after channel shutdown",net.readOutbound());
        net.finish();
    }
    @Test public void foreignThreadCannotChangeOutboundEventLoop()throws Exception{
        ManualClock clock=new ManualClock();
        final ForgeOutboundPacketHoldOwner owner=
            new ForgeOutboundPacketHoldOwner(clock);
        EmbeddedChannel net=new EmbeddedChannel(owner);
        try{
            final boolean[] denied={false};
            Thread outsider=new Thread(()->{
                try{owner.arm();}catch(IllegalStateException expected){
                    denied[0]=true;
                }
            });
            outsider.start();outsider.join();
            Assert.assertTrue(denied[0]);
            Assert.assertFalse(owner.metrics().armed);
            owner.arm();
            Assert.assertTrue(owner.metrics().armed);
        }finally{net.finish();}
    }
}
