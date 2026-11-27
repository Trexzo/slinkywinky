package dev.slinkywinky;

/** Real Java8 one-shot attack ownership/expiry regression, no game writes. */
public final class SourceAttackEventFenceSmoke {
    static void yes(boolean value,String what){
        if(!value)throw new AssertionError(what);
    }
    static void refuses(Runnable action,String what){
        boolean rejected=false;
        try{action.run();}
        catch(IllegalStateException|IllegalArgumentException expected){rejected=true;}
        yes(rejected,what);
    }
    public static void main(String[] args)throws Exception{
        Object world=new Object(),player=new Object(),other=new Object();
        SourceAttackEventFence f=new SourceAttackEventFence();
        final long at=1_000_000_000L;
        yes(!f.consume(world,player,100,at),"No event means no admission");
        f.record(world,player,100,at);
        f.record(world,player,100,at+1);
        yes(f.recorded()==1&&f.pending()==1,"Duplicate same tick coalesces");
        yes(!f.consume(world,other,100,at+2),"Wrong player rejected");
        yes(f.pending()==0,"Wrong player revokes receipt");
        f.record(world,player,100,at+10);
        yes(f.consume(world,player,101,at+11),"Next tick admitted");
        yes(!f.consume(world,player,101,at+12),"Receipt single use");
        f.record(world,player,100,at+20);
        yes(!f.consume(world,player,102,at+21),"Future tick refused");
        f.record(world,player,100,at+30);
        yes(!f.consume(world,player,99,at+31),"Earlier tick refused");
        f.record(world,player,100,at+40);
        yes(!f.consume(world,player,100,
            at+40+SourceAttackEventFence.MAX_EVENT_AGE_NANOS+1),
            "Expired receipt refused");
        f.record(world,player,100,at+50);
        yes(!f.consume(world,player,100,at+49),"Clock inversion refused");
        f.record(world,player,100,at+60);
        f.reconcile(other,player);
        yes(f.pending()==0,"World replacement clears receipt");
        f.record(world,player,100,at+70);
        f.reconcile(world,other);
        yes(f.pending()==0,"Player replacement clears receipt");
        f.record(world,player,100,at+80);
        f.reset();
        yes(f.pending()==0,"Disable revokes outstanding receipt");
        refuses(()->f.record(null,player,100,at),"Null world rejected");
        refuses(()->f.record(world,player,-1,at),"Negative tick rejected");
        refuses(()->f.record(world,player,Integer.MAX_VALUE,at),
            "Tick overflow rejected");
        final boolean[] foreign={false};
        Thread t=new Thread(()->{
            try{f.record(world,player,100,at);}
            catch(IllegalStateException expected){foreign[0]=true;}
        });
        t.start();t.join();
        yes(foreign[0],"Foreign thread cannot arm receipt");
        yes(f.recorded()==9&&f.admitted()==1&&f.rejected()>=8,
            "Counters and rejection gate remain consistent");
        f.close();
        refuses(()->f.record(world,player,100,at),"Closed observer never rearms");
        System.out.println("R354_FORGE_ATTACK_EVENT_FENCE_JAVA8_PASS");
    }
}
