package dev.slinkywinky.forge;

import net.minecraft.client.entity.EntityPlayerSP;

/**
 * Called only by the uniquely matched post-vanilla-slowdown 1.8.9
 * bytecode insertion. It is deliberately a simple call: exception
 * containment and owner-thread/game-world admission are in ForgePreview.
 */
public final class R214NoSlowHook {
    private R214NoSlowHook() {}

    public static void afterVanillaSlowdown(EntityPlayerSP player) {
        ForgePreview.afterR214VanillaSlowdown(player);
    }
}
