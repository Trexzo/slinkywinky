package dev.slinkywinky;

/** R342 finite client-applied server history and packet provenance audit. */
public final class SourceRemotePositionHistorySmoke {
    static void yes(boolean ok,String message){if(!ok)throw new AssertionError(message);}
    static void eq(int a,int b,String msg){yes(a==b,msg+": "+a+" != "+b);}
    static SourcePacketReadOnlyQueue.Observation packet(int kind,int id,
              int x,int y,int z,long time){
        SourcePacketReadOnlyQueue q=new SourcePacketReadOnlyQueue();
        yes(q.offer(SourcePacketReadOnlyQueue.IN,kind,id,true,true,
                    x/32.0d,y/32.0d,z/32.0d,time),"Absolute packet captured");
        SourcePacketReadOnlyQueue.Observation result=q.drain(1).rows.get(0);
        q.close();return result;
    }
    public static void main(String[] args){
        final long now=System.nanoTime();
        SourceRemotePositionHistory h=new SourceRemotePositionHistory();
        h.applied(21,101,102,320,640,960,now-300_000_000L);
        h.applied(21,101,102,352,640,960,now-180_000_000L);
        h.applied(21,101,102,384,640,960,now-20_000_000L);
        SourceRemotePositionHistory.Historical prior=h.historical(
            21,101,102,now,80_000_000L,600_000_000L);
        yes(prior!=null&&prior.previous.serverX==320&&
            prior.current.serverX==352,"Only actual older serverPos values selected");
        eq((int)(prior.ageNanos/1_000_000),180,"Exact historical age");
        yes(h.historical(21,101,103,now,80_000_000L,600_000_000L)==null,
            "UUID mismatch refuses reused entity ID");
        yes(h.historical(21,101,102,now+1_000_000_000L,
                         80_000_000L,600_000_000L)==null,
            "Old history cannot survive expiry");
        yes(h.historical(21,101,102,now,80_000_000L,600_000_000L)
            .current.serverX!=384,"Never use a sample younger than lookback");

        long t=now+1_000_000L;
        h.packet(packet(SourcePacketReadOnlyQueue.ENTITY_ABSOLUTE_TELEPORT,
                       21,416,640,960,t));
        SourceRemotePositionHistory.Position confirmed=h.applied(
            21,101,102,416,640,960,t+40_000_000L);
        eq(confirmed.source,SourceRemotePositionHistory.PACKET_CONFIRMED,
            "S18 known fixed point confirmed against applied vanilla serverPos");
        yes(h.confirmed()==1&&h.packetCount()==1,
            "Count only genuinely matched packet observation");
        h.packet(packet(SourcePacketReadOnlyQueue.ENTITY_ABSOLUTE_TELEPORT,
                       21,448,640,960,t+42_000_000L));
        SourceRemotePositionHistory.Position notConfirmed=h.applied(
            21,101,102,480,640,960,t+80_000_000L);
        eq(notConfirmed.source,SourceRemotePositionHistory.APPLIED_SERVER_POSITION,
           "Unmatched packet is NEVER called a confirmed server position");
        eq((int)h.confirmed(),1,"Packet mismatch did not confirm");

        SourcePacketReadOnlyQueue q=new SourcePacketReadOnlyQueue();
        yes(q.offer(SourcePacketReadOnlyQueue.IN,
                    SourcePacketReadOnlyQueue.RELATIVE_UNRESOLVED,
                    0,false,false,0,0,0,t+81_000_000L),
            "MCP 1.8.9 S14 relative has unknown public entity ID");
        h.packet(q.drain(1).rows.get(0));q.close();
        eq((int)h.barrierCount(),1,"Unknown relative packet barrier");
        // Packet candidate seen before S14 is invalidated. Historical
        // client-applied samples are still valid: no false orphaning.
        SourceRemotePositionHistory.Position after=h.applied(
            21,101,102,448,640,960,t+82_000_000L);
        eq(after.source,SourceRemotePositionHistory.APPLIED_SERVER_POSITION,
           "Relative packet cannot confirm a previous speculative position");

        SourcePacketReadOnlyQueue.Observation spawn=packet(
            SourcePacketReadOnlyQueue.PLAYER_SPAWN_ABSOLUTE,21,512,640,960,
            t+90_000_000L);
        h.packet(spawn);
        yes(h.historical(21,101,102,t+500_000_000L,
                         80_000_000L,600_000_000L)==null,
            "Spawn clears historical entity-ID owner before client UUID admission");
        SourceRemotePositionHistory.Position fresh=h.applied(
            21,555,666,512,640,960,t+100_000_000L);
        eq(fresh.source,SourceRemotePositionHistory.PACKET_CONFIRMED,
            "Spawn only confirms after new UUID client-applied serverPos matches");
        yes(h.historical(21,101,102,t+400_000_000L,
                         80_000_000L,600_000_000L)==null,
            "Old entity UUID cannot see a new spawn track");

        final long oldEpoch=h.epoch();
        h.resetWorld();
        yes(h.epoch()==oldEpoch+1&&h.tracked()==0,
            "World generation clears all remote coordinates and UUIDs");
        for(int i=0;i<SourceRemotePositionHistory.MAX_ENTITIES+40;++i)
            h.applied(i,1,2,i*32,0,0,t+200_000_000L);
        eq(h.tracked(),SourceRemotePositionHistory.MAX_ENTITIES,
            "Entity churn bounded to fixed LRU history cap");

        SourceRemotePositionHistory negativeClock=new SourceRemotePositionHistory();
        negativeClock.applied(1,2,3,32,64,96,-100_000_000L);
        yes(negativeClock.historical(1,2,3,0L,80_000_000L,600_000_000L)!=null,
            "System.nanoTime signed origin accepted");
        negativeClock.retire();
        h.retire();
        boolean denied=false;
        try{h.applied(3,1,2,0,0,0,now);}catch(IllegalStateException expected){
            denied=true;
        }
        yes(denied,"Retired history rejects further world observations");
        System.out.println("R342_VANILLA_SERVERPOS_HISTORY_AND_PACKET_PROVENANCE_PASS");
    }
}
