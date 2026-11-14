package dev.slinkywinky.forge;

import dev.slinkywinky.GameHost;
import dev.slinkywinky.HostSettings;
import dev.slinkywinky.NativeEngine;
import dev.slinkywinky.SprintObservationProbe;
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
    private SprintObservationProbe sprintProbe;
    private Object sprintWorld, sprintPlayer;
    private long sprintGeneration=1;
    private boolean enabled;
    private String status="F9: enable read-only preview";
    private long observations;
    private long renderProbeFrames;
    private static final boolean RENDER_PROBE = Boolean.getBoolean("slinkywinky.renderProbe");
    // R204 opt-in only. This reports Sprint proposals, never applies them.
    private static final boolean SPRINT_OBSERVATION =
        Boolean.getBoolean("slinkywinky.sprintObservation");
    private static final boolean SPRINT_GUI =
        Boolean.getBoolean("slinkywinky.sprint.allowGui");
    private static final boolean SPRINT_ITEM =
        Boolean.getBoolean("slinkywinky.sprint.allowUsingItem");
    private static final boolean SPRINT_BACK =
        Boolean.getBoolean("slinkywinky.sprint.allowBackwards");
    private static final boolean SPRINT_SIDE =
        Boolean.getBoolean("slinkywinky.sprint.allowSideways");
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
        // Diagnostic only: enable with -Dslinkywinky.startupProbe=true in an isolated
        // Forge 1.8.9 development JVM. It never issues game actions or packets.
        if (Boolean.getBoolean("slinkywinky.startupProbe")) {
            try (NativeEngine probe = new NativeEngine(new int[]{1,2,3,4})) {
                if (NativeEngine.abiVersion() != 1)
                    throw new IllegalStateException("Unexpected JNI ABI version");
                int[] flags = {1,0,0,0,0,0,0,0,0,0,0,-1};
                NativeEngine.Decision decision = probe.stepDetailed(
                    new long[]{1000,1}, flags, new double[6], new float[2],
                    HostSettings.safeDefaults().toNativeArray());
                if (decision.attackPulse || decision.hasRotation || decision.hasReach ||
                    decision.leftHeld || decision.rightHeld)
                    throw new IllegalStateException("Inactive preview produced an action");
                System.out.println("[SlinkyWinkyPreview] JNI_STARTUP_PROBE_PASS");
            } catch (RuntimeException | LinkageError problem) {
                System.err.println("[SlinkyWinkyPreview] JNI_STARTUP_PROBE_FAIL: " + problem);
                throw new IllegalStateException("Opt-in native probe failed", problem);
            }
        }
    }

    private void stop() {
        enabled=false;
        try {
            if(sprintProbe!=null) sprintProbe.close();
        } finally {
            sprintProbe=null;
            sprintWorld=sprintPlayer=null;
            sprintGeneration=1;
            if(host!=null) {
                try {host.close();} finally {host=null;}
            }
        }
    }

    @SubscribeEvent public void onClientTick(TickEvent.ClientTickEvent event) {
        if(event.phase!=TickEvent.Phase.END) return;
        if(toggle.isPressed()) {
            if(enabled) {stop();status="F9: enable read-only preview";}
            else {
                try {
                    host=new GameHost(new NativeEngine(new int[]{1,2,3,4}),readonlyActions);
                    if(SPRINT_OBSERVATION)
                        sprintProbe=new SprintObservationProbe();
                    enabled=true;observations=0;
                    sprintGeneration=1;
                    sprintWorld=sprintPlayer=null;
                    status="PREVIEW ONLY | collecting frames";
                } catch(LinkageError | RuntimeException error) {
                    try { stop(); } catch(RuntimeException ignored) { /* no game actions */ }
                    status="Unavailable: native JNI library missing or incompatible";
                }
            }
        }
    }

    // Render ticks can run above the fixed 20 TPS simulation cadence.
    // This remains a read-only preview: no game actions are applied.
    @SubscribeEvent public void onRenderFrame(TickEvent.RenderTickEvent event) {
        if(event.phase!=TickEvent.Phase.END) return;
        // Diagnostic is independent of F9 and native loading. It proves Forge render events arrived.
        if (RENDER_PROBE && ++renderProbeFrames == 30)
            System.out.println("[SlinkyWinkyPreview] RENDER_EVENT_PROBE_PASS frames=30");
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
        try {
            host.tick(frame);
            if(sprintProbe!=null) sampleReadOnlySprint(mc,frame);
        }
        catch(RuntimeException | LinkageError error) {
            try {stop();} catch(RuntimeException ignored) { /* native failure: no actions to release */ }
            status="Preview stopped after invalid frame or JNI error";
        }
    }

    /**
     * R204 real Forge render-event observation only. Movement is sampled from
     * player movement-input fields, with getWalkSpeed() as a diagnostic
     * *baseline proxy*, NOT the original native event-21 MoveState.speed.
     * This preview cannot establish precise original speed-event timing.
     */
    private void sampleReadOnlySprint(Minecraft mc, GameHost.Frame frame) {
        if(sprintWorld!=frame.worldToken || sprintPlayer!=frame.playerToken) {
            sprintWorld=frame.worldToken;
            sprintPlayer=frame.playerToken;
            ++sprintGeneration;
        }
        final boolean playerReady=mc.theWorld!=null && mc.thePlayer!=null;
        final int observedFlags=(mc.theWorld!=null?1:0) |
            (mc.thePlayer!=null?2:0) | (frame.targetPresent?4:0);
        final int[] flags={observedFlags,frame.guiOpen?1:0,frame.sprinting?1:0,
            playerReady && mc.thePlayer.isUsingItem()?1:0,
            SPRINT_GUI?1:0,SPRINT_ITEM?1:0,SPRINT_BACK?1:0,SPRINT_SIDE?1:0};
        final double[] coords={frame.eyeX,frame.eyeY,frame.eyeZ,
                               frame.targetX,frame.targetY,frame.targetZ};
        final float[] movement=playerReady
            ? new float[]{mc.thePlayer.moveForward, mc.thePlayer.moveStrafing,
                          mc.thePlayer.capabilities.getWalkSpeed()}
            : new float[3];
        final SprintObservationProbe.Decision decision=sprintProbe.observe(
            new long[]{frame.monotonicMs,sprintGeneration},
            flags,coords,new float[]{frame.yaw,frame.pitch},movement);
        if(!decision.admitted)
            throw new IllegalStateException("Native Sprint observation frame rejected");
        status += " | sprintPreview key="+decision.proposedKeyPressed+
            " speedProposal="+(decision.hasSpeed ? decision.proposedSpeed : "none");
        // Intentionally NO KeyBinding.setKeyBindState(), setSprinting(),
        // player motion writes, game packet sends, or synthetic input.
    }

    @SubscribeEvent public void onOverlay(RenderGameOverlayEvent.Text event) {
        Minecraft mc=Minecraft.getMinecraft();
        if(mc.fontRendererObj!=null) mc.fontRendererObj.drawStringWithShadow(
            "SlinkyWinky: "+status,4,4,0xFFDDDDDD);
    }
}
