package dev.slinkywinky.forge;

import dev.slinkywinky.EspWorldSnapshotSampler;
import java.util.*;
import net.minecraft.block.Block;
import net.minecraft.client.Minecraft;
import net.minecraft.client.entity.EntityPlayerSP;
import net.minecraft.client.multiplayer.WorldClient;
import net.minecraft.entity.Entity;
import net.minecraft.entity.item.EntityItem;
import net.minecraft.entity.player.EntityPlayer;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;
import net.minecraft.tileentity.TileEntity;
import net.minecraft.util.BlockPos;

/** R300 real Forge 1.8.9 client-world sampler; no render, packets, or writes. */
public final class ForgeEspWorldSnapshotPort implements EspWorldSnapshotSampler.Port {
    private final Minecraft mc;
    private final WorldClient world;
    private final EntityPlayerSP player;
    private final Thread thread=Thread.currentThread();
    public ForgeEspWorldSnapshotPort(Minecraft minecraft){
        if(minecraft==null||!minecraft.isCallingFromMinecraftThread()||
           minecraft.theWorld==null||minecraft.thePlayer==null||
           !minecraft.theWorld.isRemote||!minecraft.isSingleplayer())
            throw new IllegalStateException("R300 requires local client world");
        mc=minecraft;world=mc.theWorld;player=mc.thePlayer;
    }
    @Override public boolean onClientThread(){
        return Thread.currentThread()==thread&&mc.isCallingFromMinecraftThread()&&
               mc.theWorld==world&&mc.thePlayer==player&&
               world.isRemote&&mc.isSingleplayer();
    }
    private void current(){
        if(!onClientThread())throw new IllegalStateException("R300 stale owner");
    }
    @Override public Object worldIdentity(){return mc.theWorld;}
    @Override public Object playerIdentity(){return mc.thePlayer;}
    private static EspWorldSnapshotSampler.Position at(double x,double y,double z){
        return new EspWorldSnapshotSampler.Position(x,y,z);
    }
    @Override public EspWorldSnapshotSampler.Position ownerPosition(){
        current();return at(player.posX,player.posY,player.posZ);
    }
    @Override public List<EspWorldSnapshotSampler.Tile> tiles(){
        current();
        if(world.loadedTileEntityList.size()>EspWorldSnapshotSampler.MAX_TILES)
            throw new IllegalStateException("R300 tile limit");
        final List<EspWorldSnapshotSampler.Tile> out=
            new ArrayList<EspWorldSnapshotSampler.Tile>();
        for(TileEntity te:world.loadedTileEntityList){
            current();
            if(te==null||te.getPos()==null)throw new IllegalStateException("R300 null tile");
            BlockPos pos=te.getPos();
            Block block=world.getBlockState(pos).getBlock();
            if(block==null)throw new IllegalStateException("R300 missing block");
            out.add(new EspWorldSnapshotSampler.Tile(
                at(pos.getX(),pos.getY(),pos.getZ()),Block.getIdFromBlock(block)));
        }
        current();return out;
    }
    @Override public List<EspWorldSnapshotSampler.Drop> drops(){
        current();
        if(world.loadedEntityList.size()>EspWorldSnapshotSampler.MAX_ENTITIES)
            throw new IllegalStateException("R300 entity limit");
        final List<EspWorldSnapshotSampler.Drop> out=
            new ArrayList<EspWorldSnapshotSampler.Drop>();
        for(Entity entity:world.loadedEntityList){
            current();if(!(entity instanceof EntityItem))continue;
            EntityItem drop=(EntityItem)entity;
            ItemStack stack=drop.getEntityItem();
            boolean hasStack=stack!=null;
            boolean hasItem=hasStack&&stack.getItem()!=null;
            int id=hasItem?Item.getIdFromItem(stack.getItem()):-1;
            out.add(new EspWorldSnapshotSampler.Drop(
                at(drop.prevPosX,drop.prevPosY,drop.prevPosZ),
                at(drop.posX,drop.posY,drop.posZ),
                id,hasStack?stack.stackSize:0,drop.isDead,hasStack,hasItem));
        }
        current();return out;
    }
    @Override public List<EspWorldSnapshotSampler.Player> players(){
        current();
        if(world.playerEntities.size()>EspWorldSnapshotSampler.MAX_PLAYERS)
            throw new IllegalStateException("R300 player limit");
        final List<EspWorldSnapshotSampler.Player> out=
            new ArrayList<EspWorldSnapshotSampler.Player>();
        for(EntityPlayer e:world.playerEntities){
            current();if(e==null)throw new IllegalStateException("R300 null player");
            out.add(new EspWorldSnapshotSampler.Player(
                e.getUniqueID(),at(e.prevPosX,e.prevPosY,e.prevPosZ),
                at(e.posX,e.posY,e.posZ),e==player,e.isDead,
                e.getHealth(),e.getMaxHealth(),e.getAbsorptionAmount(),e.hurtTime));
        }
        current();return out;
    }
}
