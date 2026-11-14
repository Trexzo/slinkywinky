package dev.slinkywinky.forge;

import dev.slinkywinky.OwnedModuleActions;
import java.lang.reflect.Field;
import net.minecraft.client.Minecraft;
import net.minecraft.client.entity.EntityPlayerSP;
import net.minecraft.entity.EntityLivingBase;
import net.minecraft.util.MovementInput;

/**
 * R212: real Minecraft 1.8.9 Forge field adapter for the first-party
 * owned action boundary. This type does not select or enable modules.
 * Only the admitted client owner thread may construct/use it.
 *
 * The private jumpTicks mapping is resolved explicitly and refuses to
 * operate if it is absent. No unsafe pointer scans or native injection.
 */
public final class ForgeModuleActionPort implements OwnedModuleActions.Port {
    private static final Field JUMP_TICKS = resolveJumpTicks();

    private final Minecraft mc;
    private final Object world, player, input;

    public ForgeModuleActionPort(Minecraft minecraft) {
        if (minecraft == null || minecraft.theWorld == null ||
            minecraft.thePlayer == null || minecraft.thePlayer.movementInput == null ||
            !minecraft.theWorld.isRemote)
            throw new IllegalStateException("No admitted Minecraft client player");
        mc = minecraft;
        world = mc.theWorld;
        player = mc.thePlayer;
        input = mc.thePlayer.movementInput;
    }

    /** For cleanup after a world change, never target old player objects. */
    public static OwnedModuleActions.Port currentOrNull(Minecraft minecraft) {
        if (minecraft == null || minecraft.theWorld == null ||
            minecraft.thePlayer == null || minecraft.thePlayer.movementInput == null ||
            !minecraft.theWorld.isRemote)
            return null;
        return new ForgeModuleActionPort(minecraft);
    }

    private static Field resolveJumpTicks() {
        try {
            final Field field = EntityLivingBase.class.getDeclaredField("jumpTicks");
            if (field.getType() != int.class)
                throw new IllegalStateException("Wrong jumpTicks field type");
            field.setAccessible(true);
            return field;
        } catch (NoSuchFieldException | SecurityException error) {
            throw new IllegalStateException(
                "Unsupported Minecraft 1.8.9 private jumpTicks mapping", error);
        }
    }

    private void assertCurrent() {
        if (mc.theWorld != world || mc.thePlayer != player ||
            mc.thePlayer == null || mc.thePlayer.movementInput != input)
            throw new IllegalStateException("Stale Minecraft action target");
    }

    @Override public Object worldIdentity() { return mc.theWorld; }
    @Override public Object playerIdentity() { return mc.thePlayer; }
    @Override public Object movementIdentity() {
        return mc.thePlayer == null ? null : mc.thePlayer.movementInput;
    }
    private MovementInput movement() {
        assertCurrent();
        return ((EntityPlayerSP)player).movementInput;
    }
    @Override public float readForward() { return movement().moveForward; }
    @Override public float readStrafe() { return movement().moveStrafe; }
    @Override public void writeForward(float value) {
        movement().moveForward = value;
    }
    @Override public void writeStrafe(float value) {
        movement().moveStrafe = value;
    }
    @Override public int readJumpTicks() {
        assertCurrent();
        try {
            return JUMP_TICKS.getInt(player);
        } catch (IllegalAccessException error) {
            throw new IllegalStateException("Cannot read Minecraft jumpTicks", error);
        }
    }
    @Override public void writeJumpTicks(int ticks) {
        assertCurrent();
        if (ticks != 0)
            throw new IllegalArgumentException("Only zero cooldown is authorized");
        try {
            JUMP_TICKS.setInt(player, ticks);
        } catch (IllegalAccessException error) {
            throw new IllegalStateException("Cannot write Minecraft jumpTicks", error);
        }
    }
}
