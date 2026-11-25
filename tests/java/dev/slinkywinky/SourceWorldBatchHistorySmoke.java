package dev.slinkywinky;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/** R343 Java8/real JNI bounded multi-player source-world history admission. */
public final class SourceWorldBatchHistorySmoke {
    static void yes(boolean value,String what){if(!value)throw new AssertionError(what);}
    static void denied(Runnable r,String what){
        boolean failed=false;
        try{r.run();}
        catch(IllegalStateException|IllegalArgumentException expected){failed=true;}
        yes(failed,what);
    }
    static List<SourceWorldReadOnlyHost.RemoteTarget> targets(int count){
        List<SourceWorldReadOnlyHost.RemoteTarget> a=
            new ArrayList<SourceWorldReadOnlyHost.RemoteTarget>();
        for(int i=0;i<count;++i)
            a.add(new SourceWorldReadOnlyHost.RemoteTarget(
                100+i,9000+i,8000+i,(i+1)*32,640,64));
        return a;
    }
    static final class Port implements SourceWorldReadOnlyHost.HistoryPort {
        Object world=new Object(),player=new Object();
        List<SourceWorldReadOnlyHost.RemoteTarget> batch=targets(32);
        public boolean admittedClientThread(){return true;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public SourceWorldReadOnlyHost.Frame capture(){
            return SourceWorldReadOnlyHostSmoke.frame();
        }
        public SourceWorldReadOnlyHost.RemoteTarget captureRemoteTarget(){
            return batch.isEmpty()?null:batch.get(0);
        }
        public List<SourceWorldReadOnlyHost.RemoteTarget> captureRemoteTargets(){
            return batch;
        }
        public SourceWorldReadOnlyHost.RenderFrame captureRender(double dt,float partial){
            return null;
        }
        public SourceWorldReadOnlyHost.BoxFrame captureBoxes(float partial){
            return null;
        }
        public SourceWorldReadOnlyHost.BoxFrame captureHistoricalBoxes(float partial,
                SourceRemotePositionHistory.Historical h){
            return new SourceWorldReadOnlyHost.BoxFrame(new double[]{
                h.previous.x(),h.previous.y(),h.previous.z(),
                h.current.x(),h.current.y(),h.current.z(),
                0,0,0,partial,0,0});
        }
    }
    public static void main(String[] unused)throws Exception{
        Port port=new Port();
        SourceWorldReadOnlyHost h=new SourceWorldReadOnlyHost();
        SourceWorldReadOnlyHost.TickReceipt tick=h.observe(port);
        yes(tick.generation==1&&h.history().tracked()==32,
            "Native four-module tick admits all 32 authenticated remote players");
        for(int i=0;i<32;++i){
            yes(h.history().historical(100+i,9000+i,8000+i,
                System.nanoTime()+100_000_000L,80_000_000L,
                600_000_000L)!=null,
                "Distinct remote player history "+i+" survives target switch");
        }
        port.batch=targets(8);
        h.observe(port);
        yes(h.history().tracked()==32,
            "Changing target cannot unexpectedly remove other remote histories");
        final SourcePacketReadOnlyQueue boundPackets=
            new SourcePacketReadOnlyQueue();
        yes(boundPackets.offer(SourcePacketReadOnlyQueue.IN,
                SourcePacketReadOnlyQueue.UNKNOWN,0,false,false,
                0,0,0,System.nanoTime()),"Admit old-world primitive packet");
        yes(h.observePackets(port,boundPackets).incoming==1,
            "Old-world network ingress bound to one native generation");
        port.world=new Object();
        port.batch=Collections.emptyList();
        yes(h.observe(port).generation==2&&h.history().tracked()==0,
            "World swap atomically retires all old remote UUID positions");
        yes(!boundPackets.healthy()&&boundPackets.pending()==0,
            "Direct world switch closes old primitive Netty queue");
        h.close();

        Port duplicate=new Port();
        duplicate.batch=targets(2);
        duplicate.batch.add(new SourceWorldReadOnlyHost.RemoteTarget(
            100,99999,88888,32,0,0));
        SourceWorldReadOnlyHost other=new SourceWorldReadOnlyHost();
        denied(()->other.observe(duplicate),
            "Two identities reusing entity ID in same tick fail closed");
        denied(()->other.observe(duplicate),
            "Failed batch never revives native session");

        Port tooMany=new Port();
        tooMany.batch=targets(SourceRemotePositionHistory.MAX_ENTITIES+1);
        SourceWorldReadOnlyHost capped=new SourceWorldReadOnlyHost();
        denied(()->capped.observe(tooMany),
            "Cannot admit more than the strict 32-entity server history cap");
        denied(()->capped.observe(tooMany),
            "Overcapacity owner has no retained native session");

        Port nulls=new Port();
        nulls.batch=new ArrayList<SourceWorldReadOnlyHost.RemoteTarget>();
        nulls.batch.add(null);
        SourceWorldReadOnlyHost invalid=new SourceWorldReadOnlyHost();
        denied(()->invalid.observe(nulls),
            "Null/partial player records cannot poison valid history");
        System.out.println("R343_REAL_JNI_32_PLAYER_SERVER_HISTORY_BATCH_PASS");
    }
}
