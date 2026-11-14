package dev.slinkywinky.forge;

import net.minecraft.client.entity.EntityPlayerSP;

/**
 * Called only by the uniquely matched post-vanilla-slowdown 1.8.9
 * bytecode insertion. It is deliberately a simple call: exception
 * containment and owner-thread/game-world admission are in ForgePreview.
 */
public final class R214NoSlowHook {
    private R214NoSlowHook() {}

    private static int hookCalls;
    public static void afterVanillaSlowdown(EntityPlayerSP player) {
        if (++hookCalls == 1)
            System.out.println("[SlinkyWinkyR214] " +
                "R214_REAL_ENTITYPLAYER_ONLIVINGUPDATE_HOOK_REACHED");
        ForgePreview.afterR214VanillaSlowdown(player);
    }
}
