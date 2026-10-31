package dev.slinkywinky.forge;

import dev.slinkywinky.GameHost;
import dev.slinkywinky.HostSettings;
import dev.slinkywinky.NativeEngine;
import net.minecraft.client.Minecraft;
import net.minecraft.client.settings.KeyBinding;
import net.minecraft.entity.Entity;
import net.minecraft.item.ItemAxe;
import net.minecraft.item.ItemStack;
import net.minecraft.item.ItemSword;
import net.minecraft.util.MovingObjectPosition;
import net.minecraft.util.Vec3;
import net.minecraftforge.client.event.RenderGameOverlayEvent;
import net.minecraftforge.common.MinecraftForge;
import net.minecraftforge.fml.client.registry.ClientRegistry;
import net.minecraftforge.fml.common.FMLCommonHandler;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.common.event.FMLInitializationEvent;
import net.minecraftforge.fml.common.eventhandler.SubscribeEvent;
import net.minecraftforge.fml.common.gameevent.TickEvent;
import org.lwjgl.input.Keyboard;
import org.lwjgl.input.Mouse;

/** Experimental Forge 1.8.9 *read-only* preview, never applies gameplay actions. */
@Mod(modid="slinkywinky",name="SlinkyWinky Preview",version="0.1.0",clientSideOnly=true)
public final class ForgePreview {
    private KeyBinding toggle;
    private GameHost host;
    private boolean enabled;
    private String status="F9: enable read-only preview";
    private long observations;
    private final GameHost.Actions readonlyActions=new GameHost.Actions() {
        @Override public void apply(NativeEngine.Decision decision) {
            ++observations;
            status="PREVIEW ONLY | frames="+observations+
                " | aim="+decision.hasRotation+" | click="+decision.attackPulse+
                " | reach="+decision.hasReach;
            // Deliberately NO rotation, key, packet, reach or mouse changes here.
        }
        @Override public void releaseSynthetic() { status="PREVIEW ONLY | session released"; }
    };

    @Mod.EventHandler public void initialize(FMLInitializationEvent event) {
        toggle=new KeyBinding("SlinkyWinky read-only preview", Keyboard.KEY_F9, "SlinkyWinky");
        ClientRegistry.registerKeyBinding(toggle);
        FMLCommonHandler.instance().bus().register(this);
        MinecraftForge.EVENT_BUS.register(this);
        System.out.println("[SlinkyWinkyPreview] initialized - read-only");
    }

    private void stop() {
        enabled=false;
        if(host!=null) {
            try {host.close();} finally {host=null;}
        }
    }

    @SubscribeEvent public void onClientTick(TickEvent.ClientTickEvent event) {
        if(event.phase!=TickEvent.Phase.END) return;
        if(toggle.isPressed()) {
            if(enabled) {stop();status="F9: enable read-only preview";}
            else {
                try {
                    host=new GameHost(new NativeEngine(new int[]{1,2,3,4}),readonlyActions);
                    enabled=true;observations=0;
                    status="PREVIEW ONLY | collecting frames";
                } catch(LinkageError | RuntimeException error) {
                    enabled=false;host=null;
                    status="Unavailable: native JNI library missing or incompatible";
                }
            }
        }
        if(!enabled || host==null) return;
        final Minecraft mc=Minecraft.getMinecraft();
        GameHost.Frame frame=new GameHost.Frame();
        frame.monotonicMs=System.nanoTime()/1000000L;
        frame.worldToken=mc.theWorld;
        frame.playerToken=mc.thePlayer;
        frame.guiOpen=mc.currentScreen!=null;
        frame.manualLeft=Mouse.isButtonDown(0);
        frame.manualRight=Mouse.isButtonDown(1);
        // All action modules are disabled in game; enable only the decision simulator.
        frame.settings=HostSettings.safeDefaults().withToggle(HostSettings.CLICKER,true)
            .withToggle(HostSettings.AIM,true).toNativeArray();
        if(mc.thePlayer!=null && mc.theWorld!=null) {
            Vec3 eye=mc.thePlayer.getPositionEyes(1.0F);
            frame.eyeX=eye.xCoord;frame.eyeY=eye.yCoord;frame.eyeZ=eye.zCoord;
            frame.yaw=mc.thePlayer.rotationYaw;frame.pitch=mc.thePlayer.rotationPitch;
            frame.hurtTime=mc.thePlayer.hurtTime;
            frame.sprinting=mc.thePlayer.isSprinting();
            ItemStack held=mc.thePlayer.getHeldItem();
            frame.weapon=held!=null && (held.getItem() instanceof ItemSword || held.getItem() instanceof ItemAxe);
            MovingObjectPosition hit=mc.objectMouseOver;
            Entity target=hit==null?null:hit.entityHit;
            if(target!=null && target!=mc.thePlayer) {
                frame.targetPresent=true;
                frame.targetId=target.getEntityId();
                frame.targetAlive=target.isEntityAlive();
                frame.targetVisible=mc.thePlayer.canEntityBeSeen(target);
                frame.targetX=target.posX;frame.targetY=target.posY+target.getEyeHeight();frame.targetZ=target.posZ;
            }
        }
        try {host.tick(frame);}
        catch(RuntimeException | LinkageError error) {
            try {stop();} catch(RuntimeException ignored) { /* native failure: no actions to release */ }
            status="Preview stopped after invalid frame or JNI error";
        }
    }

    @SubscribeEvent public void onOverlay(RenderGameOverlayEvent.Text event) {
        Minecraft mc=Minecraft.getMinecraft();
        if(mc.fontRendererObj!=null) mc.fontRendererObj.drawStringWithShadow(
            "SlinkyWinky: "+status,4,4,0xFFDDDDDD);
    }
}
