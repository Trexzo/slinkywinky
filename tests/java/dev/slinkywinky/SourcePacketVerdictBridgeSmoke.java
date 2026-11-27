package dev.slinkywinky;

/**
 * R347: REAL native Backtrack e2 and Lag Range e3 packet verdicts cross
 * Minecraft client -> foreign Netty reader without packet references/actions.
 */
public final class SourcePacketVerdictBridgeSmoke {
    static void yes(boolean value,String what){if(!value)throw new AssertionError(what);}
    static void denied(Runnable action,String what){
        boolean refused=false;
        try{action.run();}
        catch(IllegalArgumentException|IllegalStateException expected){refused=true;}
        yes(refused,what);
    }
    static SourceWorldReadOnlyHost.Frame activeFrame(){
        // Unlike the actual live Forge (source reach unknown = 0), this
        // synthetic offline native test explicitly provides reach=1.
        return new SourceWorldReadOnlyHost.Frame(
            new int[][]{
                {1,1,1,100,90,21,42,1,1,0,0,1},
                {1,1,1,0,0,0,0,0,0,0,0,0},
                {1,1,1,1,1,0,0,1,0,1,0,1,1,1,0},
                {1,0,0,0,0,1,0,0,0,0,1,1}
            },
            new double[][]{
                {9,0,120,3,0,6,10},
                {9,10,10,0,0,150},
                {9,0,1,5,3,2,0.5},
                {4}
            });
    }
    static SourcePacketReadOnlyQueue.Observation fact(int direction,int kind,
            int entity,boolean known,boolean moving,double x,long at){
        SourcePacketReadOnlyQueue ingress=new SourcePacketReadOnlyQueue();
        yes(ingress.offer(direction,kind,entity,known,moving,x,0,0,at),
            "Raw immutable network observation accepted");
        SourcePacketReadOnlyQueue.Observation observation=
            ingress.drain(1).rows.get(0);
        ingress.close();
        return observation;
    }
    public static void main(String[] args)throws Exception{
        SourceRuntimePreview source=new SourceRuntimePreview();
        SourceWorldReadOnlyHost host=new SourceWorldReadOnlyHost(source);
        SourceWorldReadOnlyHostSmoke.Port port=new SourceWorldReadOnlyHostSmoke.Port();
        port.current=activeFrame();
        SourceWorldReadOnlyHost.TickReceipt tick=host.observe(port);
        yes(tick.backtrack.at(3)==1&&tick.lagRange.at(3)==1,
            "REAL JNI source controllers are active in explicit offline fixture");

        SourcePacketVerdictBridge bridge=new SourcePacketVerdictBridge(source);
        long channel=711;
        long at=System.nanoTime();
        bridge.begin(channel,tick);
        SourcePacketReadOnlyQueue stream=new SourcePacketReadOnlyQueue();
        Thread nettyProducer=new Thread(()->{
            yes(stream.offer(SourcePacketReadOnlyQueue.IN,
                SourcePacketReadOnlyQueue.UNKNOWN,0,false,false,0,0,0,at),
                "Backtrack native packet event 2 input");
            yes(stream.offer(SourcePacketReadOnlyQueue.IN,
                SourcePacketReadOnlyQueue.ENTITY_STATUS,21,true,false,0,0,0,at+1),
                "Known target status feedback event");
            yes(stream.offer(SourcePacketReadOnlyQueue.OUT,
                SourcePacketReadOnlyQueue.MOVEMENT,0,false,true,9,0,0,at+2),
                "Lag Range native packet event 3 input");
        });
        nettyProducer.start();nettyProducer.join();
        SourcePacketReadOnlyQueue.Batch observations=stream.drain(8);
        yes(observations.rows.size()==3,"Real Netty sequence FIFO 1,2,3");
        SourcePacketVerdictBridge.Decision b0=bridge.observe(observations.rows.get(0),
            at+5_000_000L);
        SourcePacketVerdictBridge.Decision b1=bridge.observe(observations.rows.get(1),
            at+5_000_001L);
        SourcePacketVerdictBridge.Decision l0=bridge.observe(observations.rows.get(2),
            at+5_000_002L);
        yes(b0.sourceEvent==2&&b0.packetSequence==1&&
            b0.nativeVerdict==SourcePacketVerdictBridge.BACKTRACK_HOLD&&
            b0.hypotheticalHold(),
            "Native Backtrack e2 genuinely returned a packet hold proposal");
        yes(b1.sourceEvent==2&&b1.packetSequence==2&&
            b1.nativeVerdict==SourcePacketVerdictBridge.BACKTRACK_FEEDBACK&&
            !b1.hypotheticalHold(),
            "Native Backtrack e2 pass-through for the exact target status");
        yes(l0.sourceEvent==3&&l0.packetSequence==3&&
            l0.nativeVerdict==SourcePacketVerdictBridge.LAG_RANGE_HOLD&&
            l0.hypotheticalHold(),
            "Native Lag Range e3 genuinely returned packet hold proposal");

        final boolean[] foreignWriterRefused={false};
        final SourcePacketVerdictBridge.Decision[] nettyReceived=
            new SourcePacketVerdictBridge.Decision[3];
        Thread nettyConsumer=new Thread(()->{
            yes(bridge.consume(channel+1,b0.worldToken,b0.generation,1,
                               at+6_000_000L)==null,
                "Foreign channel never receives valid native verdict");
            yes(bridge.consume(channel,b0.worldToken+1,b0.generation,1,
                               at+6_000_000L)==null,
                "Cross-world packet verdict not transferred");
            yes(bridge.consume(channel,b0.worldToken,b0.generation+1,1,
                               at+6_000_000L)==null,
                "Cross-generation verdict not transferred");
            for(int i=0;i<3;++i)
                nettyReceived[i]=bridge.consume(channel,b0.worldToken,b0.generation,
                                                i+1,at+7_000_000L+i);
            try{bridge.observe(observations.rows.get(0),at+8_000_000L);}
            catch(IllegalStateException expected){foreignWriterRefused[0]=true;}
        });
        nettyConsumer.start();nettyConsumer.join();
        yes(nettyReceived[0]==b0&&nettyReceived[1]==b1&&nettyReceived[2]==l0,
            "Foreign Netty thread receives exact JNI receipts in packet order");
        yes(foreignWriterRefused[0],"Netty cannot call source JNI from foreign thread");
        yes(bridge.pending()==0&&bridge.consumed()==3&&bridge.produced()==3,
            "Single-shot FIFO decision delivery, no reuse");
        yes(bridge.consume(channel,b0.worldToken,b0.generation,1,
                           at+8_000_000L)==null,
            "A consumed native packet verdict cannot be duplicated");

        // Producer is not permitted to fabricate duplicate Netty callbacks.
        denied(()->bridge.observe(observations.rows.get(2),at+9_000_000L),
            "Native per-packet sequence replay denied");
        SourcePacketReadOnlyQueue.Observation next=fact(
            SourcePacketReadOnlyQueue.OUT,SourcePacketReadOnlyQueue.ATTACK,
            0,false,false,0,at+10);
        // next is sequence 1, so it cannot masquerade as seq 4.
        denied(()->bridge.observe(next,at+10_000_000L),
            "Different primitive queue cannot forge next packet sequence");

        SourceWorldReadOnlyHost.TickReceipt second=host.observe(port);
        bridge.begin(channel,second);
        SourcePacketReadOnlyQueue.Observation renewed=fact(
            SourcePacketReadOnlyQueue.IN,SourcePacketReadOnlyQueue.UNKNOWN,
            0,false,false,0,at+11);
        bridge.observe(renewed,at+12_000_000L);
        yes(bridge.pending()==1,"Next JNI tick admits new primitive packet stream");
        yes(bridge.consume(channel,b0.worldToken,b0.generation,1,
            at+SourcePacketVerdictBridge.MAX_DECISION_AGE_NANOS+13_000_000L)==null,
            "TTL expiry refuses old Netty packet ownership decision");
        yes(!bridge.healthy()&&bridge.pending()==0,
            "Stale native source packet verdict poisons entire decision epoch");
        denied(()->bridge.observe(renewed,at+15_000_000L),
            "Failed epoch cannot be reused");

        port.world=new Object();
        SourceWorldReadOnlyHost.TickReceipt newWorld=host.observe(port);
        bridge.begin(channel+1,newWorld);
        SourcePacketReadOnlyQueue.Observation fresh=fact(
            SourcePacketReadOnlyQueue.IN,SourcePacketReadOnlyQueue.UNKNOWN,
            0,false,false,0,at+16);
        SourcePacketVerdictBridge.Decision changed=
            bridge.observe(fresh,at+18_000_000L);
        yes(changed.generation==b0.generation+1&&
            changed.worldToken!=b0.worldToken,
            "Real native world transition invalidates prior packet decisions");
        yes(bridge.consume(channel,b0.worldToken,b0.generation,1,
            at+19_000_000L)==null,"No old-world decision can match");
        yes(bridge.consume(channel+1,changed.worldToken,changed.generation,1,
                           at+19_000_000L)==changed,
            "Fresh client generation and channel identity admitted");
        bridge.revoke();
        yes(bridge.pending()==0&&!bridge.healthy(),
            "Explicit owner revoke leaves no candidate verdicts");

        // Overflow never partially maps a packet verdict to another Packet.
        SourceWorldReadOnlyHost.TickReceipt third=host.observe(port);
        bridge.begin(channel+1,third);
        SourcePacketReadOnlyQueue flood=new SourcePacketReadOnlyQueue();
        for(int i=0;i<SourcePacketVerdictBridge.CAPACITY+1;++i){
            yes(flood.offer(SourcePacketReadOnlyQueue.IN,
                SourcePacketReadOnlyQueue.UNKNOWN,0,false,false,0,0,0,at+20+i),
                "Bounded inbound primitive fixture");
        }
        SourcePacketReadOnlyQueue.Batch group=flood.drain(128);
        for(int i=0;i<SourcePacketVerdictBridge.CAPACITY;++i)
            bridge.observe(group.rows.get(i),at+20_000_000L+i);
        yes(bridge.pending()==SourcePacketVerdictBridge.CAPACITY,
            "Exactly 64 immutable source verdicts may be pending");
        denied(()->bridge.observe(group.rows.get(SourcePacketVerdictBridge.CAPACITY),
                                  at+21_000_000L),
               "Overflow fails closed before dispatching additional source JNI");
        yes(!bridge.healthy()&&bridge.pending()==0&&
            bridge.rejected()>=SourcePacketVerdictBridge.CAPACITY,
            "Overflow cannot leak arbitrary old per-packet decisions");

        flood.close();stream.close();
        bridge.close();host.close();
        yes(!bridge.healthy()&&bridge.consume(channel+1,changed.worldToken,
            changed.generation,1,at+30_000_000L)==null,
            "Closed source verdict bridge cannot serve foreign Netty");
        System.out.println("R347_REAL_NATIVE_PACKET_VERDICT_TO_NETTY_FIFO_PASS");
    }
}
