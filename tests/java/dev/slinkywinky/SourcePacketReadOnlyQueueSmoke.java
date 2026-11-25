package dev.slinkywinky;

/** R339: primitive-only FIFO, monotonic seq, world teardown and overflow. */
public final class SourcePacketReadOnlyQueueSmoke {
    private static void yes(boolean b,String msg){if(!b)throw new AssertionError(msg);}
    private static void denied(Runnable job,String msg){
        boolean refused=false;
        try{job.run();}catch(IllegalArgumentException|IllegalStateException|UnsupportedOperationException e){refused=true;}
        yes(refused,msg);
    }
    public static void main(String[] ignored)throws Exception{
        final SourcePacketReadOnlyQueue q=new SourcePacketReadOnlyQueue();
        final boolean[] sent={false};
        Thread io=new Thread(()->{
            sent[0]=q.offer(SourcePacketReadOnlyQueue.IN,
                SourcePacketReadOnlyQueue.ENTITY_STATUS,0,false,false,
                0,0,0,System.nanoTime());
            q.offer(SourcePacketReadOnlyQueue.OUT,
                SourcePacketReadOnlyQueue.MOVEMENT,0,false,true,
                1,2,3,System.nanoTime());
        });
        io.start();io.join();
        yes(sent[0],"Network-thread primitive ingress");
        SourcePacketReadOnlyQueue.Batch initial=q.drain(1);
        yes(initial.rows.size()==1&&initial.rows.get(0).sequence==1&&
            initial.rows.get(0).direction==SourcePacketReadOnlyQueue.IN,
            "First inbound packet retained in FIFO order");
        SourcePacketReadOnlyQueue.Batch rest=q.drain(32);
        yes(rest.rows.size()==1&&rest.rows.get(0).sequence==2&&
            rest.rows.get(0).moving&&rest.rows.get(0).x==1,
            "Second outbound movement packet retained");
        denied(()->initial.rows.clear(),"Immutable returned list");
        final boolean[] foreign={false};
        Thread other=new Thread(()->{
            try{q.drain(4);}catch(IllegalStateException e){foreign[0]=true;}
        });
        other.start();other.join();
        yes(foreign[0],"No JNI / replay from network thread");
        denied(()->q.drain(SourcePacketReadOnlyQueue.CAPACITY+1),"Bounded batch");
        q.close();
        yes(!q.offer(0,1,0,false,false,0,0,0,System.nanoTime()),
            "Closed session ignores in-flight Netty packet");
        denied(()->q.drain(4),"Closed session cannot resurrect packets");

        SourcePacketReadOnlyQueue overflow=new SourcePacketReadOnlyQueue();
        for(int i=0;i<SourcePacketReadOnlyQueue.CAPACITY;++i)
            yes(overflow.offer(0,1,0,false,false,0,0,0,System.nanoTime()),
                "Capacity minus one packets accepted");
        yes(!overflow.offer(0,1,0,false,false,0,0,0,System.nanoTime()),
            "Overflow poisons rather than silently skipping one packet");
        yes(!overflow.healthy()&&overflow.pending()==0,
            "Overflow dropped all partial stream state");
        denied(()->overflow.drain(32),"Discontinuous packet queue denied");
        overflow.close();

        SourcePacketReadOnlyQueue corrupted=new SourcePacketReadOnlyQueue();
        yes(!corrupted.offer(1,6,0,false,true,Double.NaN,0,0,System.nanoTime()),
            "Bad coordinates poison stream");
        denied(()->corrupted.drain(10),"Invalid packet stream cannot replay");
        corrupted.close();
        System.out.println("R339_PACKET_FIFO_THREAD_AND_OVERFLOW_PASS");
    }
}
