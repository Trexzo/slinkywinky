package dev.slinkywinky;

/**
 * R204: a read-only Java 8 host boundary to native Sprint decision proposals.
 * This class never calls Minecraft, presses keys, changes movement or installs
 * JNI/JVMTI hooks. Call only on the admitted game/preview owner thread.
 */
public final class SprintObservationProbe implements AutoCloseable {
    static { System.loadLibrary("slinkywinky_jni"); }

    private static native long nativeCreate();
    private static native float[] nativeObserve(
        long handle, long[] clocks, int[] flags,
        double[] coordinates, float[] rotation, float[] movement);
    private static native void nativeClose(long handle);

    public static final class Decision {
        public final boolean admitted, worldReady, sessionTransition;
        public final boolean proposedKeyPressed, hasSprinting, proposedSprinting;
        public final boolean hasSpeed, proposeSetSprinting;
        public final float proposedSpeed;

        private Decision(float[] values) {
            if (values == null || values.length != 9)
                throw new IllegalStateException("Invalid Sprint native decision length");
            for (float value : values)
                if (Float.isNaN(value) || Float.isInfinite(value))
                    throw new IllegalStateException("Nonfinite Sprint preview decision");
            for (int i = 0; i < values.length; i++)
                if (i != 7 && values[i] != 0 && values[i] != 1)
                    throw new IllegalStateException("Malformed boolean Sprint decision");
            admitted = values[0] != 0;
            worldReady = values[1] != 0;
            sessionTransition = values[2] != 0;
            proposedKeyPressed = values[3] != 0;
            hasSprinting = values[4] != 0;
            proposedSprinting = values[5] != 0;
            hasSpeed = values[6] != 0;
            proposedSpeed = values[7];
            proposeSetSprinting = values[8] != 0;
        }
    }

    private final Thread owner = Thread.currentThread();
    private long handle;

    public SprintObservationProbe() {
        handle = nativeCreate();
        if (handle == 0) throw new IllegalStateException("Native Sprint preview unavailable");
    }

    /**
     * flags: [observationFlags, guiOpen, alreadySprinting, usingItem,
     *         whileInGui, whileUsingItem, whileBackwards, whileSideways].
     * clocks: [monotonicMs, worldGeneration], coords: eye and ray target XYZ.
     * movement: [forward, strafe, speed]. Output is a decision, never an action.
     */
    public synchronized Decision observe(
            long[] clocks, int[] flags, double[] coordinates,
            float[] rotation, float[] movement) {
        assertOwner();
        if (handle == 0) throw new IllegalStateException("Sprint preview closed");
        if (clocks == null || clocks.length != 2 ||
            flags == null || flags.length != 8 ||
            coordinates == null || coordinates.length != 6 ||
            rotation == null || rotation.length != 2 ||
            movement == null || movement.length != 3)
            throw new IllegalArgumentException("Incorrect read-only Sprint snapshot shape");
        return new Decision(nativeObserve(handle, clocks, flags,
                                          coordinates, rotation, movement));
    }

    private void assertOwner() {
        if (Thread.currentThread() != owner)
            throw new IllegalStateException("Sprint preview called from non-owner thread");
    }

    @Override public synchronized void close() {
        assertOwner();
        if (handle != 0) {
            nativeClose(handle);
            handle = 0;
        }
    }
}
