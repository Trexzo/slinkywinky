package dev.slinkywinky.forge;

import dev.slinkywinky.SourceWorldReadOnlyHost;
import dev.slinkywinky.SourceRemotePositionHistory;
import java.util.UUID;
import net.minecraft.client.Minecraft;
import net.minecraft.client.entity.EntityPlayerSP;
import net.minecraft.client.gui.inventory.GuiInventory;
import net.minecraft.client.multiplayer.WorldClient;
import net.minecraft.client.settings.KeyBinding;
import net.minecraft.entity.Entity;
import net.minecraft.entity.EntityLivingBase;
import net.minecraft.item.ItemAxe;
import net.minecraft.item.ItemStack;
import net.minecraft.item.ItemSword;
import net.minecraft.util.MovingObjectPosition;
import net.minecraft.util.Vec3;
import org.lwjgl.input.Keyboard;
import org.lwjgl.input.Mouse;

/**
 * R337: Forge 1.8.9, read-only, singleplayer client-tick adapter.
 * Uses real game/player/target/key *observations*, no game writes or packets.
 * Forge ClientTick.END is NOT the original Slinky native event18 timing.
 */
public final class ForgeSourceWorldPort implements SourceWorldReadOnlyHost.HistoryPort {
    private final Minecraft mc;
    private final Thread owner=Thread.currentThread();
    private final boolean debugBoxSettings;
    public ForgeSourceWorldPort(Minecraft minecraft){this(minecraft,false);}
    public ForgeSourceWorldPort(Minecraft minecraft,boolean debugBoxSettings){
        if(minecraft==null)throw new IllegalArgumentException("Minecraft unavailable");
        mc=minecraft;
        this.debugBoxSettings=debugBoxSettings;
    }
    @Override public boolean admittedClientThread(){
        return Thread.currentThread()==owner&&
            mc.isCallingFromMinecraftThread()&&
            (mc.theWorld==null||mc.thePlayer==null||
             (mc.theWorld.isRemote&&mc.isSingleplayer()));
    }
    @Override public Object worldIdentity(){
        return mc.theWorld!=null&&mc.thePlayer!=null&&
            mc.theWorld.isRemote&&mc.isSingleplayer()?mc.theWorld:null;
    }
    @Override public Object playerIdentity(){
        return worldIdentity()!=null?mc.thePlayer:null;
    }
    private static int physical(KeyBinding key){
        // KeyBinding.isKeyDown can include simulated or injected state;
        // the observer records LWJGL key state only.
        int code=key.getKeyCode();
        return code>0&&code<256&&Keyboard.isKeyDown(code)?1:0;
    }
    @Override public SourceWorldReadOnlyHost.RemoteTarget captureRemoteTarget(){
        if(!admittedClientThread()||worldIdentity()==null)
            throw new IllegalStateException("R342 history owner is not local");
        final WorldClient world=mc.theWorld;
        final EntityPlayerSP self=mc.thePlayer;
        final MovingObjectPosition hit=mc.objectMouseOver;
        final Entity entity=hit==null?null:hit.entityHit;
        if(!(entity instanceof EntityLivingBase)||entity==self||
           entity.isDead||entity.worldObj!=world)return null;
        final UUID id=entity.getUniqueID();
        if(id==null)return null;
        final SourceWorldReadOnlyHost.RemoteTarget sample=
            new SourceWorldReadOnlyHost.RemoteTarget(entity.getEntityId(),
                id.getMostSignificantBits(),id.getLeastSignificantBits(),
                entity.serverPosX,entity.serverPosY,entity.serverPosZ);
        if(!admittedClientThread()||mc.theWorld!=world||mc.thePlayer!=self)
            throw new IllegalStateException("R342 history world switched");
        return sample;
    }

    @Override public SourceWorldReadOnlyHost.BoxFrame captureHistoricalBoxes(
            float partial,SourceRemotePositionHistory.Historical historical){
        if(historical==null)return null;
        if(!admittedClientThread()||worldIdentity()==null)
            throw new IllegalStateException("R342 historic camera owner unavailable");
        final WorldClient world=mc.theWorld;
        final EntityPlayerSP self=mc.thePlayer;
        final MovingObjectPosition aim=mc.objectMouseOver;
        final Entity target=aim==null?null:aim.entityHit;
        if(!(target instanceof EntityLivingBase)||target==self||
           target.isDead||target.worldObj!=world)return null;
        final UUID uuid=target.getUniqueID();
        if(uuid==null||target.getEntityId()!=historical.current.entityId||
           uuid.getMostSignificantBits()!=historical.current.uniqueHigh||
           uuid.getLeastSignificantBits()!=historical.current.uniqueLow)
            throw new IllegalStateException("R342 entity ID or UUID recycled");
        final net.minecraft.client.renderer.entity.RenderManager camera=
            mc.getRenderManager();
        if(camera==null)throw new IllegalStateException("Missing source history camera");
        // Historical *vanilla-applied serverPos* fixed-point values,
        // NOT a held/delayed original network packet or native Slinky ghost.
        final double[] values={
            historical.previous.x(),historical.previous.y(),historical.previous.z(),
            historical.current.x(),historical.current.y(),historical.current.z(),
            camera.viewerPosX,camera.viewerPosY,camera.viewerPosZ,
            partial,target.prevRotationYaw,target.rotationYaw
        };
        if(!admittedClientThread()||mc.theWorld!=world||mc.thePlayer!=self)
            throw new IllegalStateException("R342 historical world changed midread");
        return new SourceWorldReadOnlyHost.BoxFrame(values);
    }

    @Override public SourceWorldReadOnlyHost.BoxFrame captureBoxes(float partial){
        if(!admittedClientThread()||worldIdentity()==null)
            throw new IllegalStateException("R341 source box needs owned local world");
        final WorldClient world=mc.theWorld;
        final EntityPlayerSP self=mc.thePlayer;
        final MovingObjectPosition look=mc.objectMouseOver;
        final Entity entity=look==null?null:look.entityHit;
        if(!(entity instanceof EntityLivingBase)||entity==self||
           entity.isDead||entity.worldObj!=world)return null;
        final EntityLivingBase target=(EntityLivingBase)entity;
        final net.minecraft.client.renderer.entity.RenderManager camera=
            mc.getRenderManager();
        if(camera==null)throw new IllegalStateException("Missing e6 camera");
        // This uses current target client positions, NOT an original
        // backtracked server position. It is diagnostics only.
        final double[] values={
            target.lastTickPosX,target.lastTickPosY,target.lastTickPosZ,
            target.posX,target.posY,target.posZ,
            camera.viewerPosX,camera.viewerPosY,camera.viewerPosZ,
            partial,target.prevRotationYaw,target.rotationYaw
        };
        if(!admittedClientThread()||mc.theWorld!=world||mc.thePlayer!=self)
            throw new IllegalStateException("World changed during e6 capture");
        return new SourceWorldReadOnlyHost.BoxFrame(values);
    }

    @Override public SourceWorldReadOnlyHost.RenderFrame captureRender(
            double dt,float partial){
        if(!admittedClientThread()||worldIdentity()==null)
            throw new IllegalStateException("R338 no admitted render owner");
        final WorldClient world=mc.theWorld;
        final EntityPlayerSP self=mc.thePlayer;
        final MovingObjectPosition look=mc.objectMouseOver;
        final Entity hovered=look==null?null:look.entityHit;
        if(!(hovered instanceof EntityLivingBase)||hovered==self||
           hovered.isDead||hovered.worldObj!=world)return null;
        final EntityLivingBase target=(EntityLivingBase)hovered;
        final Vec3 eye=self.getPositionEyes(partial);
        final ItemStack held=self.getHeldItem();
        final boolean sword=held!=null&&held.getItem() instanceof ItemSword;
        final double x=target.lastTickPosX+(target.posX-target.lastTickPosX)*partial;
        final double y=target.lastTickPosY+(target.posY-target.lastTickPosY)*partial;
        final double z=target.lastTickPosZ+(target.posZ-target.lastTickPosZ)*partial;
        // Original target interpolation countdown is not exposed here.
        // The 0-step and 0.1 movement factor remain diagnostic assumptions.
        final double[] values={
            dt,partial,eye.xCoord,eye.yCoord,eye.zCoord,
            self.motionX,self.motionY,self.motionZ,
            x,y,z,target.serverPosX/32.0d,target.serverPosY/32.0d,
            target.serverPosZ/32.0d,
            self.rotationYaw,self.rotationPitch,
            self.movementInput==null?0:self.movementInput.moveForward,
            self.movementInput==null?0:self.movementInput.moveStrafe,
            0.1d,0d
        };
        if(!admittedClientThread()||mc.theWorld!=world||mc.thePlayer!=self)
            throw new IllegalStateException("R338 render owner switched midread");
        return new SourceWorldReadOnlyHost.RenderFrame(values,sword);
    }

    @Override public SourceWorldReadOnlyHost.Frame capture(){
        if(!admittedClientThread()||worldIdentity()==null)
            throw new IllegalStateException("R337 local client owner unavailable");
        final WorldClient world=mc.theWorld;
        final EntityPlayerSP self=mc.thePlayer;
        final MovingObjectPosition hit=mc.objectMouseOver;
        final Entity entity=hit==null?null:hit.entityHit;
        final EntityLivingBase target=entity instanceof EntityLivingBase&&
            entity!=self&&!entity.isDead?(EntityLivingBase)entity:null;
        final ItemStack held=self.getHeldItem();
        final boolean weapon=held!=null&&held.getItem()!=null&&
            (held.getItem() instanceof ItemSword||held.getItem() instanceof ItemAxe);
        final boolean sword=held!=null&&held.getItem() instanceof ItemSword;
        final boolean mouse=mc.currentScreen==null&&Mouse.isCreated()&&
                            Mouse.isButtonDown(0);
        final boolean canSee=target!=null&&self.canEntityBeSeen(target);
        final int targetId=target==null?0:target.getEntityId();
        final double distanceSq=target==null?0:self.getDistanceSqToEntity(target);
        final double serverX=target==null?0:target.serverPosX/32.0d;
        final double serverY=target==null?0:target.serverPosY/32.0d;
        final double serverZ=target==null?0:target.serverPosZ/32.0d;
        final boolean inGui=mc.currentScreen!=null;
        final int inventory=inGui?(mc.currentScreen instanceof GuiInventory?1:2):0;
        final float forward=self.movementInput==null?0:
                            self.movementInput.moveForward;
        final boolean jump=self.movementInput!=null&&self.movementInput.jump;
        final boolean cursorBusy=self.inventory!=null&&
                                 self.inventory.getItemStack()!=null;
        // No known original reach callback or last-attack timestamp at
        // this Forge phase: do NOT fabricate reach eligibility or attacks.
        final int[][] bits={
            // Backtrack tick: reach admission deliberately 0.
            {1,weapon?1:0,target!=null?1:0,self.ticksExisted,0,
             targetId,self.getEntityId(),target==null?0:target.hurtTime,
             0,0,debugBoxSettings?1:0,1},
            // Lag Range tick: observed target and sprint, no inferred hit.
            {1,weapon?1:0,target!=null?1:0,self.isSprinting()?1:0,
             0,0,mc.gameSettings.thirdPersonView==0?0:1,
             target==null?0:target.hurtTime,0,0,0,
             debugBoxSettings?1:0},
            // Timer Range tick: requirements tied to actual observed input.
            {1,target!=null?1:0,self.onGround?1:0,mouse?1:0,
             weapon?1:0,self.isUsingItem()?1:0,jump?1:0,
             canSee?1:0,target!=null?1:0,sword?1:0,
             target==null?0:target.hurtTime,1,1,1,0},
            // Inv Walk tick: observed physical keys, vanilla mode only;
            // silent server GUI state is NOT inferred at a Forge tick.
            {1,inventory,
             physical(mc.gameSettings.keyBindBack),
             physical(mc.gameSettings.keyBindLeft),
             physical(mc.gameSettings.keyBindRight),
             physical(mc.gameSettings.keyBindForward),
             physical(mc.gameSettings.keyBindJump),
             cursorBusy?1:0,0,0,1,1}
        };
        final double[][] values={
            {distanceSq,0,120,3,0,6,10},
            {distanceSq,10,serverX,serverY,serverZ,150},
            {distanceSq,self.motionY,forward,5,3,2,0.5},
            {4}
        };
        if(mc.theWorld!=world||mc.thePlayer!=self||
           !admittedClientThread())
            throw new IllegalStateException("R337 Forge owner changed midread");
        return new SourceWorldReadOnlyHost.Frame(bits,values);
    }
}
