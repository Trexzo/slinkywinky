package dev.slinkywinky.forge;

import dev.slinkywinky.ClutchWorldSampler;
import dev.slinkywinky.ClutchFaceSampler;
import net.minecraft.block.Block;
import net.minecraft.client.Minecraft;
import net.minecraft.client.entity.EntityPlayerSP;
import net.minecraft.client.multiplayer.WorldClient;
import net.minecraft.util.BlockPos;
import net.minecraft.util.MovingObjectPosition;
import net.minecraft.util.Vec3;

/**
 * R237: read-only Minecraft 1.8.9 block and source raycast adapter.
 * Uses Minecraft's real client world; no world writes, key events, packet
 * sends, rotation changes, or native hooks. Never retain past world swaps.
 */
public final class ForgeClutchWorldPort implements ClutchWorldSampler.Port, ClutchFaceSampler.Port {
    private final Minecraft mc;
    private final WorldClient world;
    private final EntityPlayerSP player;
    private final Thread owner=Thread.currentThread();

    public ForgeClutchWorldPort(Minecraft mc) {
        if(mc==null || !mc.isCallingFromMinecraftThread() ||
            mc.theWorld==null || mc.thePlayer==null ||
            !mc.theWorld.isRemote)
            throw new IllegalStateException("No client-thread world/player");
        this.mc=mc;world=mc.theWorld;player=mc.thePlayer;
    }
    @Override public boolean onClientThread() {
        return Thread.currentThread()==owner && mc.isCallingFromMinecraftThread();
    }
    @Override public Object worldIdentity() {return mc.theWorld;}
    @Override public Object playerIdentity() {return mc.thePlayer;}
    private void requireCurrent() {
        if(!onClientThread() || mc.theWorld!=world || mc.thePlayer!=player ||
            !world.isRemote)
            throw new IllegalStateException("Stale Clutch world/player");
    }
    @Override public int blockId(int x,int y,int z) {
        requireCurrent();
        Block block=world.getBlockState(new BlockPos(x,y,z)).getBlock();
        if(block==null)throw new IllegalStateException("Missing block state");
        int id=Block.getIdFromBlock(block);
        requireCurrent();
        return id;
    }
    @Override public ClutchWorldSampler.Pos rayTrace(double[] from,double[] to) {
        requireCurrent();
        MovingObjectPosition hit=world.rayTraceBlocks(
            new Vec3(from[0],from[1],from[2]),
            new Vec3(to[0],to[1],to[2]),false,true,false);
        requireCurrent();
        if(hit==null)return null;
        if(hit.typeOfHit!=MovingObjectPosition.MovingObjectType.BLOCK ||
            hit.getBlockPos()==null)
            throw new IllegalStateException("Non-block Clutch ray evidence");
        BlockPos block=hit.getBlockPos();
        return new ClutchWorldSampler.Pos(
            block.getX(),block.getY(),block.getZ());
    }
    /**
     * R250: current Minecraft cursor hit, read once and never synthesized.
     * Forge PlayerTick.END may observe a previously rendered cursor result,
     * so this is NOT certified event-0 placement timing.
     */
    @Override public ClutchFaceSampler.Hit cursorBlockHit() {
        requireCurrent();
        final MovingObjectPosition hit=mc.objectMouseOver;
        requireCurrent();
        if(hit==null || hit.typeOfHit!=MovingObjectPosition.MovingObjectType.BLOCK)
            return null;
        if(hit.getBlockPos()==null || hit.sideHit==null)
            throw new IllegalStateException("Malformed Minecraft cursor block");
        final BlockPos pos=hit.getBlockPos();
        final int face=hit.sideHit.ordinal();
        requireCurrent();
        return new ClutchFaceSampler.Hit(
            new ClutchFaceSampler.Pos(pos.getX(),pos.getY(),pos.getZ()),face);
    }

}
