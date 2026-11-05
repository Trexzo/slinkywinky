package dev.slinkywinky;

/** Standalone JVM-to-native smoke boundary. No Minecraft classes or hooks. */
public final class NativeEngine implements AutoCloseable {
    static { System.loadLibrary("slinkywinky_jni"); }
    private static native int nativeVersion();
    private static native long nativeCreate(int[] seed);
    private static native float[] nativeStep(long handle, long timeMs, long generation,
        boolean world, boolean gui, boolean manualLeft, boolean weapon,
        float yaw, float pitch, double[] eye, double[] target, float[] config);
    private static native void nativeClose(long handle);
    private static native float[] nativeStepDetailed(long handle, long[] clocks,
        int[] flags, double[] coords, float[] rotation, float[] settings);
    private static native float[] nativeDisable(long handle, boolean manualLeft, boolean manualRight);

    /** Output copied from a native decision; never a direct game action. */
    public static final class Decision {
        public final int leftEdge, rightEdge;
        public final boolean leftHeld, rightHeld, attackPulse, hasRotation, hasReach;
        public final float yaw, pitch, reach;
        private Decision(float[] v) {
            if (v == null || v.length != 10) throw new IllegalStateException("Incorrect native decision size");
            for (float n : v) if (Float.isNaN(n) || Float.isInfinite(n))
                throw new IllegalStateException("Nonfinite native decision");
            leftEdge = (int)v[0]; rightEdge = (int)v[1];
            if (leftEdge < 0 || leftEdge > 2 || rightEdge < 0 || rightEdge > 2)
                throw new IllegalStateException("Invalid native input edge");
            leftHeld = v[2] != 0; rightHeld = v[3] != 0;
            attackPulse = v[4] != 0; hasRotation = v[5] != 0;
            yaw = v[6]; pitch = v[7]; hasReach = v[8] != 0; reach = v[9];
        }
    }

    /** Full host-facing entry point; caller must run on the exact admitted game thread. */
    public synchronized Decision stepDetailed(long[] clocks, int[] flags,
            double[] coords, float[] rotation, float[] settings) {
        checkOwner();
        if (handle == 0) throw new IllegalStateException("Engine closed");
        if (clocks == null || clocks.length != 2 || flags == null || flags.length != 12 ||
            coords == null || coords.length != 6 || rotation == null || rotation.length != 2 ||
            settings == null || settings.length != 19)
            throw new IllegalArgumentException("Incorrect native frame shape");
        for (double d : coords) if (!Double.isFinite(d)) throw new IllegalArgumentException("Nonfinite position");
        for (float f : rotation) if (!Float.isFinite(f)) throw new IllegalArgumentException("Nonfinite rotation");
        for (float f : settings) if (!Float.isFinite(f)) throw new IllegalArgumentException("Nonfinite setting");
        if (clocks[0] < 0 || clocks[1] <= 0) throw new IllegalArgumentException("Invalid frame clock");
        return new Decision(nativeStepDetailed(handle, clocks, flags, coords, rotation, settings));
    }

    public synchronized Decision disable(boolean manualLeft, boolean manualRight) {
        checkOwner();
        if (handle == 0) throw new IllegalStateException("Engine closed");
        return new Decision(nativeDisable(handle, manualLeft, manualRight));
    }
    private long handle;
    private final Thread owner;

    public NativeEngine(int[] seed) {
        if (seed == null || seed.length != 4) throw new IllegalArgumentException("seed length must be 4");
        owner = Thread.currentThread();
        handle = nativeCreate(seed);
        if (handle == 0) throw new IllegalStateException("Could not initialize native engine");
    }
    public static int abiVersion() { return nativeVersion(); }
    /** Config: [clickerEnabled, cps, aimEnabled, aimRange, aimHSpeed, aimVSpeed, aimFov].
     * Pass null for target when the game has no valid target entity. */
    public synchronized float[] step(long timeMs, long generation,
         boolean world, boolean gui, boolean manualLeft, boolean weapon,
         float yaw, float pitch, double[] eye, double[] target, float[] config) {
        checkOwner();
        if (handle == 0) throw new IllegalStateException("Engine was closed");
        if (eye == null || eye.length != 3 || (target != null && target.length != 3) ||
            config == null || config.length != 7) throw new IllegalArgumentException("Invalid snapshot lengths");
        return nativeStep(handle,timeMs,generation,world,gui,manualLeft,weapon,
                          yaw,pitch,eye,target,config);
    }
    private void checkOwner() {
        if (Thread.currentThread() != owner) throw new IllegalStateException("Called from wrong JVM thread");
    }
    @Override public synchronized void close() {
        checkOwner();
        if (handle != 0) { nativeClose(handle); handle = 0; }
    }
}
