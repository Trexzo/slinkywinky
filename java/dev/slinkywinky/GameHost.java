package dev.slinkywinky;

/**
 * Host-facing Java 8 session manager. A game mod supplies observations and actions.
 * Does not attach to Minecraft, inject code, or manipulate unowned processes.
 */
public final class GameHost implements AutoCloseable {
    public interface Actions {
        /** Applies an ordered decision on the exact game thread. */
        void apply(NativeEngine.Decision decision);
        /** Discards all *synthetic* input; never overrides physical input. */
        void releaseSynthetic();
    }
    public static final class Frame {
        public Object worldToken, playerToken;
        public long monotonicMs;
        public boolean guiOpen, manualLeft, manualRight, weapon, sprinting;
        public int hurtTime, lastAttackerTime, targetId = -1;
        public boolean targetPresent, targetVisible, targetAlive;
        public double eyeX, eyeY, eyeZ, targetX, targetY, targetZ;
        public float yaw, pitch;
        /** 19 fields described by docs/HOST_ABI.md. */
        public float[] settings;
    }

    private final NativeEngine engine;
    private final Actions actions;
    private final Thread owner = Thread.currentThread();
    private Object currentWorld, currentPlayer;
    private long generation, previousMs = -1;
    private boolean closed;

    public GameHost(NativeEngine engine, Actions actions) {
        if (engine == null || actions == null) throw new IllegalArgumentException("Missing engine or actions");
        this.engine = engine;
        this.actions = actions;
    }
    private void assertOwner() {
        if (owner != Thread.currentThread()) throw new IllegalStateException("Wrong game thread");
        if (closed) throw new IllegalStateException("GameHost is closed");
    }
    public long generation() { assertOwner(); return generation; }
    public void tick(Frame f) {
        assertOwner();
        if (f == null) {
            failClosed();
            throw new IllegalArgumentException("Missing frame");
        }
        if (f.monotonicMs < 0 || (previousMs >= 0 && f.monotonicMs < previousMs)) {
            failClosed();
            throw new IllegalArgumentException("Non-monotonic frame clock");
        }
        previousMs = f.monotonicMs;
        if (f.worldToken == null || f.playerToken == null) {
            // Already idle: avoid redundant native calls and noisy releases every render frame.
            if (currentWorld != null || currentPlayer != null) transition();
            return;
        }
        if (currentWorld != f.worldToken || currentPlayer != f.playerToken) {
            transition();
            currentWorld = f.worldToken;
            currentPlayer = f.playerToken;
            ++generation;
        }
        if (f.settings == null || f.settings.length != 19) {
            failClosed();
            throw new IllegalArgumentException("Invalid settings");
        }
        // Reject invalid/unmapped settings before allowing any side effects.
        try { new HostSettings(f.settings); }
        catch (IllegalArgumentException invalid) { failClosed(); throw invalid; }
        // Source position and target admission remain host responsibilities.
        int[] flags = {1, bit(f.guiOpen), bit(f.manualLeft), bit(f.manualRight),
            bit(f.weapon), bit(f.sprinting), f.hurtTime, f.lastAttackerTime,
            bit(f.targetPresent), bit(f.targetVisible), bit(f.targetAlive), f.targetId};
        double[] coords = {f.eyeX, f.eyeY, f.eyeZ, f.targetX, f.targetY, f.targetZ};
        try {
            NativeEngine.Decision output = engine.stepDetailed(
                new long[]{f.monotonicMs, generation}, flags, coords,
                new float[]{f.yaw, f.pitch}, f.settings);
            actions.apply(output);
        } catch (RuntimeException | Error error) {
            failClosed();
            throw error;
        }
    }
    private static int bit(boolean b) { return b ? 1 : 0; }
    private void transition() {
        // Cannot let a previous player/world hold a synthetic key in a new generation.
        try { engine.disable(false, false); }
        finally { actions.releaseSynthetic(); currentWorld = currentPlayer = null; }
    }
    private void failClosed() {
        try { transition(); }
        finally { currentWorld = currentPlayer = null; }
    }
    @Override public void close() {
        assertOwner();
        try { failClosed(); }
        finally {
            closed = true;
            engine.close();
        }
    }
}
