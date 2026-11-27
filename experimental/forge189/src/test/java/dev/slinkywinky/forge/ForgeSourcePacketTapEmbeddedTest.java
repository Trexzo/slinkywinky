package dev.slinkywinky.forge;

import dev.slinkywinky.SourcePacketReadOnlyQueue;
import io.netty.channel.ChannelPromise;
import io.netty.channel.embedded.EmbeddedChannel;
import net.minecraft.network.play.client.C03PacketPlayer;
import net.minecraft.network.play.client.C16PacketClientStatus;
import net.minecraft.network.play.server.S06PacketUpdateHealth;
import net.minecraft.network.play.server.S19PacketEntityStatus;
import net.minecraft.network.play.server.S18PacketEntityTeleport;
import net.minecraft.network.play.server.S14PacketEntity;
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

    @Test public void originalS19EntityIdCapturedWithoutWorldAndPacketRetained()
            throws Exception{
        // Recreate a decoded private S19 field in a disposable packet.
        // Candidate rdata.s names: entityId (MCP), field_149164_a (SRG), a (obf).
        final S19PacketEntityStatus status=new S19PacketEntityStatus();
        java.lang.reflect.Field idField=null;
        for(String name:new String[]{"entityId","field_149164_a","a"}){
            try{
                final java.lang.reflect.Field candidate=
                    S19PacketEntityStatus.class.getDeclaredField(name);
                if(candidate.getType()==Integer.TYPE){
                    idField=candidate;
                    break;
                }
            }catch(NoSuchFieldException missing){/* next known mapping */}
        }
        Assert.assertNotNull("Original candidate S19 int field must resolve",idField);
        idField.setAccessible(true);
        idField.setInt(status,21);
        final SourcePacketReadOnlyQueue q=new SourcePacketReadOnlyQueue();
        final EmbeddedChannel channel=new EmbeddedChannel(
            ForgeSourcePacketTap.diagnosticObserver(q));
        try{
            Assert.assertTrue(channel.writeInbound(status));
            Assert.assertSame("Same original S19 Packet",status,channel.readInbound());
            Assert.assertNull("Never duplicated",channel.readInbound());
            SourcePacketReadOnlyQueue.Batch b=q.drain(4);
            Assert.assertEquals(1,b.rows.size());
            SourcePacketReadOnlyQueue.Observation o=b.rows.get(0);
            Assert.assertEquals(SourcePacketReadOnlyQueue.ENTITY_STATUS,o.kind);
            Assert.assertTrue("Exact candidate direct integer ID",o.entityIdKnown);
            Assert.assertEquals(21,o.entityId);
            Assert.assertEquals(1,SourcePacketReadOnlyQueue.sourceBacktrackKind(o));
            Assert.assertTrue("Observer never interrupts channel",channel.isOpen());
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

    @Test public void exactTeleportFixedPointAndPacketIdentity(){
        SourcePacketReadOnlyQueue q=new SourcePacketReadOnlyQueue();
        EmbeddedChannel channel=new EmbeddedChannel(
            ForgeSourcePacketTap.diagnosticObserver(q));
        try{
            S18PacketEntityTeleport teleport=new S18PacketEntityTeleport(
                71,320,640,-64,(byte)64,(byte)0,true);
            Assert.assertTrue(channel.writeInbound(teleport));
            Assert.assertSame("Exact same packet object forwarded",
                              teleport,channel.readInbound());
            SourcePacketReadOnlyQueue.Batch b=q.drain(16);
            Assert.assertEquals(1,b.rows.size());
            SourcePacketReadOnlyQueue.Observation o=b.rows.get(0);
            Assert.assertEquals(SourcePacketReadOnlyQueue.ENTITY_ABSOLUTE_TELEPORT,o.kind);
            Assert.assertTrue(o.entityIdKnown);
            Assert.assertEquals(71,o.entityId);
            Assert.assertTrue(o.moving);
            Assert.assertEquals(10.0,o.x,0.0);
            Assert.assertEquals(20.0,o.y,0.0);
            Assert.assertEquals(-2.0,o.z,0.0);
        }finally{
            channel.finish();
            q.close();
        }
    }

    @Test public void relativeMovementHasConservativeUnknownEntityBarrier(){
        SourcePacketReadOnlyQueue q=new SourcePacketReadOnlyQueue();
        EmbeddedChannel channel=new EmbeddedChannel(
            ForgeSourcePacketTap.diagnosticObserver(q));
        try{
            S14PacketEntity.S15PacketEntityRelMove relative=
                new S14PacketEntity.S15PacketEntityRelMove(
                    71,(byte)32,(byte)0,(byte)-16,false);
            Assert.assertTrue(channel.writeInbound(relative));
            Assert.assertSame("Relative packet untouched",
                              relative,channel.readInbound());
            SourcePacketReadOnlyQueue.Batch b=q.drain(16);
            Assert.assertEquals(1,b.rows.size());
            SourcePacketReadOnlyQueue.Observation o=b.rows.get(0);
            Assert.assertEquals(SourcePacketReadOnlyQueue.RELATIVE_UNRESOLVED,o.kind);
            Assert.assertFalse("S14 has no public getEntityId in 1.8.9",
                               o.entityIdKnown);
            Assert.assertFalse(o.moving);
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
