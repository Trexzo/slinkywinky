package dev.slinkywinky.forge;

import dev.slinkywinky.RelationshipRosterSampler;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import net.minecraft.client.Minecraft;
import net.minecraft.client.entity.EntityPlayerSP;
import net.minecraft.client.multiplayer.WorldClient;
import net.minecraft.entity.player.EntityPlayer;
import net.minecraft.item.Item;
import net.minecraft.item.ItemStack;

/** R277: read-only Forge 1.8.9 source mapping for R276 roster receipts. */
public final class ForgeRelationshipRosterPort
        implements RelationshipRosterSampler.Port {
    private final Minecraft mc;
    private final Thread callbackThread;
    public ForgeRelationshipRosterPort(Minecraft minecraft) {
        mc=minecraft;
        callbackThread=Thread.currentThread();
    }
    @Override public boolean onClientThread() {
        return Thread.currentThread()==callbackThread && mc!=null &&
            mc.theWorld!=null && mc.thePlayer!=null &&
            mc.theWorld.isRemote && mc.isSingleplayer();
    }
    @Override public Object worldIdentity() {
        return mc==null?null:mc.theWorld;
    }
    @Override public Object ownerIdentity() {
        return mc==null?null:mc.thePlayer;
    }
    @Override public UUID ownerUuid() {
        return mc==null || mc.thePlayer==null?
            null:mc.thePlayer.getUniqueID();
    }
    @Override public List<RelationshipRosterSampler.Observation> players() {
        if(!onClientThread())
            throw new IllegalStateException("Roster read outside local client");
        final WorldClient world=mc.theWorld;
        final EntityPlayerSP owner=mc.thePlayer;
        final List<RelationshipRosterSampler.Observation> result=
            new ArrayList<RelationshipRosterSampler.Observation>();
        if(world.playerEntities.size()>256)
            throw new IllegalStateException("Roster over 256 players");
        for(EntityPlayer player:world.playerEntities) {
            if(player==null)
                throw new IllegalStateException("Null roster player");
            boolean armorSeen=false;
            if(player.inventory!=null &&
               player.inventory.armorInventory!=null) {
                for(ItemStack armor:player.inventory.armorInventory) {
                    if(armor!=null && armor.getItem()!=null &&
                       Item.getIdFromItem(armor.getItem())!=0) {
                        armorSeen=true;break;
                    }
                }
            }
            result.add(new RelationshipRosterSampler.Observation(
                player,player.getUniqueID(),player.getName(),
                player.prevPosX,player.prevPosZ,player.posX,player.posZ,
                player.ticksExisted,player.isPlayerSleeping(),
                player.isInvisibleToPlayer(owner),armorSeen));
        }
        if(world!=mc.theWorld || owner!=mc.thePlayer || !onClientThread())
            throw new IllegalStateException("World/owner switched midread");
        return result;
    }
}
