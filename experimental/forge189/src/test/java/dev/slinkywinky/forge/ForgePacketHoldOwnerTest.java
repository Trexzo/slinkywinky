package dev.slinkywinky.forge;

import io.netty.channel.embedded.EmbeddedChannel;
import net.minecraft.network.Packet;
import net.minecraft.network.play.server.S06PacketUpdateHealth;
import net.minecraft.network.play.server.S18PacketEntityTeleport;
import org.junit.Assert;
import org.junit.Test;

/**
 * R344: real Minecraft 1.8.9 packet objects, actual Netty FIFO hold/release,
 * all in the JUnit test source set. NO handler is packaged in a live JAR.
 */
public final class ForgePacketHoldOwnerTest {
    private static final class ManualClock implements ForgePacketHoldOwner.Clock {
        long time=1_000_000_000L;
        public long nowNanos(){return time;}
    }
    private static S18PacketEntityTeleport teleport(int entity,int x){
        return new S18PacketEntityTeleport(
            entity,x,640,64,(byte)0,(byte)0,true);
    }
    private static EmbeddedChannel channel(ForgePacketHoldOwner owner){
        return new EmbeddedChannel(owner);
    }
    @Test public void packetAdmissionIsUnarmedByDefault(){
        ManualClock clock=new ManualClock();
        ForgePacketHoldOwner holder=new ForgePacketHoldOwner(clock);
        EmbeddedChannel net=channel(holder);
        try{
            Packet packet=teleport(71,320);
            Assert.assertTrue(net.writeInbound(packet));
            Assert.assertSame(packet,net.readInbound());
            Assert.assertNull(net.readInbound());
            ForgePacketHoldOwner.Metrics m=holder.metrics();
            Assert.assertFalse(m.armed);
            Assert.assertEquals(0,m.pending);
            Assert.assertEquals(0,m.accepted);
        }finally{net.finish();}
    }
    @Test public void selectedTeleportAndSubsequentPacketsReleaseInFIFO(){
        ManualClock clock=new ManualClock();
        ForgePacketHoldOwner holder=new ForgePacketHoldOwner(clock);
        EmbeddedChannel net=channel(holder);
        try{
            holder.arm(71);
            Packet selected=teleport(71,320);
            Packet later=new S06PacketUpdateHealth();
            Assert.assertFalse(net.writeInbound(selected));
            Assert.assertFalse(net.writeInbound(later));
            Assert.assertEquals(2,holder.metrics().pending);
            Assert.assertNull("A real Netty packet is actually withheld",
                              net.readInbound());
            holder.disarm();
            Assert.assertSame(selected,net.readInbound());
            Assert.assertSame(later,net.readInbound());
            Assert.assertNull("No double delivery",net.readInbound());
            ForgePacketHoldOwner.Metrics m=holder.metrics();
            Assert.assertFalse(m.armed);
            Assert.assertEquals(2,m.accepted);
            Assert.assertEquals(2,m.replayed);
            Assert.assertEquals(0,m.pending);
            Packet newPacket=teleport(71,352);
            Assert.assertTrue(net.writeInbound(newPacket));
            Assert.assertSame(newPacket,net.readInbound());
        }finally{net.finish();}
    }
    @Test public void wrongTargetNeverIntercepted(){
        ManualClock clock=new ManualClock();
        ForgePacketHoldOwner holder=new ForgePacketHoldOwner(clock);
        EmbeddedChannel net=channel(holder);
        try{
            holder.arm(71);
            Packet unrelated=teleport(72,512);
            Assert.assertTrue(net.writeInbound(unrelated));
            Assert.assertSame(unrelated,net.readInbound());
            Assert.assertEquals(0,holder.metrics().accepted);
            Assert.assertEquals(0,holder.metrics().pending);
        }finally{net.finish();}
    }
    @Test public void explicitExpiryReleasesHeldPacketAndDisarms(){
        ManualClock clock=new ManualClock();
        ForgePacketHoldOwner holder=new ForgePacketHoldOwner(clock);
        EmbeddedChannel net=channel(holder);
        try{
            holder.arm(71);
            Packet first=teleport(71,320);
            Packet second=teleport(71,352);
            Assert.assertFalse(net.writeInbound(first));
            clock.time+=ForgePacketHoldOwner.MAX_AGE_NANOS/2;
            Assert.assertFalse(net.writeInbound(second));
            holder.releaseDue();
            Assert.assertEquals(2,holder.metrics().pending);
            Assert.assertNull(net.readInbound());
            clock.time+=ForgePacketHoldOwner.MAX_AGE_NANOS/2+1;
            holder.releaseDue();
            Assert.assertSame(first,net.readInbound());
            Assert.assertSame(second,net.readInbound());
            Assert.assertNull(net.readInbound());
            Assert.assertEquals(2,holder.metrics().expired);
            Assert.assertEquals(2,holder.metrics().replayed);
            Assert.assertFalse(holder.metrics().armed);
        }finally{net.finish();}
    }
    @Test public void overflowFlushesAllInOrderAndFailsOpen(){
        ManualClock clock=new ManualClock();
        ForgePacketHoldOwner holder=new ForgePacketHoldOwner(clock);
        EmbeddedChannel net=channel(holder);
        try{
            holder.arm(71);
            Packet[] original=new Packet[ForgePacketHoldOwner.MAX_PENDING+1];
            for(int i=0;i<original.length;++i){
                original[i]=i==0?teleport(71,320):new S06PacketUpdateHealth();
                Assert.assertFalse(net.writeInbound(original[i])&&i<original.length-1);
            }
            ForgePacketHoldOwner.Metrics m=holder.metrics();
            Assert.assertEquals(ForgePacketHoldOwner.MAX_PENDING,m.accepted);
            Assert.assertEquals(ForgePacketHoldOwner.MAX_PENDING,m.replayed);
            Assert.assertEquals(1,m.failOpen);
            Assert.assertEquals(0,m.pending);
            Assert.assertFalse(m.armed);
            for(Packet expected:original)
                Assert.assertSame("Overflow releases old before new",expected,net.readInbound());
            Assert.assertNull(net.readInbound());
            Packet next=teleport(71,384);
            Assert.assertTrue(net.writeInbound(next));
            Assert.assertSame(next,net.readInbound());
        }finally{net.finish();}
    }
    @Test public void handlerRemovalReplaysBeforeDetaching(){
        ManualClock clock=new ManualClock();
        ForgePacketHoldOwner holder=new ForgePacketHoldOwner(clock);
        EmbeddedChannel net=channel(holder);
        try{
            holder.arm(71);
            Packet held=teleport(71,320);
            Assert.assertFalse(net.writeInbound(held));
            net.pipeline().remove(holder);
            Assert.assertSame("No packet lost on removal",held,net.readInbound());
            Assert.assertNull(net.readInbound());
            Assert.assertEquals(1,holder.metrics().replayed);
            Assert.assertTrue(holder.metrics().retired);
        }finally{net.finish();}
    }
    @Test public void disconnectAbandonsUndeliverablePacketReferences(){
        ManualClock clock=new ManualClock();
        ForgePacketHoldOwner holder=new ForgePacketHoldOwner(clock);
        EmbeddedChannel net=channel(holder);
        holder.arm(71);
        Assert.assertFalse(net.writeInbound(teleport(71,320)));
        // EmbeddedChannel in the bundled Netty 4.0.x may defer inactive
        // notifications during close(). Drive the exact handler lifecycle
        // callback explicitly before checking drop/release semantics.
        net.pipeline().fireChannelInactive();
        net.close();
        net.runPendingTasks();
        ForgePacketHoldOwner.Metrics m=holder.metrics();
        Assert.assertEquals(1,m.accepted);
        Assert.assertEquals(0,m.pending);
        Assert.assertEquals(1,m.abandoned);
        Assert.assertTrue(m.retired);
        Assert.assertNull("No replay into a dead world",net.readInbound());
        net.finish();
    }
    @Test public void foreignThreadCannotChangeEventLoopAdmission()throws Exception{
        ManualClock clock=new ManualClock();
        final ForgePacketHoldOwner holder=new ForgePacketHoldOwner(clock);
        EmbeddedChannel net=channel(holder);
        try{
            final boolean[] denied={false};
            Thread foreign=new Thread(()->{
                try{holder.arm(71);}
                catch(IllegalStateException expected){denied[0]=true;}
            });
            foreign.start();foreign.join();
            Assert.assertTrue("Netty owner-thread rule enforced",denied[0]);
            Assert.assertFalse(holder.metrics().armed);
            holder.arm(71);
            Assert.assertTrue(holder.metrics().armed);
        }finally{net.finish();}
    }
}
