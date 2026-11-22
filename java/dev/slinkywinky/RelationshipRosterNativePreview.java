package dev.slinkywinky;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.UUID;

/**
 * R282: immutable Java8 roster receipt -> native Anti Bot preview.
 * Never updates Minecraft entities, RelationshipService or packets.
 * Teams remains disabled until scoreboard/display evidence is captured.
 */
public final class RelationshipRosterNativePreview implements AutoCloseable {
    private static final int CONTRACT_VERSION=1;
    static {System.loadLibrary("slinkywinky_jni");}
    private static native long nativeCreate();
    private static native long[] nativeObserve(
        long handle,long[] owner,long[] uuids,String[] names,
        double[] positions,int[] facts,int flags,int uuidMode,float minAge);
    private static native void nativeClose(long handle);
    public static final class Settings {
        public final boolean alwaysStationary,sleeping,alwaysInvisible,invalidUsername;
        public final int invalidUuidMode;
        public final float minimumAge;
        public Settings(boolean stationary,int uuidMode,boolean sleeping,
                        boolean invis,float minAge,boolean invalidUsername) {
            if(uuidMode<0 || uuidMode>2 || Float.isNaN(minAge) ||
               Float.isInfinite(minAge) || minAge<0f)
                throw new IllegalArgumentException("Malformed Anti Bot settings");
            alwaysStationary=stationary;invalidUuidMode=uuidMode;
            this.sleeping=sleeping;alwaysInvisible=invis;
            minimumAge=minAge;this.invalidUsername=invalidUsername;
        }
        private int mask() {
            return (alwaysStationary?1:0) | (sleeping?2:0) |
                   (alwaysInvisible?4:0) | (invalidUsername?8:0);
        }
    }
    public static final class RemoteDecision {
        public final UUID uuid;
        public final boolean rejected;
        private RemoteDecision(UUID id,boolean rejected) {
            uuid=id;this.rejected=rejected;
        }
    }
    public static final class Result {
        public final long worldToken,ownerToken,generation,tick;
        public final List<RemoteDecision> remote;
        private Result(RelationshipRosterSampler.Receipt r,
                       List<RemoteDecision> decisions) {
            worldToken=r.worldToken;ownerToken=r.ownerToken;
            generation=r.generation;tick=r.tick;
            remote=Collections.unmodifiableList(
                new ArrayList<RemoteDecision>(decisions));
        }
    }
    private final Thread ownerThread=Thread.currentThread();
    private long handle;
    public RelationshipRosterNativePreview() {
        handle=nativeCreate();
        if(handle<=0)throw new IllegalStateException("Native roster session unavailable");
    }
    private void assertOwner() {
        if(handle==0 || Thread.currentThread()!=ownerThread)
            throw new IllegalStateException("Roster native session not owned");
    }
    public Result observe(RelationshipRosterSampler.Receipt receipt,
                          Settings settings) {
        assertOwner();
        if(receipt==null || settings==null || receipt.ownerUuid==null ||
           receipt.worldToken<=0 || receipt.ownerToken<=0 ||
           receipt.generation<=0 || receipt.tick<0 || receipt.remote==null ||
           receipt.remote.size()>256)
            throw new IllegalArgumentException("Malformed roster receipt");
        final int count=receipt.remote.size();
        final long[] owner={receipt.worldToken,receipt.ownerToken,
            receipt.generation,receipt.tick,
            receipt.ownerUuid.getMostSignificantBits(),
            receipt.ownerUuid.getLeastSignificantBits()};
        final long[] uuids=new long[2*count];
        final String[] names=new String[count];
        final double[] motion=new double[4*count];
        final int[] facts=new int[4*count];
        for(int i=0;i<count;++i) {
            RelationshipRosterSampler.Remote p=receipt.remote.get(i);
            if(p==null || p.uuid==null || p.name==null)
                throw new IllegalArgumentException("Incomplete remote roster");
            uuids[2*i]=p.uuid.getMostSignificantBits();
            uuids[2*i+1]=p.uuid.getLeastSignificantBits();
            names[i]=p.name;
            motion[4*i]=p.prevX;motion[4*i+1]=p.prevZ;
            motion[4*i+2]=p.x;motion[4*i+3]=p.z;
            facts[4*i]=p.ticks;facts[4*i+1]=p.sleeping?1:0;
            facts[4*i+2]=p.invisible?1:0;
            facts[4*i+3]=p.armor?1:0;
        }
        final long[] values=nativeObserve(
            handle,owner,uuids,names,motion,facts,
            settings.mask(),settings.invalidUuidMode,settings.minimumAge);
        if(values==null || values.length!=6+3*count ||
           values[0]!=CONTRACT_VERSION || values[1]!=receipt.worldToken ||
           values[2]!=receipt.ownerToken || values[3]!=receipt.generation ||
           values[4]!=receipt.tick || values[5]!=count)
            throw new IllegalStateException("Unexpected native roster receipt contract");
        List<RemoteDecision> results=new ArrayList<RemoteDecision>();
        for(int i=0;i<count;++i) {
            if(values[6+3*i]!=uuids[2*i] ||
               values[6+3*i+1]!=uuids[2*i+1] ||
               (values[6+3*i+2]!=0 && values[6+3*i+2]!=2))
                throw new IllegalStateException("Native UUID or bit-2 proposal mismatch");
            results.add(new RemoteDecision(receipt.remote.get(i).uuid,
                                          values[6+3*i+2]==2));
        }
        return new Result(receipt,results);
    }
    @Override public void close() {
        assertOwner();
        long current=handle;
        nativeClose(current);
        handle=0;
    }
}
