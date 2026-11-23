package dev.slinkywinky;

import java.util.*;
import java.util.concurrent.atomic.AtomicLong;

/** R300: read-only, immutable owner-thread Minecraft world receipts for ESP.
 *  The original event-0/event-18 schedules and actual GL remain unbound. */
public final class EspWorldSnapshotSampler implements AutoCloseable {
    public static final int MAX_TILES=4096,MAX_ENTITIES=8192,MAX_PLAYERS=512;
    private static final AtomicLong IDS=new AtomicLong(1);
    public interface Port {
        boolean onClientThread();
        Object worldIdentity(); Object playerIdentity();
        Position ownerPosition();
        List<Tile> tiles(); List<Drop> drops(); List<Player> players();
    }
    public static final class Position {
        public final double x,y,z;
        public Position(double x,double y,double z){this.x=x;this.y=y;this.z=z;}
    }
    public static final class Tile {
        public final Position position; public final int blockId;
        public Tile(Position p,int id){position=p;blockId=id;}
    }
    public static final class Drop {
        public final Position previous,position;
        public final int itemId,stackSize;
        public final boolean dead,stackPresent,itemPresent;
        public Drop(Position old,Position now,int id,int count,
                    boolean dead,boolean hasStack,boolean hasItem){
            previous=old;position=now;itemId=id;stackSize=count;
            this.dead=dead;stackPresent=hasStack;itemPresent=hasItem;
        }
    }
    public static final class Player {
        public final UUID uuid; public final Position previous,position;
        public final boolean self,dead;
        public final float health,maxHealth,absorption;
        public final int hurtTime;
        public Player(UUID id,Position old,Position now,boolean self,boolean dead,
                      float health,float maxHealth,float absorption,int hurtTime){
            uuid=id;previous=old;position=now;this.self=self;this.dead=dead;
            this.health=health;this.maxHealth=maxHealth;
            this.absorption=absorption;this.hurtTime=hurtTime;
        }
    }
    public static final class Receipt {
        public final long worldToken,playerToken,generation,tick;
        public final Position ownerPosition;
        public final List<Tile> tiles; public final List<Drop> drops;
        public final List<Player> players;
        private Receipt(long w,long p,long g,long t,Position self,
                        List<Tile> tiles,List<Drop> drops,List<Player> players){
            worldToken=w;playerToken=p;generation=g;tick=t;ownerPosition=self;
            this.tiles=Collections.unmodifiableList(new ArrayList<Tile>(tiles));
            this.drops=Collections.unmodifiableList(new ArrayList<Drop>(drops));
            this.players=Collections.unmodifiableList(new ArrayList<Player>(players));
        }
    }
    private final Thread thread=Thread.currentThread();
    private Object world,player;
    private long worldToken,playerToken,generation,lastTick=-1;
    private boolean closed;
    private void owned(){
        if(closed||Thread.currentThread()!=thread)
            throw new IllegalStateException("R300 owner thread/closed");
    }
    private static boolean finite(double d){return !Double.isNaN(d)&&!Double.isInfinite(d);}
    private static void valid(Position p){
        if(p==null||!finite(p.x)||!finite(p.y)||!finite(p.z))
            throw new IllegalStateException("R300 invalid coordinate");
    }
    private static void valid(float f){
        if(Float.isNaN(f)||Float.isInfinite(f))
            throw new IllegalStateException("R300 invalid health");
    }
    private static void bounded(List<?> list,int max){
        if(list==null||list.size()>max)
            throw new IllegalStateException("R300 unbounded or missing observations");
    }
    /**
     * Preserve raw encounter order; each original module applies its own
     * eligibility filters, source event cadence, and 100-row limit later.
     * Never guess target/relationship color or publish live game identities.
     */
    public Receipt observe(Port port,long tick){
        owned();
        if(port==null||!port.onClientThread()||tick<0)
            throw new IllegalStateException("R300 not admitted");
        final Object w=port.worldIdentity(),p=port.playerIdentity();
        if(w==null||p==null)throw new IllegalStateException("R300 absent world/player");
        final boolean changed=w!=world||p!=player;
        if(!changed&&tick<=lastTick)
            throw new IllegalStateException("R300 duplicate/stale tick");
        final Position self=port.ownerPosition();valid(self);
        final List<Tile> tiles=port.tiles();bounded(tiles,MAX_TILES);
        for(Tile t:tiles){
            if(t==null||t.blockId<0)throw new IllegalStateException("R300 tile");
            valid(t.position);
        }
        final List<Drop> drops=port.drops();bounded(drops,MAX_ENTITIES);
        for(Drop d:drops){
            if(d==null||d.itemId< -1||d.stackSize<0||
               (d.itemPresent&&(!d.stackPresent||d.itemId<0)))
                throw new IllegalStateException("R300 drop");
            valid(d.previous);valid(d.position);
        }
        final List<Player> players=port.players();bounded(players,MAX_PLAYERS);
        final Set<UUID> seen=new HashSet<UUID>();
        int selves=0;
        for(Player pl:players){
            if(pl==null||pl.uuid==null||!seen.add(pl.uuid)||pl.hurtTime<0)
                throw new IllegalStateException("R300 player identity");
            valid(pl.previous);valid(pl.position);
            valid(pl.health);valid(pl.maxHealth);valid(pl.absorption);
            if(pl.self)++selves;
        }
        if(selves!=1||!port.onClientThread()||
           port.worldIdentity()!=w||port.playerIdentity()!=p)
            throw new IllegalStateException("R300 owner changed mid-read");
        long nw=worldToken,np=playerToken,ng=generation;
        if(changed){
            nw=IDS.getAndAdd(2);np=nw+1;
            if(nw<=0||np<=0||ng==Long.MAX_VALUE)
                throw new IllegalStateException("R300 identity overflow");
            ++ng;
        }
        Receipt out=new Receipt(nw,np,ng,tick,self,tiles,drops,players);
        world=w;player=p;worldToken=nw;playerToken=np;generation=ng;lastTick=tick;
        return out;
    }
    /** Call on disconnect or world/player change before any stale draw usage. */
    public void retire(){
        owned();
        if(generation==Long.MAX_VALUE)
            throw new IllegalStateException("R300 epoch overflow");
        ++generation;world=player=null;
        worldToken=playerToken=0;lastTick=-1;
    }
    public long generation(){owned();return generation;}
    @Override public void close(){
        owned();closed=true;world=player=null;
        worldToken=playerToken=0;lastTick=-1;
    }
}
