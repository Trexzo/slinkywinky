package dev.slinkywinky;

/** R339: primitive Netty-thread packets -> bounded FIFO -> real JNI replay. */
public final class SourcePacketNativeReplaySmoke {
    static void yes(boolean b,String why){if(!b)throw new AssertionError(why);}
    static void denied(Runnable r,String why){
        boolean refused=false;
        try{r.run();}catch(IllegalArgumentException|IllegalStateException expected){
            refused=true;
        }
        yes(refused,why);
    }
    static void add(SourcePacketReadOnlyQueue q,int dir,int kind,int seq) {
        if(!q.offer(dir,kind,seq,true,kind==SourcePacketReadOnlyQueue.MOVEMENT,
                    kind==SourcePacketReadOnlyQueue.MOVEMENT?4:0,
                    0,0,System.nanoTime()))
            throw new AssertionError("Packet queue unexpectedly rejected "+kind);
    }
    public static void main(String[] args)throws Exception{
        final SourceWorldReadOnlyHost host=new SourceWorldReadOnlyHost();
        final SourceWorldReadOnlyHostSmoke.Port port=new SourceWorldReadOnlyHostSmoke.Port();
        final SourcePacketReadOnlyQueue packets=new SourcePacketReadOnlyQueue();
        yes(host.observePackets(port,packets)==null,
            "Cannot send packets to native before authenticated world tick");
        host.observe(port);
        Thread netty=new Thread(()->{
            add(packets,SourcePacketReadOnlyQueue.IN,
                SourcePacketReadOnlyQueue.ENTITY_STATUS,0);
            add(packets,SourcePacketReadOnlyQueue.IN,
                SourcePacketReadOnlyQueue.VELOCITY,42);
            add(packets,SourcePacketReadOnlyQueue.OUT,
                SourcePacketReadOnlyQueue.MOVEMENT,0);
            add(packets,SourcePacketReadOnlyQueue.OUT,
                SourcePacketReadOnlyQueue.ATTACK,0);
            add(packets,SourcePacketReadOnlyQueue.OUT,
                SourcePacketReadOnlyQueue.INV_OPEN,0);
            add(packets,SourcePacketReadOnlyQueue.OUT,
                SourcePacketReadOnlyQueue.INV_CLICK,0);
            add(packets,SourcePacketReadOnlyQueue.OUT,
                SourcePacketReadOnlyQueue.INV_CLOSE,0);
        });
        netty.start();netty.join();
        SourceWorldReadOnlyHost.PacketReceipt batch=host.observePackets(port,packets);
        yes(batch.incoming==2&&batch.outgoing==5&&batch.inventory==3&&
            batch.callbacks==15&&batch.observed==7,
            "Both directions hit JNI events 2/3/23 plus outgoing Timer Range event1");
        yes(batch.generation==1&&batch.tick==1,"Same owner epoch and tick");
        SourceWorldReadOnlyHost.PacketReceipt empty=host.observePackets(port,packets);
        yes(empty.callbacks==0&&empty.observed==7,"Packets consumed exactly once");
        final boolean[] foreign={false};
        Thread bad=new Thread(()->{
            try{host.observePackets(port,packets);}
            catch(IllegalStateException e){foreign[0]=true;}
        });
        bad.start();bad.join();
        yes(foreign[0],"Netty cannot call native replay");
        packets.close();
        denied(()->host.observePackets(port,packets),
               "Unexpected close poisons native packet session");
        denied(()->host.observe(port),"JNI source host retired on lost packet stream");

        final SourceWorldReadOnlyHost different=new SourceWorldReadOnlyHost();
        final SourceWorldReadOnlyHostSmoke.Port freshPort=
            new SourceWorldReadOnlyHostSmoke.Port();
        final SourcePacketReadOnlyQueue first=new SourcePacketReadOnlyQueue();
        final SourcePacketReadOnlyQueue other=new SourcePacketReadOnlyQueue();
        different.observe(freshPort);
        add(first,SourcePacketReadOnlyQueue.OUT,SourcePacketReadOnlyQueue.ATTACK,0);
        yes(different.observePackets(freshPort,first).callbacks==2,
            "One channel: Lag Range e3 and Timer Range e1 callbacks");
        add(other,SourcePacketReadOnlyQueue.OUT,SourcePacketReadOnlyQueue.ATTACK,0);
        denied(()->different.observePackets(freshPort,other),
               "Changed channel in same generation is forbidden");
        denied(()->different.observe(freshPort),
               "Channel swap retires native controller state");
        first.close();other.close();

        SourceWorldReadOnlyHost overloaded=new SourceWorldReadOnlyHost();
        SourcePacketReadOnlyQueue queued=new SourcePacketReadOnlyQueue();
        overloaded.observe(freshPort);
        for(int i=0;i<SourcePacketReadOnlyQueue.CAPACITY;++i)
            add(queued,SourcePacketReadOnlyQueue.IN,SourcePacketReadOnlyQueue.UNKNOWN,0);
        yes(!queued.offer(SourcePacketReadOnlyQueue.IN,
                          SourcePacketReadOnlyQueue.UNKNOWN,0,false,false,
                          0,0,0,System.nanoTime()),"Overflow triggered");
        denied(()->overloaded.observePackets(freshPort,queued),
               "Overflow refuses partial native playback");
        denied(()->overloaded.observe(freshPort),
               "Overflow permanently closed native source session");
        queued.close();

        SourceWorldReadOnlyHost renewed=new SourceWorldReadOnlyHost();
        SourcePacketReadOnlyQueue previous=new SourcePacketReadOnlyQueue();
        renewed.observe(freshPort);
        add(previous,SourcePacketReadOnlyQueue.IN,SourcePacketReadOnlyQueue.SOUND,0);
        freshPort.world=new Object();
        yes(renewed.observePackets(freshPort,previous)==null,
            "World replacement discards stale Netty payload");
        SourceWorldReadOnlyHost.TickReceipt next=renewed.observe(freshPort);
        yes(next.generation==2,"Native world generation advanced");
        previous.close();
        SourcePacketReadOnlyQueue current=new SourcePacketReadOnlyQueue();
        add(current,SourcePacketReadOnlyQueue.IN,SourcePacketReadOnlyQueue.SOUND,0);
        yes(renewed.observePackets(freshPort,current).incoming==1,
            "New-world queue admitted without old packet refs");
        current.close();renewed.close();
        System.out.println("R339_NETTY_QUEUE_REAL_JNI_SOURCE_REPLAY_PASS");
    }
}
