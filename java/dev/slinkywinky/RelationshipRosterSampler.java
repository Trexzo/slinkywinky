package dev.slinkywinky;

import java.util.*;
import java.util.concurrent.atomic.AtomicLong;

/** R276 read-only, owner-thread Java8 roster sampler; no Minecraft imports. */
public final class RelationshipRosterSampler implements AutoCloseable {
    public interface Port {
        boolean onClientThread();
        Object worldIdentity();
        Object ownerIdentity();
        UUID ownerUuid();
        List<Observation> players();
    }
    /** Input observation holds only an ephemeral game-object identity. */
    public static final class Observation {
        public final Object identity;
        public final UUID uuid;
        public final String name;
        public final double prevX,prevZ,x,z;
        public final int ticks;
        public final boolean sleeping,invisible,armor;
        public Observation(Object id,UUID uuid,String name,
                double prevX,double prevZ,double x,double z,int ticks,
                boolean sleeping,boolean invisible,boolean armor) {
            identity=id;this.uuid=uuid;this.name=name;
            this.prevX=prevX;this.prevZ=prevZ;this.x=x;this.z=z;
            this.ticks=ticks;this.sleeping=sleeping;
            this.invisible=invisible;this.armor=armor;
        }
    }
    /** Output: no reference to any Minecraft player or world object. */
    public static final class Remote {
        public final UUID uuid;
        public final String name;
        public final double prevX,prevZ,x,z;
        public final int ticks;
        public final boolean sleeping,invisible,armor;
        private Remote(Observation o) {
            uuid=o.uuid;name=o.name;
            prevX=o.prevX;prevZ=o.prevZ;x=o.x;z=o.z;
            ticks=o.ticks;sleeping=o.sleeping;
            invisible=o.invisible;armor=o.armor;
        }
    }
    public static final class Receipt {
        public final long worldToken,ownerToken,generation,tick;
        public final UUID ownerUuid;
        public final List<Remote> remote;
        private Receipt(long world,long owner,long generation,long tick,
                        UUID uuid,List<Remote> players) {
            worldToken=world;ownerToken=owner;this.generation=generation;
            this.tick=tick;ownerUuid=uuid;
            remote=Collections.unmodifiableList(new ArrayList<Remote>(players));
        }
    }
    private static final AtomicLong NEXT_TOKEN=new AtomicLong(1L);
    private final Thread ownerThread=Thread.currentThread();
    private Object lastWorld,lastOwner;
    private UUID lastOwnerUuid;
    private long worldToken,ownerToken,generation,lastTick=-1L;
    private boolean closed;

    private static boolean finite(double x) {
        return !Double.isNaN(x)&&!Double.isInfinite(x);
    }
    public Receipt observe(Port port,long tick) {
        if(closed || port==null || Thread.currentThread()!=ownerThread ||
           !port.onClientThread())throw new IllegalStateException("not owned");
        if(tick<0)throw new IllegalArgumentException("negative tick");
        final Object world=port.worldIdentity(),owner=port.ownerIdentity();
        final UUID ownerId=port.ownerUuid();
        if(world==null||owner==null||ownerId==null)
            throw new IllegalStateException("absent world/owner");
        final boolean changed=world!=lastWorld||owner!=lastOwner||
                              !ownerId.equals(lastOwnerUuid);
        if(!changed && tick<lastTick)
            throw new IllegalStateException("stale tick");
        List<Observation> all=port.players();
        if(all==null || all.size()>256)
            throw new IllegalStateException("invalid roster size");
        List<Remote> remotes=new ArrayList<Remote>();
        Set<UUID> seen=new HashSet<UUID>();
        boolean foundOwner=false;
        for(Observation o:all) {
            if(o==null||o.identity==null||o.uuid==null||o.name==null||
               o.ticks<0||!finite(o.prevX)||!finite(o.prevZ)||
               !finite(o.x)||!finite(o.z))
                throw new IllegalStateException("invalid observation");
            if(!seen.add(o.uuid))
                throw new IllegalStateException("duplicate UUID");
            if(o.identity==owner) {
                if(!o.uuid.equals(ownerId))
                    throw new IllegalStateException("owner mismatch");
                foundOwner=true;
            } else {
                if(o.uuid.equals(ownerId))
                    throw new IllegalStateException("remote reused owner UUID");
                remotes.add(new Remote(o));
            }
        }
        if(!foundOwner || !port.onClientThread() ||
           world!=port.worldIdentity() || owner!=port.ownerIdentity() ||
           !ownerId.equals(port.ownerUuid()))
            throw new IllegalStateException("unowned changed roster");
        long w=worldToken,p=ownerToken,g=generation;
        if(changed) {
            w=NEXT_TOKEN.getAndAdd(2L);p=w+1L;
            if(w<=0L||p<=0L||g==Long.MAX_VALUE)
                throw new IllegalStateException("token exhausted");
            ++g;
        }
        Receipt out=new Receipt(w,p,g,tick,ownerId,remotes);
        lastWorld=world;lastOwner=owner;lastOwnerUuid=ownerId;
        worldToken=w;ownerToken=p;generation=g;lastTick=tick;
        return out;
    }
    public long generation(){return generation;}
    @Override public void close(){
        if(Thread.currentThread()!=ownerThread)
            throw new IllegalStateException("wrong close thread");
        closed=true;lastWorld=lastOwner=null;lastOwnerUuid=null;
        worldToken=ownerToken=0L;lastTick=-1L;
    }
}
