package dev.slinkywinky;

/**
 * R212: owner-thread-only, reversible Minecraft action boundary for the
 * existing first-party No Slow and No Jump Delay *decision* modules.
 *
 * This class contains no Minecraft imports and cannot inject a client. A
 * Minecraft-specific adapter supplies a scoped Port. Gameplay writes are
 * denied unless an explicitly authorized host passes allowActions=true.
 *
 * Sprint remains decision-only pending original event/key ownership evidence.
 */
public final class OwnedModuleActions {
    public interface Port {
        Object worldIdentity();
        Object playerIdentity();
        Object movementIdentity();
        float readForward();
        float readStrafe();
        void writeForward(float value);
        void writeStrafe(float value);
        int readJumpTicks();
        void writeJumpTicks(int value);
    }

    public static final class Input {
        public final boolean allowActions, playerReady;
        public final long worldGeneration;
        public final boolean noSlowActive, replaceInput, jumpDelayActive, hasJumpTicks;
        public final float proposedForward, proposedStrafe;
        public final int proposedJumpTicks;

        public Input(boolean allowActions, boolean playerReady,
                long worldGeneration, boolean noSlowActive, boolean replaceInput,
                float proposedForward, float proposedStrafe,
                boolean jumpDelayActive, boolean hasJumpTicks, int proposedJumpTicks) {
            this.allowActions = allowActions;
            this.playerReady = playerReady;
            this.worldGeneration = worldGeneration;
            this.noSlowActive = noSlowActive;
            this.replaceInput = replaceInput;
            this.proposedForward = proposedForward;
            this.proposedStrafe = proposedStrafe;
            this.jumpDelayActive = jumpDelayActive;
            this.hasJumpTicks = hasJumpTicks;
            this.proposedJumpTicks = proposedJumpTicks;
        }
    }

    public static final class Result {
        public final boolean admitted, noSlowWritten, jumpTicksWritten;
        Result(boolean admitted, boolean noSlowWritten, boolean jumpTicksWritten) {
            this.admitted = admitted;
            this.noSlowWritten = noSlowWritten;
            this.jumpTicksWritten = jumpTicksWritten;
        }
    }

    private final Thread owner = Thread.currentThread();
    private Object lastWorld, lastPlayer, lastMovement;
    private long generation;
    private boolean movementOwned, closed;
    private float oldForward, oldStrafe, writtenForward, writtenStrafe;

    public Result apply(Input input, Port port) {
        assertOwner();
        if (closed || input == null || port == null)
            throw new IllegalStateException("Closed or absent module-action session");
        if (input.worldGeneration <= 0 || input.proposedJumpTicks != 0 ||
            !Float.isFinite(input.proposedForward) ||
            !Float.isFinite(input.proposedStrafe) ||
            Math.abs(input.proposedForward) > 32f ||
            Math.abs(input.proposedStrafe) > 32f)
            throw new IllegalArgumentException("Invalid bounded module action proposal");
        if (!input.allowActions || !input.playerReady) {
            release(port);
            return new Result(false, false, false);
        }
        Object world = port.worldIdentity();
        Object player = port.playerIdentity();
        Object movement = port.movementIdentity();
        if (world == null || player == null || movement == null)
            throw new IllegalStateException("Missing live module action target");

        if (lastWorld != world || lastPlayer != player ||
            lastMovement != movement || generation != input.worldGeneration) {
            // A replaced player/world is not our old target. Never write
            // through a stale object after logout or world replacement.
            dropOwnership();
            lastWorld = world;
            lastPlayer = player;
            lastMovement = movement;
            generation = input.worldGeneration;
        } else {
            restoreIfStillOwned(port);
        }

        boolean wroteMovement = false;
        boolean wroteJump = false;
        if (input.noSlowActive && input.replaceInput) {
            final float currentForward = port.readForward();
            final float currentStrafe = port.readStrafe();
            if (!Float.isFinite(currentForward) || !Float.isFinite(currentStrafe))
                throw new IllegalArgumentException("Nonfinite Minecraft movement state");
            oldForward = currentForward;
            oldStrafe = currentStrafe;
            writtenForward = input.proposedForward;
            writtenStrafe = input.proposedStrafe;
            // Set ownership before writes so an exception can safely unwind.
            movementOwned = true;
            try {
                port.writeStrafe(writtenStrafe);
                port.writeForward(writtenForward);
                wroteMovement = true;
            } catch (RuntimeException failure) {
                restoreIfStillOwned(port);
                throw failure;
            }
        }
        if (input.jumpDelayActive && input.hasJumpTicks) {
            // Zero cooldown is a one-tick override, not a held synthetic
            // key. Disabling simply stops writing and allows normal
            // Minecraft cooldown behavior to resume.
            int ticks = port.readJumpTicks();
            if (ticks > 0) {
                port.writeJumpTicks(0);
                wroteJump = true;
            }
        }
        return new Result(true, wroteMovement, wroteJump);
    }

    /** Release only on the admitted owner thread; never update a new world. */
    public void release(Port port) {
        assertOwner();
        if (closed) return;
        if (port != null && lastWorld == port.worldIdentity() &&
            lastPlayer == port.playerIdentity() &&
            lastMovement == port.movementIdentity())
            restoreIfStillOwned(port);
        dropOwnership();
        lastWorld = lastPlayer = lastMovement = null;
        generation = 0;
    }

    public void close(Port port) {
        assertOwner();
        if (closed) return;
        release(port);
        closed = true;
    }

    public boolean isClosed() { return closed; }

    private void restoreIfStillOwned(Port port) {
        if (!movementOwned) return;
        // A fresh physical input update can replace either field. Never
        // overwrite a value that no longer equals our last write.
        if (Float.compare(port.readStrafe(), writtenStrafe) == 0)
            port.writeStrafe(oldStrafe);
        if (Float.compare(port.readForward(), writtenForward) == 0)
            port.writeForward(oldForward);
        movementOwned = false;
    }

    private void dropOwnership() {
        movementOwned = false;
        oldForward = oldStrafe = writtenForward = writtenStrafe = 0f;
    }

    private void assertOwner() {
        if (Thread.currentThread() != owner)
            throw new IllegalStateException("Foreign Minecraft module action thread");
    }
}
