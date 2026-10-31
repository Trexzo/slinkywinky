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
