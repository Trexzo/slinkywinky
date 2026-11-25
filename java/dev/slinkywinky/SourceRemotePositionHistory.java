package dev.slinkywinky;

import java.util.ArrayDeque;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * R342: finite history of REAL vanilla serverPosX/Y/Z observed on the
 * Minecraft client thread; S18/S0C/S0F absolute packet facts are admitted
 * only after matching the subsequently applied vanilla fixed-point position.
 * No entity/packet reference and no gameplay side effects.
 */
public final class SourceRemotePositionHistory {
    public static final int MAX_ENTITIES=32,MAX_SAMPLES=24;
    public static final int APPLIED_SERVER_POSITION=1,PACKET_CONFIRMED=2;
    public static final long MIN_AGE_NANOS=80_000_000L;
    public static final long MAX_AGE_NANOS=600_000_000L;
    public static final long PACKET_MATCH_NANOS=300_000_000L;
    public static final class Position {
        public final int entityId,source;
        public final long uniqueHigh,uniqueLow,timestampNanos,sequence;
        public final int serverX,serverY,serverZ;
        private Position(int id,long hi,long lo,long timestamp,long sequence,
                         int x,int y,int z,int src){
            entityId=id;uniqueHigh=hi;uniqueLow=lo;
            timestampNanos=timestamp;this.sequence=sequence;
            serverX=x;serverY=y;serverZ=z;source=src;
        }
        public double x(){return serverX/32.0d;}
        public double y(){return serverY/32.0d;}
        public double z(){return serverZ/32.0d;}
    }
    public static final class Historical {
        public final Position previous,current;
        public final long ageNanos;
        private Historical(Position previous,Position current,long age){
            this.previous=previous;this.current=current;ageNanos=age;
        }
    }
    private static final class Track {
        final long hi,lo;
        final ArrayDeque<Position> samples=new ArrayDeque<Position>(MAX_SAMPLES);
        Track(long hi,long lo){this.hi=hi;this.lo=lo;}
    }
    private static final class Absolute {
        final int x,y,z;
        final long timestamp;
        Absolute(int x,int y,int z,long t){this.x=x;this.y=y;this.z=z;timestamp=t;}
    }
    // Access-order LRU cap guarantees a hostile churn of entity IDs cannot
    // grow without bound or repeatedly allocate new unbounded history maps.
    private final LinkedHashMap<Integer,Track> tracks=
        new LinkedHashMap<Integer,Track>(MAX_ENTITIES,0.75f,true);
    private final LinkedHashMap<Integer,Absolute> unconfirmedPackets=
        new LinkedHashMap<Integer,Absolute>(MAX_ENTITIES,0.75f,true);
    private long acceptedPackets,confirmedPackets,unknownRelativeBarriers;
    private long samplingSequence,epoch=1;
    private boolean closed;
    private void alive(){if(closed)throw new IllegalStateException("Retired server history");}
    private static void finiteClock(long now){
        // System.nanoTime() has an arbitrary signed origin; zero/negative
        // values are valid and must never be rejected as missing.
    }
    private static <V> void bounded(Map<Integer,V> m){
        while(m.size()>MAX_ENTITIES)m.remove(m.keySet().iterator().next());
    }
    public long epoch(){alive();return epoch;}
    public int tracked(){alive();return tracks.size();}
    public long confirmed(){alive();return confirmedPackets;}
    public long barrierCount(){alive();return unknownRelativeBarriers;}
    public long packetCount(){alive();return acceptedPackets;}
    public void resetWorld(){
        alive();
        if(epoch==Long.MAX_VALUE)throw new IllegalStateException("History epoch exhausted");
        ++epoch;
        tracks.clear();unconfirmedPackets.clear();samplingSequence=0;
    }
    /**
     * Called on the CLIENT owner thread only, AFTER FIFO dequeue.
     * Absolute teleport/spawn packet coordinates are vanilla fixed/32.
     * Entity IDs are not trusted until the later vanilla serverPos sample.
     */
    public void packet(SourcePacketReadOnlyQueue.Observation o){
        alive();
        if(o==null)throw new IllegalArgumentException("Missing packet history fact");
        if(o.direction!=SourcePacketReadOnlyQueue.IN)return;
        if(o.kind==SourcePacketReadOnlyQueue.RELATIVE_UNRESOLVED){
            // S14 has NO public getEntityId() in MCP 1.8.9.
            // Clear unconfirmed packet claims, but previous vanilla-applied
            // samples remain authentic fixed-point server observations.
            unconfirmedPackets.clear();
            ++unknownRelativeBarriers;
            return;
        }
        if(o.kind==SourcePacketReadOnlyQueue.DESTROY_UNRESOLVED){
            // S13 may destroy several IDs at once. All old entity-ID
            // ownership must fail closed until the next vanilla observation.
            tracks.clear();unconfirmedPackets.clear();
            ++unknownRelativeBarriers;
            return;
        }
        if(o.kind!=SourcePacketReadOnlyQueue.ENTITY_ABSOLUTE_TELEPORT&&
           o.kind!=SourcePacketReadOnlyQueue.PLAYER_SPAWN_ABSOLUTE&&
           o.kind!=SourcePacketReadOnlyQueue.MOB_SPAWN_ABSOLUTE)return;
        if(!o.entityIdKnown||!o.moving||!Double.isFinite(o.x)||
           !Double.isFinite(o.y)||!Double.isFinite(o.z)||
           Math.abs(o.x)>10000000||Math.abs(o.y)>10000000||
           Math.abs(o.z)>10000000)
            throw new IllegalArgumentException("Unverifiable vanilla absolute packet");
        final int x=toFixed(o.x),y=toFixed(o.y),z=toFixed(o.z);
        // A spawn explicitly begins a new entity-ID lifecycle. Never
        // allow a previous inhabitant of the same ID to provide ghost data.
        if(o.kind!=SourcePacketReadOnlyQueue.ENTITY_ABSOLUTE_TELEPORT)
            tracks.remove(o.entityId);
        unconfirmedPackets.put(o.entityId,new Absolute(x,y,z,o.nanoTime));
        bounded(unconfirmedPackets);
        ++acceptedPackets;
    }
    private static int toFixed(double x){
        final double fixed=x*32.0d;
        if(!Double.isFinite(fixed)||fixed>Integer.MAX_VALUE||fixed<Integer.MIN_VALUE||
           Math.rint(fixed)!=fixed)
            throw new IllegalArgumentException("Nonfixed-point packet position");
        return (int)fixed;
    }
    /** Only actual client-applied Entity.serverPos integer triplets. */
    public Position applied(int entityId,long uuidHigh,long uuidLow,
                            int x,int y,int z,long atNanos){
        alive();finiteClock(atNanos);
        Track track=tracks.get(entityId);
        if(track==null||track.hi!=uuidHigh||track.lo!=uuidLow){
            track=new Track(uuidHigh,uuidLow);
            tracks.put(entityId,track);
            bounded(tracks);
        }
        final Position last=track.samples.peekLast();
        if(last!=null&&atNanos<last.timestampNanos)
            throw new IllegalArgumentException("Server sample clock regressed");
        int provenance=APPLIED_SERVER_POSITION;
        Absolute packet=unconfirmedPackets.get(entityId);
        if(packet!=null){
            final long age=atNanos-packet.timestamp;
            if(age>=0&&age<=PACKET_MATCH_NANOS&&
               x==packet.x&&y==packet.y&&z==packet.z){
                provenance=PACKET_CONFIRMED;
                ++confirmedPackets;
                unconfirmedPackets.remove(entityId);
            }else if(age>PACKET_MATCH_NANOS||age<0)
                unconfirmedPackets.remove(entityId);
        }
        if(last!=null&&last.serverX==x&&last.serverY==y&&last.serverZ==z){
            // A later absolute packet can confirm an already applied
            // position without inventing a second movement sample.
            if(provenance==PACKET_CONFIRMED&&last.source!=PACKET_CONFIRMED){
                track.samples.removeLast();
                Position upgraded=new Position(entityId,uuidHigh,uuidLow,
                    last.timestampNanos,last.sequence,x,y,z,PACKET_CONFIRMED);
                track.samples.addLast(upgraded);
                return upgraded;
            }
            return last;
        }
        if(samplingSequence==Long.MAX_VALUE)
            throw new IllegalStateException("Server history sequence exhausted");
        Position result=new Position(entityId,uuidHigh,uuidLow,atNanos,
                                     ++samplingSequence,x,y,z,provenance);
        track.samples.addLast(result);
        if(track.samples.size()>MAX_SAMPLES)track.samples.removeFirst();
        return result;
    }
    /** Past position must be strictly older than lookback; never extrapolate. */
    public Historical historical(int entityId,long uuidHigh,long uuidLow,
                                  long nowNanos,long minAgeNanos,long maxAgeNanos){
        alive();finiteClock(nowNanos);
        if(minAgeNanos<MIN_AGE_NANOS||maxAgeNanos>MAX_AGE_NANOS||
           maxAgeNanos<minAgeNanos)
            throw new IllegalArgumentException("Unbounded historical lookback");
        Track t=tracks.get(entityId);
        if(t==null||t.hi!=uuidHigh||t.lo!=uuidLow)return null;
        Position latest=null,previous=null;
        for(Position p:t.samples){
            final long age=nowNanos-p.timestampNanos;
            if(age>=minAgeNanos&&age<=maxAgeNanos){
                previous=latest;
                latest=p;
            }
        }
        if(latest==null)return null;
        if(previous==null)previous=latest;
        return new Historical(previous,latest,nowNanos-latest.timestampNanos);
    }
    public void retire(){closed=true;tracks.clear();unconfirmedPackets.clear();}
}
