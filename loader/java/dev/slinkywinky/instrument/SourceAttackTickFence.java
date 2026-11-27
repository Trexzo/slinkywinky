package dev.slinkywinky.instrument;

/**
 * R360 single-use conservative attack-phase inference from exact player-clock
 * transitions. NOT the original event15 hook; no synthesis on first attach,
 * unchanged attack tick, duplicate game tick, world switch or disconnect.
 */
public final class SourceAttackTickFence implements AutoCloseable {
    private final Thread owner=Thread.currentThread();
    private boolean closed,haveBaseline;
    private long generation;
    private int priorTick,priorAttack;
    private long admitted,suppressed;

    private void owner(){
        if(closed||owner!=Thread.currentThread())
            throw new IllegalStateException("R360 closed or foreign combat clock fence");
    }
    public boolean observe(long worldGeneration,Vanilla189CombatClock.Sample clock){
        owner();
        if(worldGeneration<=0||clock==null){
            reset();return false;
        }
        if(!haveBaseline||generation!=worldGeneration){
            generation=worldGeneration;haveBaseline=true;
            priorTick=clock.playerTicks;priorAttack=clock.lastAttackerTime;
            ++suppressed;return false;
        }
        if(clock.playerTicks<priorTick||clock.lastAttackerTime<priorAttack)
            throw new IllegalStateException("R360 player clock reversed in same world");
        if(clock.playerTicks==priorTick){
            if(clock.lastAttackerTime!=priorAttack)
                throw new IllegalStateException("R360 attack changed without a player tick");
            ++suppressed;return false;
        }
        boolean candidate=clock.lastAttackerTime>0 &&
            clock.lastAttackerTime==clock.playerTicks &&
            clock.lastAttackerTime>priorAttack &&
            clock.playerTicks>priorTick;
        priorTick=clock.playerTicks;priorAttack=clock.lastAttackerTime;
        if(candidate)++admitted;else ++suppressed;
        return candidate;
    }
    public void reset(){
        owner();
        haveBaseline=false;generation=0;priorTick=priorAttack=0;
    }
    public long admitted(){owner();return admitted;}
    public long suppressed(){owner();return suppressed;}
    @Override public void close(){owner();reset();closed=true;}
}
