package dev.slinkywinky;

import java.util.HashMap;
import java.util.Map;

public final class ClutchWorldSamplerSmoke {
    private static void check(boolean yes,String why) {
        if(!yes)throw new AssertionError(why);
    }
    private static void refused(Runnable r,String why) {
        try{r.run();throw new AssertionError("accepted "+why);}
        catch(IllegalStateException expected) { /* fail closed */ }
    }
    private static final class Fake implements ClutchWorldSampler.Port {
        Object world=new Object(),player=new Object();
        final Map<String,Integer> cells=new HashMap<String,Integer>();
        int reads,rays,mutateAt=-1;
        ClutchWorldSampler.Pos hit;
        boolean client=true;
        String key(int x,int y,int z) {return x+":"+y+":"+z;}
        void put(int x,int y,int z,int id){cells.put(key(x,y,z),id);}
        public boolean onClientThread(){return client;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public int blockId(int x,int y,int z) {
            ++reads;
            if(reads==mutateAt)world=new Object();
            Integer id=cells.get(key(x,y,z));
            return id==null?0:id.intValue();
        }
        public ClutchWorldSampler.Pos rayTrace(double[] from,double[] to) {
            ++rays;return hit;
        }
    }
    private static ClutchWorldSampler.Request request(long gen,long tick,
            int range,float height,ClutchWorldSampler.Pos previous) {
        return new ClutchWorldSampler.Request(gen,tick,
            new double[]{.2,64,.2},new double[]{.8,65.8,.8},
            new double[]{.5,65.5,.5},range,height,previous);
    }
    public static void main(String[] argv) throws Exception {
        int[] offsets=ClutchWorldSampler.sourcePacked();
        int[] counts=ClutchWorldSampler.sourcePrefixes();
        check(offsets.length==167 && counts.length==8,"table lengths");
        check(offsets[0]==0x888 && offsets[1]==0x887 &&
              offsets[2]==0x878 && offsets[5]==0x889 &&
              offsets[166]==0xb7b,"source offset order");
        offsets[0]=0;check(ClutchWorldSampler.sourcePacked()[0]==0x888,
            "immutable candidate offsets");
        Fake port=new Fake();
        port.put(1,63,0,5);
        ClutchWorldSampler owner=new ClutchWorldSampler();
        ClutchWorldSampler.Snapshot snap=owner.capture(port,request(1,1,1,0,null));
        check(snap.baseX==0 && snap.baseY==63 && snap.baseZ==0,"base");
        check(snap.candidateBlockIds().length==6 &&
            snap.candidateBlockIds()[5]==5 && port.reads==6,"ordered six reads");
        int[] copy=snap.candidateBlockIds();copy[5]=99;
        check(snap.candidateBlockIds()[5]==5,"immutable receipt");
        refused(()->owner.capture(port,request(1,1,1,0,null)),"replayed tick");

        Fake all=new Fake();
        ClutchWorldSampler complete=new ClutchWorldSampler();
        snap=complete.capture(all,request(1,10,7,0,null));
        check(snap.candidateBlockIds().length==167 && all.reads==167,
              "full 167 sampled on source order");

        Fake ground=new Fake();
        ground.put(0,63,0,3);
        ClutchWorldSampler withGround=new ClutchWorldSampler();
        snap=withGround.capture(ground,request(1,1,7,2,null));
        check(snap.groundBlocked && ground.reads==1 &&
              snap.groundBlockIds()[0]==3,"source ground stops candidate reads");

        Fake prior=new Fake();
        prior.put(-1,63,0,1);
        ClutchWorldSampler raySession=new ClutchWorldSampler();
        snap=raySession.capture(prior,request(4,1,0,0,
            new ClutchWorldSampler.Pos(-1,63,0)));
        check(snap.previousBlockId==1,"prior source block id");
        ClutchWorldSampler.Snapshot raySnapshot=snap;
        ClutchWorldSampler.RayReceipt receipt=raySession.priorRay(
            prior,snap,new double[]{0,63.5,.5});
        check(receipt.tick==1 && receipt.hit==null && prior.rays==1,
              "source null ray is observed, not guessed");
        refused(()->raySession.priorRay(prior,raySnapshot,
            new double[]{0,63.5,.5}),"ray replay");
        check(prior.rays==1,"no repeated game ray call");

        Fake far=new Fake();
        far.put(-1,63,0,1);
        ClutchWorldSampler farSession=new ClutchWorldSampler();
        final ClutchWorldSampler.Snapshot farSnap=farSession.capture(
            far,request(1,1,0,0,new ClutchWorldSampler.Pos(-1,63,0)));
        refused(()->farSession.priorRay(far,farSnap,
            new double[]{200,63.5,.5}),"out-of-reach ray");
        check(far.rays==0,"no speculative ray");

        Fake drift=new Fake();
        drift.mutateAt=3;
        ClutchWorldSampler lost=new ClutchWorldSampler();
        refused(()->lost.capture(drift,request(1,1,7,0,null)),
            "world transition mid-scan");
        refused(()->lost.capture(drift,request(2,2,0,0,null)),
            "closed owner cannot revive");

        Fake bad=new Fake();
        ClutchWorldSampler invalid=new ClutchWorldSampler();
        refused(()->invalid.capture(bad,request(1,1,8,0,null)),"range 8");
        check(bad.reads==0,"reject malformed range before sampling");

        Fake changed=new Fake();
        ClutchWorldSampler strict=new ClutchWorldSampler();
        strict.capture(changed,request(9,1,0,0,null));
        changed.player=new Object();
        refused(()->strict.capture(changed,request(10,2,0,0,null)),
            "new generation cannot hide stale identity");

        Fake wrongThread=new Fake();
        final ClutchWorldSampler threaded=new ClutchWorldSampler();
        final boolean[] denied={false};
        Thread other=new Thread(()->{
            try {threaded.capture(wrongThread,request(1,1,0,0,null));}
            catch(IllegalStateException good){denied[0]=true;}
        });
        other.start();other.join();
        check(denied[0],"thread owner fence");
        System.out.println("R237_JAVA8_CLUTCH_WORLD_OBSERVATION_SMOKE_PASS");
    }
}
