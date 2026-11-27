package dev.slinkywinky;

/**
 * R353 real native Lag Range source e15 -> e18 -> e3 JNI.
 * No game input, packets, network ownership or forged source events.
 */
public final class SourceLagRangeAttackPhaseSmoke {
    static void yes(boolean condition,String message){
        if(!condition)throw new AssertionError(message);
    }
    static int[] tickBits(){
        // LagRange tick hit flag remains 0; ONLY e15 may supply the hit.
        return new int[]{1,1,1,0,0,0,0,0,0,0,0,0};
    }
    static double[] tickFields(){return new double[]{9,10,10,0,0,150};}
    static SourceRuntimePreview.Receipt outgoingAttack(SourceRuntimePreview source,
                                  long w,long p,long gen,long tick){
        return source.observe(w,p,gen,tick,SourceRuntimePreview.LAG_RANGE_PACKET,
            new int[]{2,1,0,1},new double[]{0,0,0,0});
    }
    static final class Port implements SourceWorldReadOnlyHost.AttackPort {
        Object world=new Object(),player=new Object();
        boolean current=true;
        SourceAttackEventFence attackFence;
        int playerTick=100;
        public boolean admittedClientThread(){return true;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public boolean sourceLagRangeAttackNow(){
            return (attackFence==null||attackFence.consume(world,player,
                playerTick,System.nanoTime()))&&current;
        }
        public SourceWorldReadOnlyHost.Frame capture(){
            return new SourceWorldReadOnlyHost.Frame(new int[][]{
                {1,1,0,100,0,0,1,0,0,0,0,0},
                tickBits(),
                {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0},
                {1,0,0,0,0,0,0,0,0,0,1,1}
            },new double[][]{
                {0,0,120,3,0,6,10},tickFields(),
                {0,0,0,5,3,2,0.5},{4}
            });
        }
    }
    public static void main(String[] args)throws Exception{
        SourceRuntimePreview nativeSource=new SourceRuntimePreview();
        final long w=1301,p=1302;
        SourceRuntimePreview.Receipt e15=nativeSource.observe(w,p,1,1,
            SourceRuntimePreview.LAG_RANGE_ATTACK,new int[]{1,1},new double[0]);
        yes(e15.sourceEvent==15&&e15.at(2)==1&&e15.at(3)==1,
            "Original registered native Lag Range e15 executed");
        nativeSource.observe(w,p,1,1,SourceRuntimePreview.LAG_RANGE_TICK,
            tickBits(),tickFields());
        yes(outgoingAttack(nativeSource,w,p,1,1).at(4)==1,
            "Native event15 hit state drives event18 attack-release decision");

        // A new native world must clear the prior phase-1 hit state.
        nativeSource.observe(w+2,p+2,2,1,SourceRuntimePreview.LAG_RANGE_TICK,
            tickBits(),tickFields());
        yes(outgoingAttack(nativeSource,w+2,p+2,2,1).at(4)==0,
            "No phantom event15 across world generation");

        // Phase other than 1 is explicitly rejected as a hit signal.
        SourceRuntimePreview.Receipt wrongPhase=nativeSource.observe(w+4,p+4,3,1,
            SourceRuntimePreview.LAG_RANGE_ATTACK,new int[]{0,1},new double[0]);
        yes(wrongPhase.at(2)==1&&wrongPhase.at(3)==0,
            "Original native event15 phase 0 produces no hit");
        nativeSource.observe(w+4,p+4,3,1,SourceRuntimePreview.LAG_RANGE_TICK,
            tickBits(),tickFields());
        yes(outgoingAttack(nativeSource,w+4,p+4,3,1).at(4)==0,
            "Wrong attack phase cannot trigger a release");
        boolean invalid=false;
        try{
            nativeSource.observe(w+4,p+4,3,1,SourceRuntimePreview.LAG_RANGE_ATTACK,
                new int[]{2,1},new double[0]);
        }catch(IllegalArgumentException expected){invalid=true;}
        yes(invalid,"Invalid native source phase bool rejected");
        final boolean[] foreign={false};
        Thread other=new Thread(()->{
            try{
                nativeSource.observe(w+4,p+4,3,1,
                    SourceRuntimePreview.LAG_RANGE_ATTACK,new int[]{1,1},new double[0]);
            }catch(IllegalStateException expected){foreign[0]=true;}
        });
        other.start();other.join();
        yes(foreign[0],"Foreign thread cannot invoke native attack event");
        nativeSource.close();

        // Same JNI owner session inside real SourceWorldReadOnlyHost observer.
        SourceRuntimePreview nativeHost=new SourceRuntimePreview();
        SourceWorldReadOnlyHost host=new SourceWorldReadOnlyHost(nativeHost);
        Port port=new Port();
        // R354: no source e15 without an owned, recent Forge attempt.
        SourceAttackEventFence attackFence=new SourceAttackEventFence();
        port.attackFence=attackFence;
        port.current=true;
        SourceWorldReadOnlyHost.TickReceipt unarmed=host.observe(port);
        yes(unarmed.lagRangeAttack==null,"No attack attempt means no native e15");
        attackFence.record(port.world,port.player,port.playerTick,System.nanoTime());
        ++port.playerTick;
        SourceWorldReadOnlyHost.TickReceipt received=host.observe(port);
        yes(attackFence.admitted()==1&&attackFence.pending()==0,
            "Host's real AttackPort consumed one owned Forge event");
        yes(received.lagRangeAttack!=null&&received.lagRangeAttack.sourceEvent==15&&
            received.lagRangeAttack.at(2)==1&&
            received.lagRangeAttack.tick==received.lagRange.tick,
            "Owner host sends JNI event15 before event18 of same tick");
        yes(outgoingAttack(nativeHost,received.lagRange.world,received.lagRange.player,
            received.generation,received.tick).at(4)==1,
            "Host-backed native attack state reaches packet release model");
        attackFence.record(port.world,port.player,port.playerTick,System.nanoTime());
        port.world=new Object();port.current=false;++port.playerTick;
        SourceWorldReadOnlyHost.TickReceipt switched=host.observe(port);
        yes(attackFence.pending()==0&&attackFence.rejected()>=1,
            "World replacement rejected stale pending Forge attack event");
        yes(switched.lagRangeAttack==null&&switched.generation==received.generation+1,
            "World replacement refuses previous attack epoch");
        yes(outgoingAttack(nativeHost,switched.lagRange.world,switched.lagRange.player,
            switched.generation,switched.tick).at(4)==0,
            "New world never retains old attack release state");
        host.close();attackFence.close();
        System.out.println("R353_NATIVE_LAG_RANGE_EVENT15_EVENT18_JNI_PASS");
    }
}
