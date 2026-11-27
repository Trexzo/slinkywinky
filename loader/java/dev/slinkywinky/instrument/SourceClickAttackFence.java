package dev.slinkywinky.instrument;

/**
 * R362: one-shot observed Minecraft clickMouse-exit combat clock evidence.
 * This is a safe observation-only candidate, NOT a proof of an entity hit or
 * the recovered original Slinky PhaseHook invocation.
 */
public final class SourceClickAttackFence implements AutoCloseable {
    private final Thread owner=Thread.currentThread();
    private boolean closed,primed;
    private long generation;
    private int baselineTick,baselineAttack,emittedTick=-1;
    private long calls,accepted,rejected;

    private void owner(){
        if(closed||owner!=Thread.currentThread())
            throw new IllegalStateException("R362 closed/foreign click event owner");
    }
    public void prime(long worldGeneration,Vanilla189CombatClock.Sample clock){
        owner();
        if(worldGeneration<=0||clock==null){
            reset();return;
        }
        if(!primed||generation!=worldGeneration){
            reset();generation=worldGeneration;primed=true;
        }else if(clock.playerTicks<baselineTick||
                 clock.lastAttackerTime<baselineAttack){
            throw new IllegalStateException("R362 combat clock rolled back");
        }
        baselineTick=clock.playerTicks;
        baselineAttack=clock.lastAttackerTime;
    }
    public boolean onClick(long worldGeneration,Vanilla189CombatClock.Sample clock){
        owner();++calls;
        if(!primed||worldGeneration!=generation||clock==null){
            ++rejected;return false;
        }
        if(clock.playerTicks<baselineTick||
           clock.playerTicks>baselineTick+1||
           clock.lastAttackerTime<baselineAttack||
           clock.lastAttackerTime>clock.playerTicks)
            throw new IllegalStateException("R362 impossible post-click combat clock");
        // This method is called only at a verified normal RETURN from the
        // original clickMouse()V. No first-attach or preexisting hit replay.
        final boolean candidate=clock.playerTicks>0&&
            clock.lastAttackerTime==clock.playerTicks&&
            clock.lastAttackerTime>baselineAttack&&
            clock.playerTicks>emittedTick;
        if(candidate){
            emittedTick=clock.playerTicks;
            ++accepted;return true;
        }
        ++rejected;return false;
    }
    public long accepted(){owner();return accepted;}
    public long rejected(){owner();return rejected;}
    public long calls(){owner();return calls;}
    public void reset(){
        owner();
        primed=false;generation=0;baselineTick=baselineAttack=0;
        emittedTick=-1;
    }
    @Override public void close(){owner();reset();closed=true;}
}
