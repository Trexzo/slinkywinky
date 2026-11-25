package dev.slinkywinky.forge;

import dev.slinkywinky.SourcePacketReadOnlyQueue;
import io.netty.channel.ChannelPromise;
import io.netty.channel.embedded.EmbeddedChannel;
import net.minecraft.network.play.client.C03PacketPlayer;
import net.minecraft.network.play.client.C16PacketClientStatus;
import net.minecraft.network.play.server.S06PacketUpdateHealth;
import net.minecraft.network.play.server.S19PacketEntityStatus;
import org.junit.Assert;
import org.junit.Test;

/** R340: actual Netty handler, same Packet identity/promise and overflow. */
public final class ForgeSourcePacketTapEmbeddedTest {
    @Test public void originalPacketsPassUnmodifiedInBothDirections() {
        SourcePacketReadOnlyQueue ingress=new SourcePacketReadOnlyQueue();
        EmbeddedChannel channel=new EmbeddedChannel(
            ForgeSourcePacketTap.diagnosticObserver(ingress));
        try{
            S06PacketUpdateHealth inbound=new S06PacketUpdateHealth();
            Assert.assertTrue(channel.writeInbound(inbound));
            Assert.assertSame("Inbound original object",inbound,channel.readInbound());
            Assert.assertNull("Never duplicated",channel.readInbound());

            C03PacketPlayer outbound=new C03PacketPlayer(true);
            ChannelPromise promise=channel.newPromise();
            channel.pipeline().writeAndFlush(outbound,promise);
            channel.runPendingTasks();
            Assert.assertTrue("Same original outbound promise completed",promise.isSuccess());
            Assert.assertSame("Outbound original object",outbound,channel.readOutbound());
            Assert.assertNull("Never duplicated",channel.readOutbound());

            SourcePacketReadOnlyQueue.Batch batch=ingress.drain(16);
            Assert.assertEquals(2,batch.rows.size());
            Assert.assertEquals(SourcePacketReadOnlyQueue.IN,batch.rows.get(0).direction);
            Assert.assertEquals(SourcePacketReadOnlyQueue.UPDATE_HEALTH,batch.rows.get(0).kind);
            Assert.assertEquals(SourcePacketReadOnlyQueue.OUT,batch.rows.get(1).direction);
            Assert.assertEquals(SourcePacketReadOnlyQueue.MOVEMENT,batch.rows.get(1).kind);
            Assert.assertEquals(1,batch.rows.get(0).sequence);
            Assert.assertEquals(2,batch.rows.get(1).sequence);
            Assert.assertFalse("Ground-only packet is not position-bearing",batch.rows.get(1).moving);
        }finally{
            channel.finish();
            ingress.close();
        }
    }

    @Test public void unknownEntityIdNeverInventedOnNettyThread(){
        SourcePacketReadOnlyQueue q=new SourcePacketReadOnlyQueue();
        EmbeddedChannel channel=new EmbeddedChannel(
            ForgeSourcePacketTap.diagnosticObserver(q));
        try{
            S19PacketEntityStatus status=new S19PacketEntityStatus();
            Assert.assertTrue(channel.writeInbound(status));
            Assert.assertSame(status,channel.readInbound());
            SourcePacketReadOnlyQueue.Batch batch=q.drain(16);
            Assert.assertEquals(1,batch.rows.size());
            Assert.assertEquals(SourcePacketReadOnlyQueue.ENTITY_STATUS,
                                batch.rows.get(0).kind);
            Assert.assertFalse("Original entity ID unavailable on Netty",
                               batch.rows.get(0).entityIdKnown);
        }finally{
            channel.finish();
            q.close();
        }
    }

    @Test public void overflowNeverBlocksOrSwallowsRealPackets(){
        SourcePacketReadOnlyQueue q=new SourcePacketReadOnlyQueue();
        EmbeddedChannel channel=new EmbeddedChannel(
            ForgeSourcePacketTap.diagnosticObserver(q));
        try{
            for(int i=0;i<=SourcePacketReadOnlyQueue.CAPACITY;++i){
                S06PacketUpdateHealth p=new S06PacketUpdateHealth();
                Assert.assertTrue(channel.writeInbound(p));
                Assert.assertSame("Even post-overflow original packet forwarded",
                                  p,channel.readInbound());
            }
            Assert.assertFalse("Observation failed closed",q.healthy());
            Assert.assertEquals("Partial source stream discarded",0,q.pending());
            S06PacketUpdateHealth after=new S06PacketUpdateHealth();
            Assert.assertTrue(channel.writeInbound(after));
            Assert.assertSame("Packet flow survives observer poisoning",
                              after,channel.readInbound());
            Assert.assertTrue("Channel itself stays open",channel.isOpen());
        }finally{
            channel.finish();
            q.close();
        }
    }

    @Test public void inventoryClassDoesNotMutateOriginalPacket(){
        SourcePacketReadOnlyQueue q=new SourcePacketReadOnlyQueue();
        EmbeddedChannel channel=new EmbeddedChannel(
            ForgeSourcePacketTap.diagnosticObserver(q));
        try{
            C16PacketClientStatus open=new C16PacketClientStatus(
                C16PacketClientStatus.EnumState.OPEN_INVENTORY_ACHIEVEMENT);
            Assert.assertTrue(channel.writeOutbound(open));
            Assert.assertSame(open,channel.readOutbound());
            SourcePacketReadOnlyQueue.Batch batch=q.drain(16);
            Assert.assertEquals(1,batch.rows.size());
            Assert.assertEquals(SourcePacketReadOnlyQueue.INV_OPEN,batch.rows.get(0).kind);
        }finally{
            channel.finish();
            q.close();
        }
    }
}
