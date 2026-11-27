package dev.slinkywinky;

/**
 * R354: client-thread, one-entry, one-shot Forge attack-attempt observation.
 * This is not a native attack phase or a hit confirmation.
 * Identity references never cross JNI or the network.
 */
public final class SourceAttackEventFence implements AutoCloseable {
    public static final long MAX_EVENT_AGE_NANOS=150_000_000L;
    private final Thread owner=Thread.currentThread();
    private Object world,player;
    private int tick;
    private long observedNanos;
    private boolean pending,closed;
    private long recorded,admitted,rejected;

    private void owner(){
        if(closed||Thread.currentThread()!=owner)
            throw new IllegalStateException("R354 closed or foreign owner");
    }
    private void clear(){
        world=null;player=null;pending=false;tick=0;observedNanos=0;
    }
    public void record(Object w,Object p,int t,long now){
        owner();
        if(w==null||p==null||w==p||t<0||t==Integer.MAX_VALUE)
            throw new IllegalArgumentException("Invalid R354 attack identity");
        if(pending){
            if(world==w&&player==p&&tick==t)return; // same-tick coalescing
            ++rejected;clear();
        }
        world=w;player=p;tick=t;observedNanos=now;pending=true;++recorded;
    }
    public void reconcile(Object w,Object p){
        owner();
        if(pending&&(world!=w||player!=p)){
            ++rejected;clear();
        }
    }
    public boolean consume(Object w,Object p,int t,long now){
        owner();
        if(!pending)return false;
        final long age=now-observedNanos;
        final boolean ok=w==world&&p==player&&t>=tick&&t<=tick+1&&
            age>=0&&age<=MAX_EVENT_AGE_NANOS;
        clear();
        if(ok)++admitted;else ++rejected;
        return ok;
    }
    public void reset(){
        owner();
        if(pending)++rejected;
        clear();
    }
    public int pending(){owner();return pending?1:0;}
    public long recorded(){owner();return recorded;}
    public long admitted(){owner();return admitted;}
    public long rejected(){owner();return rejected;}
    @Override public void close(){
        owner();
        reset();closed=true;
    }
}
