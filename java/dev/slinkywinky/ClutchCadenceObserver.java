package dev.slinkywinky;

/**
 * R252: value-only model of the recovered Clutch event-0 20/CPS timer.
 * PlayerTick.END can sample this but is not the original event-0 hook.
 * No press callback, synthetic input, world mutation or native action port.
 */
public final class ClutchCadenceObserver implements AutoCloseable {
    public static final class Step {
        public final long generation,tick;
        public final float timerAfter;
        public final boolean timerDue,wouldPress;
        public final ClutchFaceSampler.Pos adjacent;
        private Step(long generation,long tick,float timer,boolean due,
                     boolean press,ClutchFaceSampler.Pos next) {
            this.generation=generation;this.tick=tick;timerAfter=timer;
            timerDue=due;wouldPress=press;
            adjacent=next==null?null:
                new ClutchFaceSampler.Pos(next.x,next.y,next.z);
        }
    }
    private final Thread owner=Thread.currentThread();
    private Object world,player;
    private long generation,lastTick;
    private float timer;
    private boolean closed;

    private void require(boolean valid) {
        if(!valid) {
            close();
            throw new IllegalStateException("R252 Clutch cadence owner/evidence denied");
        }
    }
    private static boolean posEquals(ClutchFaceSampler.Pos a,
                                     ClutchFaceSampler.Pos b) {
        return a!=null && b!=null && a.x==b.x && a.y==b.y && a.z==b.z;
    }
    public Step advance(ClutchFaceSampler.Port port,long gen,long tick,
                        float cps,boolean ready,
                        ClutchFaceSampler.Pos priorTarget,boolean centered,
                        ClutchFaceSampler.Receipt receipt) {
        require(!closed && Thread.currentThread()==owner &&
            port!=null && port.onClientThread() &&
            port.worldIdentity()!=null && port.playerIdentity()!=null &&
            gen>0 && tick>lastTick && tick>0 &&
            Float.isFinite(cps) && cps>=1.0f && cps<=20.0f &&
            Float.isFinite(timer) && timer>=-1.0f && timer<=20.0f);
        if(world==null) {
            world=port.worldIdentity();player=port.playerIdentity();
            generation=gen;
        }
        require(port.worldIdentity()==world &&
            port.playerIdentity()==player && gen==generation);
        require((priorTarget==null && receipt==null) ||
            (priorTarget!=null && receipt!=null &&
             receipt.generation==gen && receipt.tick==tick &&
             posEquals(priorTarget,receipt.target) &&
             receipt.centered==centered));
        // Same float operations/order as the R247 native pure click gate.
        float next=timer;
        boolean due=true;
        if(next>0.0f) {
            next-=1.0f;
            if(next>0.0f)due=false;
        }
        if(due)next+=20.0f/cps;
        require(Float.isFinite(next) && next>=-1.0f && next<=20.0f);
        boolean press=due && ready && priorTarget!=null &&
                      receipt.matchesPlannedFace();
        // A matched receipt is a proposal only. It did NOT press or
        // change the target block in Minecraft.
        ClutchFaceSampler.Pos adjacent=press?receipt.adjacent:null;
        require(port.onClientThread() &&
            port.worldIdentity()==world && port.playerIdentity()==player);
        lastTick=tick;timer=next;
        return new Step(gen,tick,next,due,press,adjacent);
    }
    public float currentTimer() {return timer;}
    @Override public void close() {
        closed=true;world=null;player=null;
    }
}
