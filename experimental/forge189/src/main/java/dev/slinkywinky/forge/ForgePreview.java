package dev.slinkywinky.forge;

import dev.slinkywinky.GameHost;
import dev.slinkywinky.HostSettings;
import dev.slinkywinky.NativeEngine;
import dev.slinkywinky.SprintObservationProbe;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiMainMenu;
import net.minecraft.world.WorldSettings;
import net.minecraft.world.WorldType;
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
    private boolean r208WorldRequested, r208WorldReady, r208ObservationReceipt;
    private int r208MenuTicks;
    private long r208WorldClientTicks, r208WorldPlayerTicks, r208FirstWorldMs;
    private long r209LastSampleMs, r209MinGapMs=Long.MAX_VALUE, r209MaxGapMs;
    private boolean r209TickReceipt;
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
    // R209 moves only Sprint observation to Forge PlayerTickEvent.END.
    // Forge 1.8.9 has no InputUpdateEvent; movementInput is sampled at
    // this tick boundary, NOT misrepresented as original native event21.
    private static final boolean R209_PLAYER_TICK_TEST =
        Boolean.getBoolean("slinkywinky.r209PlayerTickTest");
    // Test-only: create a local world only in a named TEMP game directory.
    private static final boolean R208_LOCAL_WORLD_TEST =
        Boolean.getBoolean("slinkywinky.r208LocalWorldTest") ||
        R209_PLAYER_TICK_TEST;
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
        if (R208_LOCAL_WORLD_TEST) {
            final Minecraft mc = Minecraft.getMinecraft();
            final String home = mc.mcDataDir.getAbsolutePath().replace('\\', '/');
            final String temp = System.getProperty("java.io.tmpdir").replace('\\', '/');
            final String instancePrefix = R209_PLAYER_TICK_TEST ?
                "/slinkywinky-r209-independent-forge-" :
                "/slinkywinky-r208-independent-forge-";
            final String expectedUser =
                R209_PLAYER_TICK_TEST ? "R209Offline" : "R208Offline";
            if (!SPRINT_OBSERVATION || !home.startsWith(temp) ||
                !home.contains(instancePrefix) || mc.getSession() == null ||
                !expectedUser.equals(mc.getSession().getUsername())) {
                throw new IllegalStateException(
                    "R208 local world test requires TEMP gameDir, dummy offline " +
                    "identity and the read-only Sprint observer");
            }
            System.out.println("[SlinkyWinkyPreview] " +
                (R209_PLAYER_TICK_TEST ? "R209_ISOLATION_GATE_PASS" :
                                        "R208_ISOLATION_GATE_PASS"));
        }
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

    private void startPreview() {
        try {
            host=new GameHost(new NativeEngine(new int[]{1,2,3,4}),readonlyActions);
            if(SPRINT_OBSERVATION) sprintProbe=new SprintObservationProbe();
            enabled=true;
            observations=0;
            sprintGeneration=1;
            sprintWorld=sprintPlayer=null;
            status="PREVIEW ONLY | collecting frames";
        } catch(LinkageError | RuntimeException error) {
            try { stop(); } catch(RuntimeException ignored) { /* no game actions */ }
            status="Unavailable: native JNI library missing or incompatible";
            if (R208_LOCAL_WORLD_TEST)
                System.err.println("[SlinkyWinkyPreview] R208_PREVIEW_START_FAIL " + error);
        }
    }

    @SubscribeEvent public void onClientTick(TickEvent.ClientTickEvent event) {
        if(event.phase!=TickEvent.Phase.END) return;
        if(toggle.isPressed()) {
            if(enabled) {stop();status="F9: enable read-only preview";}
            else startPreview();
        }

        final Minecraft mc=Minecraft.getMinecraft();
        if (enabled && sprintProbe!=null && mc.thePlayer==null &&
            (sprintWorld!=null || sprintPlayer!=null)) {
            // PlayerTickEvent does not fire after a disconnect. Admit one
            // absent-player observation and retire the native session.
            GameHost.Frame absent=new GameHost.Frame();
            absent.monotonicMs=System.nanoTime()/1000000L;
            absent.worldToken=mc.theWorld;
            absent.playerToken=null;
            sampleReadOnlySprint(mc,absent);
            System.out.println("[SlinkyWinkyPreview] R209_WORLD_ABSENCE_OBSERVED");
        }
        if (!R208_LOCAL_WORLD_TEST) return;
        if (!r208WorldRequested &&
            mc.theWorld==null && mc.currentScreen instanceof GuiMainMenu &&
            ++r208MenuTicks >= 30) {
            r208WorldRequested=true;
            System.out.println("[SlinkyWinkyPreview] " +
                (R209_PLAYER_TICK_TEST ? "R209_LOCAL_WORLD_REQUESTED" :
                                        "R208_LOCAL_WORLD_REQUESTED"));
            // Opt-in writes a new save ONLY inside the isolated TEMP gameDir.
            final String worldName =
                R209_PLAYER_TICK_TEST ? "R209-local-only" : "R208-local-only";
            mc.launchIntegratedServer(worldName, "Isolated Sprint diagnostic",
                new WorldSettings(209L, WorldSettings.GameType.CREATIVE,
                    false, false, WorldType.FLAT));
        }
        if (mc.theWorld!=null && mc.thePlayer!=null && mc.isSingleplayer()) {
            ++r208WorldClientTicks;
            if (!r208WorldReady) {
                r208WorldReady=true;
                System.out.println("[SlinkyWinkyPreview] " +
                    (R209_PLAYER_TICK_TEST ? "R209_LOCAL_WORLD_PLAYER_READY_PASS" :
                                            "R208_LOCAL_WORLD_PLAYER_READY_PASS"));
            }
            if (!enabled) startPreview(); // decision-only observer
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
            host.tick(frame); // Existing render-only decision host stays unchanged.
            // Sprint now samples the actual Forge PlayerTickEvent.END only.
        }
        catch(RuntimeException | LinkageError error) {
            try {stop();} catch(RuntimeException ignored) { /* native failure: no actions to release */ }
            status="Preview stopped after invalid frame or JNI error";
        }
    }

    @SubscribeEvent public void onPlayerTick(TickEvent.PlayerTickEvent event) {
        if (event.phase!=TickEvent.Phase.END || !enabled || sprintProbe==null)
            return;
        final Minecraft mc=Minecraft.getMinecraft();
        // The integrated server also emits PlayerTickEvent. Only the actual
        // client-side EntityPlayerSP and a remote WorldClient are admitted.
        if (mc.theWorld==null || mc.thePlayer==null ||
            event.player!=mc.thePlayer || !mc.theWorld.isRemote) return;
        GameHost.Frame frame=new GameHost.Frame();
        frame.monotonicMs=System.nanoTime()/1000000L;
        frame.worldToken=mc.theWorld;
        frame.playerToken=mc.thePlayer;
        frame.guiOpen=mc.currentScreen!=null;
        frame.sprinting=mc.thePlayer.isSprinting();
        Vec3 eye=mc.thePlayer.getPositionEyes(1.0F);
        frame.eyeX=eye.xCoord;
        frame.eyeY=eye.yCoord;
        frame.eyeZ=eye.zCoord;
        frame.yaw=mc.thePlayer.rotationYaw;
        frame.pitch=mc.thePlayer.rotationPitch;
        try {
            sampleReadOnlySprint(mc,frame);
        } catch (RuntimeException | LinkageError problem) {
            try {stop();} catch (RuntimeException ignored) { }
            status="Preview stopped after player-tick JNI failure";
            if(R209_PLAYER_TICK_TEST)
                System.err.println("[SlinkyWinkyPreview] R209_PLAYER_TICK_FAIL "+problem);
        }
    }

    /**
     * Real Forge PlayerTickEvent.END sample; movementInput is observed after
     * Minecraft's per-tick input update. There is no InputUpdateEvent
     * in the pinned Forge 1.8.9 API. getWalkSpeed() remains an explicit
     * diagnostic speed proxy, not the original event-21 MoveState.speed.
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
            ? new float[]{mc.thePlayer.movementInput==null ? 0.0f :
                              mc.thePlayer.movementInput.moveForward,
                          mc.thePlayer.movementInput==null ? 0.0f :
                              mc.thePlayer.movementInput.moveStrafe,
                          mc.thePlayer.capabilities.getWalkSpeed()}
            : new float[3];
        final SprintObservationProbe.Decision decision=sprintProbe.observe(
            new long[]{frame.monotonicMs,sprintGeneration},
            flags,coords,new float[]{frame.yaw,frame.pitch},movement);
        if(!decision.admitted)
            throw new IllegalStateException("Native Sprint observation frame rejected");
        status += " | sprintPreview key="+decision.proposedKeyPressed+
            " speedProposal="+(decision.hasSpeed ? decision.proposedSpeed : "none");
        if (R208_LOCAL_WORLD_TEST && playerReady && mc.isSingleplayer()) {
            if (r208FirstWorldMs==0) r208FirstWorldMs=frame.monotonicMs;
            ++r208WorldPlayerTicks;
            final long gap=frame.monotonicMs-r209LastSampleMs;
            if(r209LastSampleMs!=0 && gap>=0) {
                r209MinGapMs=Math.min(r209MinGapMs,gap);
                r209MaxGapMs=Math.max(r209MaxGapMs,gap);
            }
            r209LastSampleMs=frame.monotonicMs;
            if (R209_PLAYER_TICK_TEST && !r209TickReceipt &&
                r208WorldPlayerTicks>=32) {
                r209TickReceipt=true;
                System.out.println("[SlinkyWinkyPreview] " +
                    "R209_PLAYER_TICK_SPRINT_JNI_PASS" +
                    " playerTickSamples="+r208WorldPlayerTicks +
                    " clientTicks="+r208WorldClientTicks +
                    " elapsedMs="+(frame.monotonicMs-r208FirstWorldMs) +
                    " minSampleGapMs="+r209MinGapMs +
                    " maxSampleGapMs="+r209MaxGapMs +
                    " inputForward="+movement[0] +
                    " inputStrafe="+movement[1] +
                    " proposedKeyPressed="+decision.proposedKeyPressed +
                    " proposedSpeed="+decision.hasSpeed +
                    " speedSource=getWalkSpeed_proxy");
            } else if (!R209_PLAYER_TICK_TEST && !r208ObservationReceipt &&
                       r208WorldPlayerTicks>=32) {
                r208ObservationReceipt=true;
                System.out.println("[SlinkyWinkyPreview] " +
                    "R208_IN_WORLD_SPRINT_JNI_OBSERVATION_PASS" +
                    " playerTickSamples="+r208WorldPlayerTicks +
                    " clientTicks="+r208WorldClientTicks +
                    " elapsedMs="+(frame.monotonicMs-r208FirstWorldMs) +
                    " latestKeyProposal="+decision.proposedKeyPressed +
                    " latestSpeedProposal="+decision.hasSpeed);
            }
        }
        // Intentionally NO KeyBinding.setKeyBindState(), setSprinting(),
        // player motion writes, game packet sends, or synthetic input.
    }

    @SubscribeEvent public void onOverlay(RenderGameOverlayEvent.Text event) {
        Minecraft mc=Minecraft.getMinecraft();
        if(mc.fontRendererObj!=null) mc.fontRendererObj.drawStringWithShadow(
            "SlinkyWinky: "+status,4,4,0xFFDDDDDD);
    }
}
