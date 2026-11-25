package dev.slinkywinky;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * R339: bounded thread-handoff between Netty's channel event loop and the
 * Minecraft client thread. Holds ONLY primitive facts, never Packet, Channel,
 * World, Entity, JNI refs, client-side actions, or callback objects.
 *
 * Every accepted snapshot has a strict FIFO sequence. Overflow poisons the
 * entire ingress (not merely an individual packet): a discontinuous stream
 * can NEVER be presented to stateful source controllers as complete.
 */
public final class SourcePacketReadOnlyQueue implements AutoCloseable {
    public static final int CAPACITY=256;
    public static final int IN=0,OUT=1;
    public static final int UNKNOWN=0;
    public static final int ENTITY_STATUS=1,UPDATE_HEALTH=2,SOUND=3;
    public static final int VELOCITY=4,TELEPORT=5,MOVEMENT=6,ATTACK=7;
    public static final int STOP_SPRINT=8,INV_OPEN=9,INV_CLICK=10,INV_CLOSE=11;
    public static final class Observation {
        public final int direction,kind,entityId,sequence;
        public final long nanoTime;
        public final boolean moving,entityIdKnown;
        public final double x,y,z;
        private Observation(int direction,int kind,int entity,int sequence,
                            long time,boolean moving,boolean idKnown,
                            double x,double y,double z) {
            this.direction=direction;this.kind=kind;entityId=entity;
            this.sequence=sequence;nanoTime=time;
            this.moving=moving;entityIdKnown=idKnown;
            this.x=x;this.y=y;this.z=z;
        }
    }
    public static final class Batch {
        public final List<Observation> rows;
        public final long dropped,totalCaptured;
        private Batch(List<Observation> rows,long dropped,long total){
            this.rows=Collections.unmodifiableList(rows);
            this.dropped=dropped;totalCaptured=total;
        }
    }
    private final Thread owner=Thread.currentThread();
    private final ArrayDeque<Observation> queue=new ArrayDeque<Observation>(CAPACITY);
    private boolean closed,failed;
    private long dropped,captured;
    private int nextSeq=1;
    /** May be invoked on ANY Netty event-loop thread. Never blocks network I/O. */
    public synchronized boolean offer(int direction,int kind,int entity,
                           boolean entityKnown,boolean hasPosition,
                           double x,double y,double z,long atNanos) {
        if(closed||failed)return false;
        if(direction!=IN&&direction!=OUT || kind<UNKNOWN||kind>INV_CLOSE||
           !Double.isFinite(x)||!Double.isFinite(y)||
           !Double.isFinite(z) || nextSeq==Integer.MAX_VALUE ||
           queue.size()==CAPACITY){
            poisonLocked();
            return false;
        }
        queue.addLast(new Observation(direction,kind,entity,nextSeq++,
                                      atNanos,hasPosition,entityKnown,x,y,z));
        ++captured;
        return true;
    }
    /** On callback parse failure: fail closed while allowing Netty to forward. */
    public synchronized void poison(){poisonLocked();}
    private void poisonLocked(){
        if(!failed){failed=true;dropped+=queue.size()+1L;queue.clear();}
    }
    public synchronized boolean healthy(){return !closed&&!failed;}
    public Batch drain(int maximum) {
        if(Thread.currentThread()!=owner)
            throw new IllegalStateException("Netty packets must replay on client owner");
        if(maximum<1||maximum>CAPACITY)
            throw new IllegalArgumentException("Unbounded packet replay request");
        synchronized(this){
            if(closed||failed)
                throw new IllegalStateException("Detached or incomplete packet stream");
            List<Observation> out=new ArrayList<Observation>(Math.min(maximum,queue.size()));
            while(out.size()<maximum&&!queue.isEmpty())out.add(queue.removeFirst());
            return new Batch(out,dropped,captured);
        }
    }
    public synchronized int pending(){return queue.size();}
    @Override public void close(){
        if(Thread.currentThread()!=owner)
            throw new IllegalStateException("Packet queue must retire on client owner");
        synchronized(this){
            closed=true;queue.clear();
        }
    }
}
