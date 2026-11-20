package dev.slinkywinky.instrument;

/**
 * R258: read-only metadata envelope from the actual ObservedState.sample()
 * call on the admitted client tick thread. It is NOT Minecraft's Reach event,
 * NOT an original module event dispatcher, and deliberately has no gameplay
 * data for deciding Reach, Hitboxes or Ghost Hand.
 *
 * Only immutable primitive values are retained: no Minecraft, player, world,
 * entity, class-loader or reflection references survive capture/close.
 */
public final class R258Event8ReadOnlyObservation {
    public static final class Receipt {
        public final long sequence, generation, monotonicMs;
        public final int flags;
        public final double eyeX,eyeY,eyeZ,targetX,targetY,targetZ;
        public final float yaw,pitch;

        private Receipt(long seq,ObservedState.Frame frame) {
            sequence=seq;
            generation=frame.generation;
            monotonicMs=frame.timeMs;
            flags=frame.flags;
            eyeX=frame.xyz[0];eyeY=frame.xyz[1];eyeZ=frame.xyz[2];
            targetX=frame.xyz[3];targetY=frame.xyz[4];targetZ=frame.xyz[5];
            yaw=frame.angles[0];pitch=frame.angles[1];
        }
        public boolean worldAndPlayer() {return (flags&3)==3;}
        public boolean targetSelected() {return (flags&4)!=0;}
        // R258 has no native callback/action authority. The original
        // game sampler does NOT observe attacker-time, sprint eligibility,
        // held item category, friend flags or source module settings.
        public boolean hasAllSourceEvent8Inputs(){return false;}

        public String envelope() {
            // Only event/session metadata; never print eye/target locations.
            return "R258_EVENT8_OBS "+sequence+" "+generation+" "+flags+
                   " "+monotonicMs;
        }
    }

    private final Thread owner=Thread.currentThread();
    private long lastSequence,lastGeneration,lastTimeMs;
    private int lastFlags;
    private boolean closed;

    public Receipt capture(ObservedState.Frame source) {
        checkOwner();
        if(source==null || source.generation<=0 || source.timeMs<0 ||
           (source.flags!=0 && source.flags!=1 &&
            source.flags!=3 && source.flags!=7) ||
           source.xyz==null || source.xyz.length!=6 ||
           source.angles==null || source.angles.length!=2 ||
           lastSequence==Long.MAX_VALUE)
            throw new IllegalArgumentException("R258 malformed read-only game frame");
        for(double value:source.xyz) {
            if(!Double.isFinite(value))
                throw new IllegalArgumentException("R258 nonfinite coordinate");
        }
        for(float value:source.angles) {
            if(!Float.isFinite(value))
                throw new IllegalArgumentException("R258 nonfinite camera");
        }
        if(lastSequence>0 &&
           (source.generation<lastGeneration ||
            source.timeMs<lastTimeMs ||
            ((source.flags&3)!=(lastFlags&3) &&
             source.generation<=lastGeneration)))
            throw new IllegalArgumentException("R258 replayed or stale world");
        // Commit only after complete validation; rejected samples do not
        // consume sequence or cause an implicit owner/session reset.
        Receipt out=new Receipt(lastSequence+1,source);
        lastSequence=out.sequence;
        lastGeneration=out.generation;
        lastTimeMs=out.monotonicMs;
        lastFlags=out.flags;
        return out;
    }

    public long accepted() {
        checkOwner();
        return lastSequence;
    }
    public void close() {
        checkOwner();
        closed=true;
        lastSequence=lastGeneration=lastTimeMs=0;
        lastFlags=0;
    }
    private void checkOwner(){
        if(closed || Thread.currentThread()!=owner)
            throw new IllegalStateException("R258 foreign or closed source owner");
    }
}
