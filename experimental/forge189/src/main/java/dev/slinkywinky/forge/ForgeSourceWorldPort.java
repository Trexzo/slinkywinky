package dev.slinkywinky.forge;

import dev.slinkywinky.SourceWorldReadOnlyHost;
import dev.slinkywinky.SourceRemotePositionHistory;
import dev.slinkywinky.SourceBacktrackReachEvidence;
import dev.slinkywinky.SourceAttackEventFence;
import java.lang.reflect.Field;
import net.minecraft.client.entity.EntityOtherPlayerMP;
import java.util.UUID;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import net.minecraft.client.Minecraft;
import net.minecraft.client.entity.EntityPlayerSP;
import net.minecraft.entity.player.EntityPlayer;
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
public final class ForgeSourceWorldPort implements
        SourceWorldReadOnlyHost.HistoryPort,SourceWorldReadOnlyHost.AttackPort {
    private final Minecraft mc;
    private final Thread owner=Thread.currentThread();
    private final boolean debugBoxSettings;
    private final SourceAttackEventFence attackFence;
    private static final boolean R349_EXACT_REACH_READ_ONLY =
        Boolean.getBoolean("slinkywinky.r349BacktrackExactReachReadOnly");
    private static final boolean R353_LAG_ATTACK_READ_ONLY =
        Boolean.getBoolean("slinkywinky.r353NativeLagAttackReadOnly");
    private static Field sourceField(Class<?> type,String name)
            throws ReflectiveOperationException{
        final Field field=type.getDeclaredField(name);
        field.setAccessible(true);
        return field;
    }
    private static final class R349Fields {
        static final Field[] FIELDS=resolve();
        private static Field[] resolve(){
            try {
                return new Field[]{
                    sourceField(EntityLivingBase.class,"lastAttackerTime"),
                    sourceField(EntityOtherPlayerMP.class,"otherPlayerMPX"),
                    sourceField(EntityOtherPlayerMP.class,"otherPlayerMPY"),
                    sourceField(EntityOtherPlayerMP.class,"otherPlayerMPZ")
                };
            }catch(ReflectiveOperationException|SecurityException unavailable){
                // Under unrecognized reobfuscation mappings, never guess.
                return null;
            }
        }
    }

    private static final class R353AttackField {
        static final Field FIELD=resolve();
        private static Field resolve(){
            try{
                Field f=sourceField(EntityLivingBase.class,"lastAttackerTime");
                return f.getType()==Integer.TYPE?f:null;
            }catch(ReflectiveOperationException|SecurityException unavailable){
                return null;
            }
        }
    }

    public ForgeSourceWorldPort(Minecraft minecraft){this(minecraft,false,null);}
    public ForgeSourceWorldPort(Minecraft minecraft,boolean debugBoxSettings){
        this(minecraft,debugBoxSettings,null);
    }
    public ForgeSourceWorldPort(Minecraft minecraft,boolean debugBoxSettings,
                                SourceAttackEventFence attackFence){
        if(minecraft==null)throw new IllegalArgumentException("Minecraft unavailable");
        mc=minecraft;
        this.debugBoxSettings=debugBoxSettings;
        this.attackFence=attackFence;
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

    /**
     * R343: bounded 32-entity, 24-block radial history sampling.
     * The selected crosshair target is first, then up to 31 nearest
     * other real player entities. O(players * 31) and O(31) scratch,
     * independent of world entity count and with no packet retention.
     */
    @Override public List<SourceWorldReadOnlyHost.RemoteTarget> captureRemoteTargets(){
        if(!admittedClientThread()||worldIdentity()==null)
            throw new IllegalStateException("R343 remote player scan requires local owner");
        final WorldClient world=mc.theWorld;
        final EntityPlayerSP self=mc.thePlayer;
        final SourceWorldReadOnlyHost.RemoteTarget targeted=captureRemoteTarget();
        final int wanted=SourceRemotePositionHistory.MAX_ENTITIES-
                         (targeted==null?0:1);
        final List<EntityPlayer> nearest=new ArrayList<EntityPlayer>(wanted);
        for(Object raw:world.playerEntities){
            if(!(raw instanceof EntityPlayer))continue;
            final EntityPlayer candidate=(EntityPlayer)raw;
            if(candidate==self||candidate.isDead||candidate.worldObj!=world||
               targeted!=null&&candidate.getEntityId()==targeted.entityId)
                continue;
            final double distance=self.getDistanceSqToEntity(candidate);
            if(!Double.isFinite(distance)||distance<0||distance>576.0d)continue;
            int insertion=0;
            for(;insertion<nearest.size();++insertion){
                final EntityPlayer existing=nearest.get(insertion);
                final double existingDistance=self.getDistanceSqToEntity(existing);
                if(distance<existingDistance||
                   distance==existingDistance&&
                   candidate.getEntityId()<existing.getEntityId())
                    break;
            }
            if(insertion<wanted){
                nearest.add(insertion,candidate);
                if(nearest.size()>wanted)nearest.remove(wanted);
            }
        }
        final List<SourceWorldReadOnlyHost.RemoteTarget> out=
            new ArrayList<SourceWorldReadOnlyHost.RemoteTarget>(
                SourceRemotePositionHistory.MAX_ENTITIES);
        if(targeted!=null)out.add(targeted);
        for(EntityPlayer e:nearest){
            final UUID uuid=e.getUniqueID();
            if(uuid==null)continue;
            out.add(new SourceWorldReadOnlyHost.RemoteTarget(e.getEntityId(),
                uuid.getMostSignificantBits(),uuid.getLeastSignificantBits(),
                e.serverPosX,e.serverPosY,e.serverPosZ));
        }
        if(!admittedClientThread()||mc.theWorld!=world||mc.thePlayer!=self)
            throw new IllegalStateException("Remote player batch changed world");
        return Collections.unmodifiableList(out);
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

    /**
     * R353 read-only approximation at Forge END tick, *not* an actual
     * original Slinky attack phase-1 callback. No keyboard/network writes.
     */
    @Override public boolean sourceLagRangeAttackNow(){
        if(attackFence==null&&!R353_LAG_ATTACK_READ_ONLY)return false;
        if(!admittedClientThread()||worldIdentity()==null)
            throw new IllegalStateException("R353 owner not admitted");
        final WorldClient world=mc.theWorld;
        final EntityPlayerSP player=mc.thePlayer;
        if(attackFence!=null&&!attackFence.consume(world,player,
                player.ticksExisted,System.nanoTime()))
            return false;
        final Field field=R353AttackField.FIELD;
        if(field==null)throw new IllegalStateException(
            "R353 missing original-named lastAttackerTime");
        try{
            int last=field.getInt(player);
            final boolean current=last>0&&player.ticksExisted==last;
            if(mc.theWorld!=world||mc.thePlayer!=player||!admittedClientThread())
                throw new IllegalStateException("R353 owner changed during attack");
            return current;
        }catch(IllegalAccessException noSource){
            throw new IllegalStateException("R353 source attack field inaccessible",noSource);
        }
    }

    @Override public SourceWorldReadOnlyHost.Frame capture(){
        if(!admittedClientThread()||worldIdentity()==null)
            throw new IllegalStateException("R337 local client owner unavailable");
        final WorldClient world=mc.theWorld;
        final EntityPlayerSP self=mc.thePlayer;
        final MovingObjectPosition hit=mc.objectMouseOver;
        final Entity entity=hit==null?null:hit.entityHit;
        final EntityLivingBase target=entity instanceof EntityLivingBase&&
            entity!=self&&!entity.isDead&&entity.worldObj==world?(EntityLivingBase)entity:null;
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
        // Candidate-only original field snapshot. No inferred network state.
        int sourceAttackTime=0;
        boolean sourceReachEligible=false;
        if(R349_EXACT_REACH_READ_ONLY && target instanceof EntityOtherPlayerMP){
            final Field[] fields=R349Fields.FIELDS;
            if(fields==null)
                throw new IllegalStateException(
                    "R349 original-named 1.8.9 reach/attack fields unavailable");
            try {
                sourceAttackTime=fields[0].getInt(self);
                sourceReachEligible=
                    SourceBacktrackReachEvidence.observedRecentAttack(
                        self.ticksExisted,sourceAttackTime)&&
                    SourceBacktrackReachEvidence.currentIsCloserThanShown(
                        self.posX,SourceBacktrackReachEvidence.candidateEyeY(
                            1,self.posY),self.posZ,
                        target.posX,target.posY,target.posZ,
                        fields[1].getDouble(target),fields[2].getDouble(target),
                        fields[3].getDouble(target));
            }catch(IllegalAccessException missing){
                throw new IllegalStateException("R349 exact reach fields inaccessible",missing);
            }
        }
        final int[][] bits={
            // Backtrack: reach evidence opt-in, no packet ownership.
            {1,weapon?1:0,target!=null?1:0,self.ticksExisted,sourceAttackTime,
             targetId,self.getEntityId(),target==null?0:target.hurtTime,
             sourceReachEligible?1:0,0,debugBoxSettings?1:0,1},
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
