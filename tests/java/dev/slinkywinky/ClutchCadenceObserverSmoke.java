package dev.slinkywinky;

public final class ClutchCadenceObserverSmoke {
    private static void check(boolean value,String what){
        if(!value)throw new AssertionError(what);
    }
    private static void refused(Runnable action,String what){
        try{action.run();throw new AssertionError("accepted "+what);}
        catch(IllegalStateException denied){}
    }
    private static final class Port implements ClutchFaceSampler.Port {
        Object world=new Object(),player=new Object();
        ClutchFaceSampler.Hit hit;
        int reads;
        boolean onThread=true;
        public boolean onClientThread(){return onThread;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public ClutchFaceSampler.Hit cursorBlockHit(){
            ++reads;return hit;
        }
        void face(int v){
            hit=new ClutchFaceSampler.Hit(
                new ClutchFaceSampler.Pos(4,66,7),v);
        }
    }
    private static final ClutchFaceSampler.Pos TARGET =
        new ClutchFaceSampler.Pos(4,66,7);
    private static ClutchFaceSampler.Receipt observed(
            ClutchFaceSampler scanner,Port p,long tick,boolean top){
        return scanner.capture(p,new ClutchFaceSampler.Request(
            11,tick,TARGET,top));
    }
    public static void main(String[] args)throws Exception {
        Port p=new Port();p.face(5);
        ClutchFaceSampler scanner=new ClutchFaceSampler();
        ClutchCadenceObserver observer=new ClutchCadenceObserver();
        // Native fixture at 10CPS: due, 1-tick wait, due.
        ClutchCadenceObserver.Step s=observer.advance(p,11,1,10,true,
            TARGET,false,observed(scanner,p,1,false));
        check(s.timerDue && s.wouldPress && s.timerAfter==2.0f &&
              s.adjacent.x==5 && s.adjacent.y==66,"first press proposal");
        s=observer.advance(p,11,2,10,true,TARGET,false,
            observed(scanner,p,2,false));
        check(!s.timerDue && !s.wouldPress && s.timerAfter==1.0f,
              "intermediate tick");
        s=observer.advance(p,11,3,10,true,TARGET,false,
            observed(scanner,p,3,false));
        check(s.timerDue && s.wouldPress && s.timerAfter==2.0f,
              "third tick");
        refused(()->observer.advance(p,11,3,10,true,TARGET,false,null),
                "repeated tick");
        check(p.reads==3,"observer never reads cursor itself");
        // Source consumes cadence even with no plan or game focus.
        Port absent=new Port();
        ClutchCadenceObserver without=new ClutchCadenceObserver();
        s=without.advance(absent,11,1,10,false,null,false,null);
        check(s.timerDue && !s.wouldPress && s.timerAfter==2.0f,
              "advance while inactive");
        s=without.advance(absent,11,2,10,true,null,false,null);
        check(!s.timerDue && !s.wouldPress,"advance without candidate");
        // Fractional float remainder parity with C++ R247 fixture.
        final boolean[] expected={true,false,true,false,true,true,false,true};
        Port fp=new Port();fp.face(2);
        ClutchFaceSampler fscan=new ClutchFaceSampler();
        ClutchCadenceObserver fractional=new ClutchCadenceObserver();
        for(int i=0;i<expected.length;i++){
            s=fractional.advance(fp,11,i+1,12.5f,true,TARGET,false,
                observed(fscan,fp,i+1,false));
            check(s.wouldPress==expected[i],"12.5CPS step "+i);
        }
        // Wrong real face: timer advances but no press proposal.
        Port wrong=new Port();wrong.face(1);
        ClutchFaceSampler wscan=new ClutchFaceSampler();
        ClutchCadenceObserver wobs=new ClutchCadenceObserver();
        s=wobs.advance(wrong,11,1,20,true,TARGET,false,
            observed(wscan,wrong,1,false));
        check(s.timerDue && !s.wouldPress,"UP not sideways");
        // A valid receipt cannot be substituted for another target or tick.
        Port substituted=new Port();substituted.face(5);
        ClutchFaceSampler ss=new ClutchFaceSampler();
        ClutchFaceSampler.Receipt actual=observed(ss,substituted,1,false);
        ClutchCadenceObserver sb=new ClutchCadenceObserver();
        refused(()->sb.advance(substituted,11,1,20,true,
            new ClutchFaceSampler.Pos(9,66,7),false,actual),
            "cross-target receipt");
        // No retroactive acceptance after a failed owner check.
        refused(()->sb.advance(substituted,11,2,20,true,null,false,null),
                "denied owner remains closed");
        Port swapped=new Port();swapped.face(5);
        ClutchFaceSampler sx=new ClutchFaceSampler();
        ClutchCadenceObserver ox=new ClutchCadenceObserver();
        ox.advance(swapped,11,1,20,true,TARGET,false,
            observed(sx,swapped,1,false));
        swapped.world=new Object();
        refused(()->ox.advance(swapped,11,2,20,true,null,false,null),
                "stale world");
        Port bad=new Port();
        ClutchCadenceObserver invalid=new ClutchCadenceObserver();
        refused(()->invalid.advance(bad,11,1,Float.NaN,true,
            null,false,null),"nonfinite cps");
        Port foreign=new Port();
        ClutchCadenceObserver fown=new ClutchCadenceObserver();
        final boolean[] denied={false};
        Thread t=new Thread(()->{
            try {fown.advance(foreign,11,1,10,true,null,false,null);}
            catch(IllegalStateException rejection){denied[0]=true;}
        });
        t.start();t.join();
        check(denied[0] && foreign.reads==0,"foreign thread rejected");
        System.out.println("R252_JAVA8_CLUTCH_OWNER_CADENCE_PARITY_PASS");
    }
}
