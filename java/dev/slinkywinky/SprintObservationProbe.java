package dev.slinkywinky;

import java.util.Arrays;

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
    private static native void nativeSetEnabled(long handle, boolean enabled);
    private static native void nativeSetSettings(long handle, int mask);
    private static native long[] nativeControlState(long handle);
    private static native void nativeSetModuleEnabled(
        long handle, String canonicalName, boolean enabled);
    private static native long[] nativeModuleControlState(
        long handle, String canonicalName);
    private static native void nativeSetNoSlowSettings(
        long handle, float bow, float sword, float consumable);
    private static native float[] nativeObserveModules(
        long handle, long[] clocks, int[] flags,
        double[] coordinates, float[] rotation, float[] movement,
        int[] moduleContext, float[] passiveNoSlowSettings);

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

    // Canonical name is shared with the native module manager and
    // the first-party module_control_contract.hpp, not the GUI demo flag.
    public static final String MODULE_NAME = "Sprint";
    public static final String NO_JUMP_DELAY = "No Jump Delay";
    public static final String NO_SLOW = "No Slow";
    public static final String[] CONTROLLED_MODULES = {
        NO_JUMP_DELAY, NO_SLOW, MODULE_NAME
    };

    public static final class ControlState {
        public final boolean requestedEnabled, activeInWorld, settingsOverride;
        public final long worldGeneration;
        public final String moduleName;

        private ControlState(long[] values, String name) {
            if (values == null || values.length != 5 || values[0] != 1 ||
                values[4] < 0)
                throw new IllegalStateException("Unsupported native module-control contract");
            for (int i = 1; i <= 3; ++i)
                if (values[i] != 0 && values[i] != 1)
                    throw new IllegalStateException("Malformed native module state");
            moduleName = name;
            requestedEnabled = values[1] == 1;
            activeInWorld = values[2] == 1;
            settingsOverride = values[3] == 1;
            worldGeneration = values[4];
        }
    }

    /**
     * Owner-thread-only request. A GUI must use the state() snapshot,
     * never assume a button click itself enabled a native module.
     */
    public synchronized void setEnabled(boolean enabled) {
        assertOwner();
        assertOpen();
        nativeSetEnabled(handle, enabled);
    }

    /**
     * Canonical Sprint controls: in GUI, using an item, backward,
     * sideways. These replace older passive/preview-only flag values.
     * Neither this method nor any native policy directly changes the game.
     */
    public synchronized void setSettings(
            boolean inGui, boolean usingItem,
            boolean backwards, boolean sideways) {
        assertOwner();
        assertOpen();
        final int mask = (inGui ? 1 : 0) | (usingItem ? 2 : 0) |
                         (backwards ? 4 : 0) | (sideways ? 8 : 0);
        nativeSetSettings(handle, mask);
    }

    public synchronized ControlState state() {
        assertOwner();
        assertOpen();
        return new ControlState(nativeControlState(handle), MODULE_NAME);
    }

    private void assertOpen() {
        if (handle == 0)
            throw new IllegalStateException("Sprint module-control session closed");
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

    /**
     * Canonical module toggle, acknowledged by the owner-thread native
     * event manager; never modifies Minecraft input or motion directly.
     */
    public synchronized void setModuleEnabled(String name, boolean enabled) {
        assertOwner();
        assertOpen();
        requireKnownModule(name);
        nativeSetModuleEnabled(handle, name, enabled);
    }

    public synchronized ControlState moduleState(String name) {
        assertOwner();
        assertOpen();
        requireKnownModule(name);
        return new ControlState(nativeModuleControlState(handle, name), name);
    }

    public synchronized ControlState[] moduleStates() {
        assertOwner();
        assertOpen();
        ControlState[] result = new ControlState[CONTROLLED_MODULES.length];
        for (int i = 0; i < result.length; ++i) {
            String name = CONTROLLED_MODULES[i];
            result[i] = new ControlState(
                nativeModuleControlState(handle, name), name);
        }
        return result;
    }

    public synchronized void setNoSlowSettings(
            float bow, float sword, float consumable) {
        assertOwner();
        assertOpen();
        if (Float.isNaN(bow) || Float.isInfinite(bow) || bow < 0 ||
            Float.isNaN(sword) || Float.isInfinite(sword) || sword < 0 ||
            Float.isNaN(consumable) || Float.isInfinite(consumable) ||
            consumable < 0)
            throw new IllegalArgumentException("Invalid No Slow multiplier");
        nativeSetNoSlowSettings(handle, bow, sword, consumable);
    }

    /**
     * The legacy observe() remains the 9-float Sprint-only JNI ABI.
     * The extended typed ABI adds No Jump Delay and No Slow observations
     * to the SAME admitted native event dispatch. moduleContext fields:
     * [inGame, observedJumpTicks, usingItem, heldItemPresent, itemId].
     * In-game/usingItem values must agree with the existing flags.
     */
    public synchronized ModuleDecision observeModules(
            long[] clocks, int[] flags, double[] coordinates,
            float[] rotation, float[] movement,
            int[] moduleContext, float[] passiveNoSlowSettings) {
        assertOwner();
        assertOpen();
        if (clocks == null || clocks.length != 2 ||
            flags == null || flags.length != 8 ||
            coordinates == null || coordinates.length != 6 ||
            rotation == null || rotation.length != 2 ||
            movement == null || movement.length != 3 ||
            moduleContext == null || moduleContext.length != 5 ||
            passiveNoSlowSettings == null || passiveNoSlowSettings.length != 3)
            throw new IllegalArgumentException("Invalid typed module frame dimensions");
        return new ModuleDecision(nativeObserveModules(
            handle, clocks, flags, coordinates, rotation, movement,
            moduleContext, passiveNoSlowSettings));
    }

    public static final class ModuleDecision {
        public final Decision sprint;
        public final boolean hasJumpTicks, noSlowEvaluated, replaceInput;
        public final int proposedJumpTicks;
        public final float proposedStrafe, proposedForward;

        private ModuleDecision(float[] values) {
            if (values == null || values.length != 15)
                throw new IllegalStateException("Incorrect three-module JNI result length");
            for (float v : values)
                if (Float.isNaN(v) || Float.isInfinite(v))
                    throw new IllegalStateException("Nonfinite native module decision");
            for (int i : new int[]{9,11,12})
                if (values[i] != 0 && values[i] != 1)
                    throw new IllegalStateException("Malformed native module flag");
            sprint = new Decision(Arrays.copyOf(values, 9));
            hasJumpTicks = values[9] == 1;
            if (values[10] < Integer.MIN_VALUE || values[10] > Integer.MAX_VALUE ||
                values[10] != (float)(int)values[10])
                throw new IllegalStateException("Malformed native jump counter");
            proposedJumpTicks = (int)values[10];
            noSlowEvaluated = values[11] == 1;
            replaceInput = values[12] == 1;
            if (replaceInput && !noSlowEvaluated)
                throw new IllegalStateException("No Slow replaced input without callback");
            proposedStrafe = values[13];
            proposedForward = values[14];
        }
    }

    private static void requireKnownModule(String name) {
        if (!MODULE_NAME.equals(name) && !NO_JUMP_DELAY.equals(name) &&
            !NO_SLOW.equals(name))
            throw new IllegalArgumentException("Unknown canonical module name");
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
