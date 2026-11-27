package dev.slinkywinky;

/**
 * R348 actual existing Forge-shaped native host packet callback receipt:
 * live-style passive observation -> real C++ JNI per-event proposals ->
 * immutable aggregate telemetry. No packet hold/replay operation.
 */
public final class SourceNativeVerdictTelemetrySmoke {
    static void yes(boolean value,String what){if(!value)throw new AssertionError(what);}
    public static void main(String[] args)throws Exception{
        SourceWorldReadOnlyHost host=new SourceWorldReadOnlyHost();
        SourceWorldReadOnlyHostSmoke.Port port=new SourceWorldReadOnlyHostSmoke.Port();
        // Force source reach admission ONLY for the offline acceptance case.
        // Actual Forge read-only runtime correctly leaves it unverified.
        port.current=SourcePacketVerdictBridgeSmoke.activeFrame();
        SourceWorldReadOnlyHost.TickReceipt tick=host.observe(port);
        yes(tick.backtrack.at(3)==1&&tick.lagRange.at(3)==1,
            "Native Backtrack/Lag Range tick callbacks armed in controlled fixture");
        SourcePacketReadOnlyQueue queue=new SourcePacketReadOnlyQueue();
        Thread netty=new Thread(()->{
            long t=System.nanoTime();
            yes(queue.offer(SourcePacketReadOnlyQueue.IN,
                 SourcePacketReadOnlyQueue.UNKNOWN,0,false,false,0,0,0,t),
                 "Unrecognized actual packet class still observed as primitive");
            yes(queue.offer(SourcePacketReadOnlyQueue.IN,
                 SourcePacketReadOnlyQueue.ENTITY_STATUS,21,true,false,0,0,0,t+1),
                 "Exact target ID status feedback is observed");
            yes(queue.offer(SourcePacketReadOnlyQueue.OUT,
                 SourcePacketReadOnlyQueue.MOVEMENT,0,false,true,9,0,0,t+2),
                 "Native closer movement packet observed without intervention");
        });
        netty.start();netty.join();
        SourceWorldReadOnlyHost.PacketReceipt receipt=host.observePackets(port,queue);
        yes(receipt!=null&&receipt.incoming==2&&receipt.outgoing==1&&
            receipt.callbacks==4&&receipt.observed==3,
            "Native callbacks retain original four-phase e2/e3/e1 order");
        yes(receipt.backtrackHold==1&&receipt.backtrackFeedback==1,
            "ACTUAL Backtrack JNI event2 hold and matching target pass-feedback");
        yes(receipt.lagRangeHold==1,
            "ACTUAL Lag Range JNI event3 moving closer hold proposal");
        yes(receipt.timerHold==0||receipt.timerHold==1,
            "Timer Range native event1 remains a validated boolean proposal");
        yes(queue.pending()==0&&queue.healthy(),
            "Native proposal telemetry cannot mutate, poison or resend packets");
        SourceWorldReadOnlyHost.PacketReceipt empty=host.observePackets(port,queue);
        yes(empty.callbacks==0&&empty.backtrackHold==0&&
            empty.backtrackFeedback==0&&empty.lagRangeHold==0&&
            empty.timerHold==0&&empty.observed==3,
            "Empty source native batch cannot invent packet holding");
        port.world=new Object();
        SourceWorldReadOnlyHost.TickReceipt newWorld=host.observe(port);
        yes(newWorld.generation==tick.generation+1&&!queue.healthy(),
            "World replacement revokes original passive packet ingress");
        host.close();
        System.out.println("R348_REAL_FORGE_SHAPED_NATIVE_PACKET_PROPOSALS_PASS");
    }
}
