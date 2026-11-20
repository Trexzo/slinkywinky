package dev.slinkywinky;

public final class ClutchFaceSamplerSmoke {
    private static void check(boolean v,String msg) {
        if(!v)throw new AssertionError(msg);
    }
    private static void refused(Runnable r,String msg) {
        try {r.run();throw new AssertionError("accepted "+msg);}
        catch(IllegalStateException expected) { }
    }
    private static final class Fake implements ClutchFaceSampler.Port {
        Object world=new Object(),player=new Object();
        ClutchFaceSampler.Hit hit;boolean client=true;
        int calls;boolean swapOnRead;
        public boolean onClientThread(){return client;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public ClutchFaceSampler.Hit cursorBlockHit(){
            ++calls;
            if(swapOnRead)world=new Object();
            return hit;
        }
        void set(int x,int y,int z,int face){
            hit=new ClutchFaceSampler.Hit(new ClutchFaceSampler.Pos(x,y,z),face);
        }
    }
    private static ClutchFaceSampler.Request req(long gen,long tick,boolean top){
        return new ClutchFaceSampler.Request(gen,tick,
            new ClutchFaceSampler.Pos(2,64,3),top);
    }
    public static void main(String[] args)throws Exception {
        final int[][] expected={{2,63,3},{2,65,3},{2,64,2},
                                {2,64,4},{1,64,3},{3,64,3}};
        for(int face=0;face<6;face++){
            Fake port=new Fake();port.set(2,64,3,face);
            ClutchFaceSampler sampler=new ClutchFaceSampler();
            ClutchFaceSampler.Receipt r=sampler.capture(port,
                req(1,1,face==1));
            check(r.matchesPlannedFace(),"matched original face "+face);
            check(r.adjacent.x==expected[face][0] &&
                  r.adjacent.y==expected[face][1] &&
                  r.adjacent.z==expected[face][2],"adjacent "+face);
            check(port.calls==1,"one real sample only");
            sampler.close();
        }
        Fake missing=new Fake();ClutchFaceSampler none=new ClutchFaceSampler();
        ClutchFaceSampler.Receipt r=none.capture(missing,req(1,1,false));
        check(!r.hitBlock && r.observedFace==-1 && r.adjacent==null,
              "null is observed absence");
        missing.set(9,64,3,5);
        r=none.capture(missing,req(1,2,false));
        check(r.hitBlock && !r.matchedTarget && !r.matchesPlannedFace(),
              "foreign target cannot satisfy plan");
        missing.set(2,64,3,1);
        r=none.capture(missing,req(1,3,false));
        check(r.matchedTarget && !r.matchedFace && r.adjacent==null,
              "UP cannot satisfy sideways plan");
        refused(()->none.capture(missing,req(1,3,false)),"replay tick");
        check(missing.calls==3,"replay refused before I/O");

        Fake swapped=new Fake();swapped.set(2,64,3,1);
        swapped.swapOnRead=true;
        ClutchFaceSampler old=new ClutchFaceSampler();
        refused(()->old.capture(swapped,req(1,1,true)),"world swap on read");
        refused(()->old.capture(swapped,req(2,2,true)),"closed owner stays dead");

        Fake bad=new Fake();bad.set(2,64,3,6);
        ClutchFaceSampler invalid=new ClutchFaceSampler();
        refused(()->invalid.capture(bad,req(1,1,false)),"invalid face");
        Fake changed=new Fake();changed.set(2,64,3,1);
        ClutchFaceSampler life=new ClutchFaceSampler();
        life.capture(changed,req(1,1,true));
        changed.player=new Object();
        refused(()->life.capture(changed,req(1,2,true)),"changed player");
        Fake own=new Fake();own.set(2,64,3,1);
        ClutchFaceSampler thread=new ClutchFaceSampler();
        final boolean[] denied={false};
        Thread worker=new Thread(()->{
            try {thread.capture(own,req(1,1,true));}
            catch(IllegalStateException expected){denied[0]=true;}
        });worker.start();worker.join();
        check(denied[0] && own.calls==0,"no foreign thread I/O");
        Fake edge=new Fake();edge.set(999999,64,3,5);
        ClutchFaceSampler overflow=new ClutchFaceSampler();
        refused(()->overflow.capture(edge,new ClutchFaceSampler.Request(
            1,1,new ClutchFaceSampler.Pos(999999,64,3),false)),
            "adjacent coordinate overflow");
        System.out.println("R250_JAVA8_CLUTCH_REAL_CURSOR_FACE_NEGATIVES_PASS");
    }
}
