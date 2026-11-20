package dev.slinkywinky.forge;

import dev.slinkywinky.GameHost;
import dev.slinkywinky.HostSettings;
import dev.slinkywinky.NativeEngine;
import dev.slinkywinky.OwnedModuleActions;
import dev.slinkywinky.SprintObservationProbe;
import dev.slinkywinky.ClutchWorldSampler;
import dev.slinkywinky.ClutchNativePreview;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiMainMenu;
import net.minecraft.world.WorldSettings;
import net.minecraft.world.WorldType;
import net.minecraft.client.settings.KeyBinding;
import net.minecraft.entity.Entity;
import net.minecraft.item.Item;
import net.minecraft.init.Items;
import net.minecraft.util.MovementInput;
import net.minecraft.util.MovementInputFromOptions;
import net.minecraft.client.entity.EntityPlayerSP;
import net.minecraft.item.ItemAxe;
import net.minecraft.item.ItemStack;
import net.minecraft.item.ItemSword;
import net.minecraft.util.MovingObjectPosition;
import net.minecraft.util.Vec3;
import net.minecraftforge.client.event.RenderGameOverlayEvent;
import net.minecraftforge.common.MinecraftForge;
import net.minecraftforge.event.entity.living.LivingEvent;
import net.minecraftforge.fml.client.registry.ClientRegistry;
import net.minecraftforge.fml.common.FMLCommonHandler;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.common.event.FMLInitializationEvent;
import net.minecraftforge.fml.common.eventhandler.SubscribeEvent;
import net.minecraftforge.fml.common.gameevent.TickEvent;
import org.lwjgl.input.Keyboard;
import org.lwjgl.input.Mouse;

/** Forge 1.8.9 read-only preview by default. R212 permits bounded
 *  local-only actions behind a separately verified TEMP/offline gate. */
@Mod(modid="slinkywinky",name="SlinkyWinky Preview",version="0.1.0",clientSideOnly=true)
public final class ForgePreview {
    private KeyBinding toggle;
    private GameHost host;
    private SprintObservationProbe sprintProbe;
    private OwnedModuleActions moduleActions;
    private static volatile ForgePreview r214Active;
    private long r214HookAdmitted, r214HookWrites, r214HookFirstMs, r214LastSampleMs;
    private boolean r214InputFixtureArmed, r214ReceiptEmitted;
    private MovementInput r214OriginalInput;
    private EntityPlayerSP r214FixturePlayer;
    private long r214DiagnosticClientTicks, r214PreLivingItemUseRequests;
    private int r214Phase, r214PhaseCount;
    // Six category windows for R217, with the original R216 3-state
    // measurement preserved exactly in its first three entries.
    private final double[] r216PhaseDistance = new double[6];
    private final int[] r216PhaseSamples = new int[6];
    private double r216LastX, r216LastZ;
    private boolean r216PositionReady;
    private int r216GroundWarmupCalls;
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
    // R224: source-grounded real Forge PlayerTick -> read-only Strafe JNI.
    private boolean r224ForwardArmed,r224ReceiptEmitted,r224CleanupDone;
    // R237 source-ordered real-world block observations. Read-only.
    private ClutchWorldSampler r237Sampler;
    private ClutchNativePreview r239Native;
    private int r239NativePlans,r241AimReceipts;
    private Object r237World,r237Player;
    private long r237Generation=1,r237Tick;
    private int r237Samples,r237NonAir;
    private int r224WorldSamples,r224MotionSamples,r224Proposals;
    private double r224StartX,r224StartZ,r224LastX,r224LastZ;
    private long r213InWorldTicks;
    private boolean r213JumpEnabledVerified, r213JumpDisabledVerified,
                    r213JumpReenabledVerified;
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
    private static final boolean R224_LOCAL_STRAFE_TEST =
        Boolean.getBoolean("slinkywinky.r224LocalStrafeReadOnlyTest");
    private static final boolean R237_CLUTCH_READONLY =
        Boolean.getBoolean("slinkywinky.r237ClutchReadOnlyProbe");
    // R212 is a deliberately separate, TEMP-only local singleplayer test.
    // Default Forge preview remains read-only, including normal F9 toggle.
    private static final boolean R217_ITEM_CATEGORIES_TEST =
        Boolean.getBoolean("slinkywinky.r217ItemCategoriesTest");
    private static final boolean R216_DISPLACEMENT_TEST =
        Boolean.getBoolean("slinkywinky.r216DisplacementTest") ||
        R217_ITEM_CATEGORIES_TEST;
    private static final boolean R215_VANILLA_INPUT_TEST =
        Boolean.getBoolean("slinkywinky.r215VanillaInputTest") ||
        R216_DISPLACEMENT_TEST;
    private static final boolean R214_LOCAL_NO_SLOW_TEST =
        Boolean.getBoolean("slinkywinky.r214LocalNoSlowAcceptance") ||
        R215_VANILLA_INPUT_TEST;
    private static final boolean R213_LOCAL_ACTION_ACCEPTANCE =
        Boolean.getBoolean("slinkywinky.r213LocalActionAcceptance");
    private static final boolean R212_ACTION_TEST =
        Boolean.getBoolean("slinkywinky.r212ActionTest") ||
        R213_LOCAL_ACTION_ACCEPTANCE || R214_LOCAL_NO_SLOW_TEST;
    // Test-only: create a local world only in a named TEMP game directory.
    private static final boolean R208_LOCAL_WORLD_TEST =
        Boolean.getBoolean("slinkywinky.r208LocalWorldTest") ||
        R209_PLAYER_TICK_TEST || R224_LOCAL_STRAFE_TEST;
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
        System.out.println("[SlinkyWinkyPreview] initialized - " +
            (R212_ACTION_TEST ? "isolated R212 action-trial mode" : "read-only"));
        if (R212_ACTION_TEST) {
            final Minecraft mc = Minecraft.getMinecraft();
            final String gameDir = mc.mcDataDir.getAbsolutePath().replace('\\','/');
            final String tempDir = System.getProperty("java.io.tmpdir").replace('\\','/');
            final String prefix=R217_ITEM_CATEGORIES_TEST ?
                "/slinkywinky-r217-independent-forge-" :
                R216_DISPLACEMENT_TEST ?
                "/slinkywinky-r216-independent-forge-" :
                R215_VANILLA_INPUT_TEST ?
                "/slinkywinky-r215-independent-forge-" :
                R214_LOCAL_NO_SLOW_TEST ?
                "/slinkywinky-r214-independent-forge-" :
                R213_LOCAL_ACTION_ACCEPTANCE ?
                "/slinkywinky-r213-independent-forge-" :
                "/slinkywinky-r212-independent-forge-";
            final String username=R217_ITEM_CATEGORIES_TEST ?
                "R217Offline" : R216_DISPLACEMENT_TEST ?
                "R216Offline" : R215_VANILLA_INPUT_TEST ?
                "R215Offline" : R214_LOCAL_NO_SLOW_TEST ?
                "R214Offline" : R213_LOCAL_ACTION_ACCEPTANCE ?
                "R213Offline" : "R212Offline";
            if (!SPRINT_OBSERVATION || R208_LOCAL_WORLD_TEST ||
                (R214_LOCAL_NO_SLOW_TEST &&
                 !Boolean.getBoolean("slinkywinky.r214NoSlowAgent")) ||
                !gameDir.startsWith(tempDir) ||
                !gameDir.contains(prefix) ||
                mc.getSession() == null ||
                !username.equals(mc.getSession().getUsername())) {
                throw new IllegalStateException(
                    "R212 game actions require a named TEMP-only offline test");
            }
            System.out.println("[SlinkyWinkyPreview] " +
                (R217_ITEM_CATEGORIES_TEST ?
                    "R217_CATEGORIES_ISOLATION_GATE_PASS" :
                    R216_DISPLACEMENT_TEST ?
                    "R216_DISPLACEMENT_ISOLATION_GATE_PASS" :
                    R215_VANILLA_INPUT_TEST ?
                    "R215_VANILLA_INPUT_ISOLATION_GATE_PASS" :
                    R214_LOCAL_NO_SLOW_TEST ?
                    "R214_ACTION_ISOLATION_GATE_PASS" :
                    R213_LOCAL_ACTION_ACCEPTANCE ?
                    "R213_ACTION_ISOLATION_GATE_PASS" :
                    "R212_ACTION_ISOLATION_GATE_PASS"));
            if(R214_LOCAL_NO_SLOW_TEST) {
                // The independent test JVM is unattended. Keep its
                // integrated local server ticking even without window
                // focus; this writes no original Minecraft setting.
                mc.gameSettings.pauseOnLostFocus=false;
                r214Active=this;
                System.out.println("[SlinkyWinkyPreview] " +
                    "R214_LOCAL_SERVER_UNFOCUSED_TICKING_ENABLED");
            }
        }
        if (R208_LOCAL_WORLD_TEST) {
            final Minecraft mc = Minecraft.getMinecraft();
            final String home = mc.mcDataDir.getAbsolutePath().replace('\\', '/');
            final String temp = System.getProperty("java.io.tmpdir").replace('\\', '/');
            final String instancePrefix = R224_LOCAL_STRAFE_TEST ?
                "/slinkywinky-r224-independent-forge-" :
                R209_PLAYER_TICK_TEST ?
                "/slinkywinky-r209-independent-forge-" :
                "/slinkywinky-r208-independent-forge-";
            final String expectedUser = R224_LOCAL_STRAFE_TEST ?
                "R224Offline" :
                R209_PLAYER_TICK_TEST ? "R209Offline" : "R208Offline";
            if (!SPRINT_OBSERVATION || !home.startsWith(temp) ||
                !home.contains(instancePrefix) || mc.getSession() == null ||
                !expectedUser.equals(mc.getSession().getUsername())) {
                throw new IllegalStateException(
                    "R208 local world test requires TEMP gameDir, dummy offline " +
                    "identity and the read-only Sprint observer");
            }
            System.out.println("[SlinkyWinkyPreview] " +
                (R224_LOCAL_STRAFE_TEST ? "R224_STRAFE_ISOLATION_GATE_PASS" :
                 R209_PLAYER_TICK_TEST ? "R209_ISOLATION_GATE_PASS" :
                                         "R208_ISOLATION_GATE_PASS"));
            if(R224_LOCAL_STRAFE_TEST)
                mc.gameSettings.pauseOnLostFocus=false;
        }
        // R237 is never admitted in ordinary profiles or action-test modes.
        // The operator must independently enter a local offline world and
        // press F9; this option does not create worlds or modify keys.
        if(R237_CLUTCH_READONLY) {
            final Minecraft mc=Minecraft.getMinecraft();
            final String home=mc.mcDataDir.getAbsolutePath().replace('\\','/');
            final String temp=System.getProperty("java.io.tmpdir").replace('\\','/');
            if(!SPRINT_OBSERVATION || R212_ACTION_TEST ||
               R208_LOCAL_WORLD_TEST || !home.startsWith(temp) ||
               !home.contains("/slinkywinky-r237-independent-forge-") ||
               mc.getSession()==null ||
               !"R237Offline".equals(mc.getSession().getUsername()))
                throw new IllegalStateException(
                    "R237 requires isolated TEMP offline read-only Forge");
            System.out.println("[SlinkyWinkyPreview] R237_CLUTCH_ISOLATION_GATE_PASS");
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
        if(r239Native!=null) {r239Native.close();r239Native=null;}
        if(r237Sampler!=null) {r237Sampler.close();r237Sampler=null;}
        r237World=r237Player=null;
        if(R214_LOCAL_NO_SLOW_TEST && r214Active==this)
            r214Active=null;
        try {
            // Restore only still-owned local movement fields before closing
            // the session. If the player has gone, stale objects are never written.
            if (moduleActions != null) {
                try {
                    moduleActions.close(
                        ForgeModuleActionPort.currentOrNull(Minecraft.getMinecraft()));
                } finally { moduleActions=null; }
            }
            if(sprintProbe!=null) sprintProbe.close();
        } finally {
            if(R224_LOCAL_STRAFE_TEST && r224ForwardArmed) {
            final Minecraft mc=Minecraft.getMinecraft();
            KeyBinding.setKeyBindState(
                mc.gameSettings.keyBindForward.getKeyCode(),false);
            r224ForwardArmed=false;
        }
        if(R214_LOCAL_NO_SLOW_TEST && r214InputFixtureArmed) {
                Minecraft mc=Minecraft.getMinecraft();
                if(R215_VANILLA_INPUT_TEST) {
                    KeyBinding.setKeyBindState(
                        mc.gameSettings.keyBindForward.getKeyCode(),false);
                } else if(r214FixturePlayer!=null &&
                   mc.thePlayer==r214FixturePlayer &&
                   mc.thePlayer.movementInput instanceof R214TestMovementInput) {
                    mc.thePlayer.movementInput=r214OriginalInput;
                }
                r214InputFixtureArmed=false;
                r214OriginalInput=null;
                r214FixturePlayer=null;
            }
            moduleActions=null;
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
            if(R212_ACTION_TEST) {
                if(sprintProbe==null)
                    throw new IllegalStateException("R212 native module observer unavailable");
                moduleActions=new OwnedModuleActions();
                sprintProbe.setModuleEnabled(SprintObservationProbe.NO_JUMP_DELAY,true);
                sprintProbe.setModuleEnabled(SprintObservationProbe.NO_SLOW,true);
                if(R217_ITEM_CATEGORIES_TEST)
                    sprintProbe.setNoSlowSettings(1.0f,0.65f,0.45f);
                else
                    sprintProbe.setNoSlowSettings(1.0f,1.0f,1.0f);
                if(R214_LOCAL_NO_SLOW_TEST)
                    sprintProbe.setModuleEnabled(
                        SprintObservationProbe.NO_JUMP_DELAY,false);
            }
            enabled=true;
            r237Samples=r237NonAir=r239NativePlans=r241AimReceipts=0;
            r237Tick=0;
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
            if(R212_ACTION_TEST && moduleActions!=null)
                moduleActions.release(ForgeModuleActionPort.currentOrNull(mc));
            // PlayerTickEvent does not fire after a disconnect. Admit one
            // absent-player observation and retire the native session.
            GameHost.Frame absent=new GameHost.Frame();
            absent.monotonicMs=System.nanoTime()/1000000L;
            absent.worldToken=mc.theWorld;
            absent.playerToken=null;
            sampleReadOnlySprint(mc,absent);
            System.out.println("[SlinkyWinkyPreview] R209_WORLD_ABSENCE_OBSERVED");
        }
        if (!R208_LOCAL_WORLD_TEST && !R213_LOCAL_ACTION_ACCEPTANCE &&
            !R214_LOCAL_NO_SLOW_TEST) return;
        if (!r208WorldRequested &&
            mc.theWorld==null && mc.currentScreen instanceof GuiMainMenu &&
            ++r208MenuTicks >= 30) {
            r208WorldRequested=true;
            System.out.println("[SlinkyWinkyPreview] " +
                (R224_LOCAL_STRAFE_TEST ? "R224_LOCAL_WORLD_REQUESTED" :
                 R217_ITEM_CATEGORIES_TEST ? "R217_LOCAL_WORLD_REQUESTED" :
                    R216_DISPLACEMENT_TEST ? "R216_LOCAL_WORLD_REQUESTED" :
                    R215_VANILLA_INPUT_TEST ? "R215_LOCAL_WORLD_REQUESTED" :
                    R214_LOCAL_NO_SLOW_TEST ? "R214_LOCAL_WORLD_REQUESTED" :
                    R213_LOCAL_ACTION_ACCEPTANCE ? "R213_LOCAL_WORLD_REQUESTED" :
                    R209_PLAYER_TICK_TEST ? "R209_LOCAL_WORLD_REQUESTED" :
                                            "R208_LOCAL_WORLD_REQUESTED"));
            // Opt-in writes a new save ONLY inside the isolated TEMP gameDir.
            final String worldName = R224_LOCAL_STRAFE_TEST ?
                "R224-local-only" : R217_ITEM_CATEGORIES_TEST ?
                "R217-local-only" : R216_DISPLACEMENT_TEST ?
                "R216-local-only" : R215_VANILLA_INPUT_TEST ?
                "R215-local-only" : R214_LOCAL_NO_SLOW_TEST ?
                "R214-local-only" : R213_LOCAL_ACTION_ACCEPTANCE ?
                "R213-local-only" :
                R209_PLAYER_TICK_TEST ? "R209-local-only" : "R208-local-only";
            mc.launchIntegratedServer(worldName, "Isolated module diagnostic",
                new WorldSettings(209L, WorldSettings.GameType.CREATIVE,
                    false, false, WorldType.FLAT));
        }
        if (mc.theWorld!=null && mc.thePlayer!=null && mc.isSingleplayer()) {
            ++r208WorldClientTicks;
            if (!r208WorldReady) {
                r208WorldReady=true;
                System.out.println("[SlinkyWinkyPreview] " +
                    (R224_LOCAL_STRAFE_TEST ? "R224_LOCAL_WORLD_PLAYER_READY_PASS" :
                     R209_PLAYER_TICK_TEST ? "R209_LOCAL_WORLD_PLAYER_READY_PASS" :
                                            "R208_LOCAL_WORLD_PLAYER_READY_PASS"));
            }
            if (!enabled &&
                !(R214_LOCAL_NO_SLOW_TEST && r214ReceiptEmitted) &&
                !(R224_LOCAL_STRAFE_TEST && r224ReceiptEmitted))
                startPreview(); // completed TEMP-only tests never auto-restart
            if(R224_LOCAL_STRAFE_TEST && enabled &&
               !r224ReceiptEmitted && !r224ForwardArmed) {
                if (!(mc.thePlayer.movementInput instanceof MovementInputFromOptions))
                    throw new IllegalStateException(
                        "R224 requires original vanilla MovementInputFromOptions");
                r224StartX=mc.thePlayer.posX;
                r224StartZ=mc.thePlayer.posZ;
                KeyBinding.setKeyBindState(
                    mc.gameSettings.keyBindForward.getKeyCode(),true);
                r224ForwardArmed=true;
                System.out.println("[SlinkyWinkyPreview] " +
                    "R224_REAL_VANILLA_INPUT_ARMED source=MovementInputFromOptions " +
                    "synthetic_key=forward");
            }
            if(R224_LOCAL_STRAFE_TEST && r224ReceiptEmitted &&
               !r224CleanupDone) {
                r224CleanupDone=true;
                stop();
                if(enabled || sprintProbe!=null || host!=null ||
                   r224ForwardArmed ||
                   mc.gameSettings.keyBindForward.isKeyDown())
                    throw new IllegalStateException("R224 cleanup retained state");
                System.out.println("[SlinkyWinkyPreview] " +
                    "R224_READONLY_STRAFE_CLEANUP_PASS key_released=1 " +
                    "native_owner_closed=1 preview_closed=1 auto_restart=0 " +
                    "gameplay_motion_writes=0");
            }
            if(R214_LOCAL_NO_SLOW_TEST && enabled) {
                if(++r214DiagnosticClientTicks==20 ||
                   r214DiagnosticClientTicks==60) {
                    final ItemStack item=mc.thePlayer.getHeldItem();
                    System.out.println("[SlinkyWinkyPreview] R214_PLAYER_STATE_DIAGNOSTIC " +
                        "tick="+r214DiagnosticClientTicks+
                        " paused="+mc.isGamePaused()+
                        " pauseOnLostFocus="+mc.gameSettings.pauseOnLostFocus+
                        " focus="+mc.inGameHasFocus+
                        " screen="+(mc.currentScreen==null?"none":
                            mc.currentScreen.getClass().getSimpleName())+
                        " usingItem="+mc.thePlayer.isUsingItem()+
                        " itemId="+(item==null?-1:Item.getIdFromItem(item.getItem()))+
                        " forward="+mc.thePlayer.movementInput.moveForward+
                        " movementClass="+mc.thePlayer.movementInput.getClass().getSimpleName());
                }
                if(r214ReceiptEmitted) {
                    stop();
                    System.out.println("[SlinkyWinkyPreview] " +
                        (R217_ITEM_CATEGORIES_TEST ?
                            "R217_CATEGORY_ACTION_CLEANUP_PASS" :
                            R216_DISPLACEMENT_TEST ?
                            "R216_DISPLACEMENT_AND_ACTION_CLEANUP_PASS" :
                            R215_VANILLA_INPUT_TEST ?
                            "R215_VANILLA_INPUT_CLEANUP_PASS" :
                            "R214_INPUT_FIXTURE_AND_ACTION_CLEANUP_PASS"));
                } else {
                    armR214LocalInputFixture(mc);
                }
            }
        }
    }

    // Test-only deterministic movement-input fixture. Minecraft's REAL
    // EntityPlayerSP.onLivingUpdate still calls updatePlayerMoveState,
    // scales the returned 1.0 by vanilla 0.2, then runs the injected
    // after-slow/pre-consumption callback. No OS/physical key is held.
    private static final class R214TestMovementInput extends MovementInput {
        @Override public void updatePlayerMoveState() {
            moveForward=1.0f;
            moveStrafe=0.0f;
            jump=false;
            sneak=false;
        }
    }

    private void armR214LocalInputFixture(Minecraft mc) {
        if(r214ReceiptEmitted) return;
        if(!r214InputFixtureArmed){
            r214FixturePlayer=mc.thePlayer;
            r214OriginalInput=mc.thePlayer.movementInput;
            if(R215_VANILLA_INPUT_TEST) {
                if(!(mc.thePlayer.movementInput instanceof MovementInputFromOptions))
                    throw new IllegalStateException(
                        "R215 requires original Minecraft MovementInputFromOptions");
                // Local dummy-profile key-binding stimulus, NOT physical input.
                KeyBinding.setKeyBindState(
                    mc.gameSettings.keyBindForward.getKeyCode(),true);
            } else {
                mc.thePlayer.movementInput=new R214TestMovementInput();
            }
            mc.thePlayer.inventory.currentItem=0;
            mc.thePlayer.inventory.setInventorySlotContents(
                0,new ItemStack(R217_ITEM_CATEGORIES_TEST ?
                    Items.diamond_sword : Items.bow));
            r214InputFixtureArmed=true;
            System.out.println("[SlinkyWinkyPreview] " +
                (R217_ITEM_CATEGORIES_TEST ?
                    "R217_REAL_MOVEMENT_INPUT_ATTACHED synthetic_binding=forward" :
                    R216_DISPLACEMENT_TEST ?
                    "R216_REAL_MOVEMENT_INPUT_ATTACHED synthetic_binding=forward" :
                    R215_VANILLA_INPUT_TEST ?
                    "R215_REAL_MOVEMENT_INPUT_ATTACHED synthetic_binding=forward" :
                    "R214_LOCAL_INPUT_FIXTURE_ARMED deterministic_moveForward=1.0"));
        }
        if(mc.thePlayer!=r214FixturePlayer ||
           (R215_VANILLA_INPUT_TEST ?
            mc.thePlayer.movementInput!=r214OriginalInput ||
            !(mc.thePlayer.movementInput instanceof MovementInputFromOptions) :
            !(mc.thePlayer.movementInput instanceof R214TestMovementInput)))
            throw new IllegalStateException("R214/R215 local fixture player replaced");
        ItemStack item=mc.thePlayer.getHeldItem();
        if(item!=null &&
           (R217_ITEM_CATEGORIES_TEST ?
               (r214Phase<3 ? item.getItem()==Items.diamond_sword :
                                item.getItem()==Items.apple) :
               item.getItem()==Items.bow) &&
           !mc.thePlayer.isUsingItem())
            mc.thePlayer.setItemInUse(item,item.getMaxItemUseDuration());
    }

    private long nextR214MonotonicMs() {
        long now=System.nanoTime()/1000000L;
        if(now<=r214LastSampleMs) now=r214LastSampleMs+1;
        r214LastSampleMs=now;
        return now;
    }

    /** Called by EXACTLY ONE R214 ASM insertion after the two vanilla
     * 0.2f movement field stores in EntityPlayerSP.onLivingUpdate.
     * Cannot be called from another user's normal game installation.
     */
    static void afterR214VanillaSlowdown(EntityPlayerSP player) {
        ForgePreview host=r214Active;
        if(host!=null) host.acceptR214PreConsume(player);
    }

    private void acceptR214PreConsume(EntityPlayerSP player) {
        if(!R214_LOCAL_NO_SLOW_TEST || !enabled || sprintProbe==null ||
           moduleActions==null || r214ReceiptEmitted) return;
        final Minecraft mc=Minecraft.getMinecraft();
        if(player!=mc.thePlayer || mc.theWorld==null || !mc.theWorld.isRemote ||
           !mc.isSingleplayer() || mc.getSession()==null ||
           !(R217_ITEM_CATEGORIES_TEST ?
               "R217Offline" : R216_DISPLACEMENT_TEST ?
               "R216Offline" : R215_VANILLA_INPUT_TEST ?
               "R215Offline" : "R214Offline").equals(
                   mc.getSession().getUsername()) ||
           !mc.mcDataDir.getAbsolutePath().replace('\\','/').contains(
               R217_ITEM_CATEGORIES_TEST ?
               "/slinkywinky-r217-independent-forge-" :
               R216_DISPLACEMENT_TEST ?
               "/slinkywinky-r216-independent-forge-" :
               R215_VANILLA_INPUT_TEST ?
               "/slinkywinky-r215-independent-forge-" :
               "/slinkywinky-r214-independent-forge-")) return;
        final ItemStack held=player.getHeldItem();
        if(!player.isUsingItem() || held==null) return;
        final float beforeForward=player.movementInput.moveForward;
        if(R215_VANILLA_INPUT_TEST &&
           (!(player.movementInput instanceof MovementInputFromOptions) ||
            !mc.gameSettings.keyBindForward.isKeyDown()))
            throw new IllegalStateException(
                "R215 actual vanilla movement reader or local bound key unavailable");
        final float beforeStrafe=player.movementInput.moveStrafe;
        if(beforeForward==0 && beforeStrafe==0) return;
        if(sprintWorld!=mc.theWorld || sprintPlayer!=player) {
            sprintWorld=mc.theWorld;
            sprintPlayer=player;
            ++sprintGeneration;
        }
        try {
            final ForgeModuleActionPort port=new ForgeModuleActionPort(mc);
            final int item=Item.getIdFromItem(held.getItem());
            final SprintObservationProbe.ModuleDecision proposed =
                sprintProbe.observeModules(
                    new long[]{nextR214MonotonicMs(),sprintGeneration},
                    new int[]{3,mc.currentScreen!=null?1:0,
                        player.isSprinting()?1:0,1,0,0,0,0},
                    new double[]{player.posX,player.posY,player.posZ,0,0,0},
                    new float[]{player.rotationYaw,player.rotationPitch},
                    new float[]{beforeForward,beforeStrafe,
                                player.capabilities.getWalkSpeed()},
                    new int[]{1,Math.max(0,port.readJumpTicks()),1,1,item},
                    new float[]{1,1,1});
            if(!proposed.sprint.admitted || !proposed.sprint.worldReady)
                throw new IllegalStateException("R214 native world frame refused");
            final boolean active=sprintProbe.moduleState(
                SprintObservationProbe.NO_SLOW).activeInWorld;
            final boolean expectedActive=R217_ITEM_CATEGORIES_TEST ?
                (r214Phase % 3 != 1) : r214Phase!=1;
            if(active!=expectedActive ||
               proposed.noSlowEvaluated!=expectedActive ||
               proposed.replaceInput!=expectedActive)
                throw new IllegalStateException("R214 native module toggle not reflected");
            final OwnedModuleActions.Result applied=moduleActions.apply(
                new OwnedModuleActions.Input(
                    true,true,sprintGeneration,active,proposed.replaceInput,
                    proposed.proposedForward,proposed.proposedStrafe,
                    false,false,0,true /* ASM verified precise phase */),port);
            if(!applied.admitted || applied.noSlowBlockedByPhase ||
               Math.abs(beforeForward-0.2f)>0.0001f)
                throw new IllegalStateException("R214 vanilla 0.2 source not observed");
            ++r214HookAdmitted;
            final float expectedMovement=R217_ITEM_CATEGORIES_TEST ?
                r217ExpectedForward() : 1.0f;
            if(R217_ITEM_CATEGORIES_TEST &&
               item!=(r214Phase<3?276:260))
                throw new IllegalStateException(
                    "R217 native category item ID not the selected test item");
            if(expectedActive){
                if(!applied.noSlowWritten ||
                   Math.abs(port.readForward()-expectedMovement)>0.0001f)
                    throw new IllegalStateException(
                        "R214/R217 active No Slow failed real multiplier field");
                ++r214HookWrites;
            }else{
                if(applied.noSlowWritten ||
                   Math.abs(port.readForward()-0.2f)>0.0001f)
                    throw new IllegalStateException("R214 disabled No Slow wrote input");
            }
            // Each R216 window measures actual player posX/posZ change
            // from one REAL onLivingUpdate callback to the next. Initial
            // frames after each toggle are discarded to avoid residual
            // movement momentum skewing the comparison.
            if(R216_DISPLACEMENT_TEST &&
               !sampleR216ActualDisplacement(player))
                return; // Spawn still falling: do not count as a movement tick.
            ++r214PhaseCount;
            if(r214HookFirstMs==0) r214HookFirstMs=System.currentTimeMillis();
            if(R216_DISPLACEMENT_TEST) {
                if(r214PhaseCount>=45) {
                    if(R217_ITEM_CATEGORIES_TEST)
                        completeR217CategoryPhase(player);
                    else
                        completeR216DisplacementPhase();
                }
            } else if(r214PhaseCount>=10){
                if(r214Phase==0){
                    System.out.println("[SlinkyWinkyPreview] " +
                        "R214_NO_SLOW_ENABLED_INPUT_0P2_TO_1P0_PASS samples=10");
                    sprintProbe.setModuleEnabled(
                        SprintObservationProbe.NO_SLOW,false);
                    r214Phase=1;
                    r214PhaseCount=0;
                }else if(r214Phase==1){
                    System.out.println("[SlinkyWinkyPreview] " +
                        "R214_NO_SLOW_DISABLED_INPUT_REMAINS_0P2_PASS samples=10");
                    sprintProbe.setModuleEnabled(
                        SprintObservationProbe.NO_SLOW,true);
                    r214Phase=2;
                    r214PhaseCount=0;
                }else {
                    r214ReceiptEmitted=true;
                    // ClientTick.END releases the deterministic temporary
                    // input and native owner after this player update.
                    System.out.println("[SlinkyWinkyPreview] " +
                        "R214_NO_SLOW_REENABLED_INPUT_0P2_TO_1P0_PASS samples=10");
                    if(R215_VANILLA_INPUT_TEST)
                        System.out.println("[SlinkyWinkyPreview] " +
                            "R215_VANILLA_INPUT_THREE_PHASES_PASS " +
                            "movementClass=MovementInputFromOptions " +
                            "keySource=synthetic_local_binding " +
                            "callbacks="+r214HookAdmitted+
                            " writes="+r214HookWrites);
                    System.out.println("[SlinkyWinkyPreview] " +
                        "R214_IN_WORLD_NO_SLOW_PRECONSUME_PASS " +
                        "callbacks="+r214HookAdmitted+
                        " verifiedFieldWrites="+r214HookWrites+
                        " phases=enabled_disabled_reenabled" +
                        " beforeForward="+beforeForward+
                        " afterForward="+proposed.proposedForward+
                        " itemId="+item+
                        " elapsedMs="+(System.currentTimeMillis()-r214HookFirstMs));
                }
            }
        }catch(RuntimeException | LinkageError failure){
            System.err.println("[SlinkyWinkyPreview] R214_NO_SLOW_ACTION_FAIL "+
                failure);
            stop();
            throw new IllegalStateException("R214 local No Slow acceptance failed",failure);
        }
    }

    /** R216: on the actual game player update, sample horizontal
     * displacement between player positions, not only input fields.
     * Exactly one local synthetic forward-key stimulus is maintained.
     * 12 warmup callbacks after each native toggle, then 33 samples.
     */
    private boolean sampleR216ActualDisplacement(EntityPlayerSP player) {
        final double x=player.posX;
        final double z=player.posZ;
        if(!Double.isFinite(x) || !Double.isFinite(z) ||
           player.capabilities.isFlying ||
           player.isInWater() || player.isInLava())
            throw new IllegalStateException(
                "R216 cannot measure flying, liquid, or non-finite movement");
        if(!player.onGround) {
            if(r214PhaseCount!=0 || r216PositionReady)
                throw new IllegalStateException(
                    "R216 player left the ground during displacement window");
            if(++r216GroundWarmupCalls>80)
                throw new IllegalStateException(
                    "R216 spawn never reached a stable grounded state");
            return false;
        }
        if(!r216PositionReady)
            System.out.println("[SlinkyWinkyPreview] "+
                "R216_GROUNDED_MEASUREMENT_STARTED fallTicks="+
                r216GroundWarmupCalls);
        if(r216PositionReady && r214PhaseCount>=12) {
            final double distance=Math.hypot(x-r216LastX,z-r216LastZ);
            if(!Double.isFinite(distance) || distance>1.0)
                throw new IllegalStateException("R216 teleport/collision data rejected");
            r216PhaseDistance[r214Phase]+=distance;
            ++r216PhaseSamples[r214Phase];
        }
        r216LastX=x;
        r216LastZ=z;
        r216PositionReady=true;
        return true;
    }

    private void completeR216DisplacementPhase() {
        final int phase=r214Phase;
        final int n=r216PhaseSamples[phase];
        if(n!=33 || r214PhaseCount!=45)
            throw new IllegalStateException("R216 incomplete 45-tick displacement");
        final double average=r216PhaseDistance[phase]/n;
        if(!Double.isFinite(average) || average<=0)
            throw new IllegalStateException("R216 no positive real-world displacement");
        System.out.println("[SlinkyWinkyPreview] " +
            "R216_PHASE_DISPLACEMENT phase="+
            (phase==0?"enabled":phase==1?"disabled":"reenabled")+
            " samples="+n+
            " horizontalBlocks="+r216PhaseDistance[phase]+
            " meanBlocksPerTick="+average+
            " moduleWrites="+r214HookWrites);
        if(phase==0){
            sprintProbe.setModuleEnabled(SprintObservationProbe.NO_SLOW,false);
            r214Phase=1;
            r214PhaseCount=0;
        }else if(phase==1){
            sprintProbe.setModuleEnabled(SprintObservationProbe.NO_SLOW,true);
            r214Phase=2;
            r214PhaseCount=0;
        }else{
            final double first=r216PhaseDistance[0]/r216PhaseSamples[0];
            final double vanilla=r216PhaseDistance[1]/r216PhaseSamples[1];
            final double last=r216PhaseDistance[2]/r216PhaseSamples[2];
            final double ratio1=first/vanilla;
            final double ratio2=last/vanilla;
            final double repeat=last/first;
            if(!Double.isFinite(ratio1) || !Double.isFinite(ratio2) ||
               ratio1<1.75 || ratio2<1.75 || repeat<0.55 ||
               repeat>1.45)
                throw new IllegalStateException(
                    "R216 native module did not measurably change game displacement"+
                    " ratio1="+ratio1+" ratio2="+ratio2+" repeat="+repeat);
            r214ReceiptEmitted=true;
            System.out.println("[SlinkyWinkyPreview] " +
                "R216_REAL_WORLD_DISPLACEMENT_ACCEPTANCE_PASS "+
                "samplesPerPhase=33 callbacks="+r214HookAdmitted+
                " ownedFieldWrites="+r214HookWrites+
                " enabledMean="+first+" disabledMean="+vanilla+
                " reenabledMean="+last+
                " enabledToDisabledRatio="+ratio1+
                " reenabledToDisabledRatio="+ratio2+
                " repeatRatio="+repeat+
                " inputClass=MovementInputFromOptions"+
                " source=synthetic_local_keybind");
        }
    }

    // These six stage values come from first-party native NoSlowSettings:
    // 0 sword 65%, 1 sword vanilla, 2 sword 40%, 3 apple 45%,
    // 4 apple vanilla, 5 apple 80%. Item IDs are checked at the actual
    // hooked player-update before allowing a proposal to write.
    private float r217ExpectedForward() {
        switch(r214Phase){
            case 0: return 0.65f;
            case 1: return 0.2f;
            case 2: return 0.4f;
            case 3: return 0.45f;
            case 4: return 0.2f;
            case 5: return 0.8f;
            default: throw new IllegalStateException(
                "R217 invalid native category stage: "+r214Phase);
        }
    }

    private void completeR217CategoryPhase(EntityPlayerSP player) {
        final int phase=r214Phase;
        if(phase<0 || phase>=6 || r214PhaseCount!=45 ||
           r216PhaseSamples[phase]!=33)
            throw new IllegalStateException("R217 incomplete category movement window");
        final double mean=r216PhaseDistance[phase]/33.0;
        if(!Double.isFinite(mean) || mean<=0)
            throw new IllegalStateException("R217 nonpositive game displacement");
        System.out.println("[SlinkyWinkyPreview] " +
            "R217_CATEGORY_DISPLACEMENT item="+(phase<3?"sword":"apple")+
            " phase="+(phase%3==0?"enabled":phase%3==1?"disabled":"reconfigured")+
            " multiplier="+r217ExpectedForward()+
            " itemId="+(phase<3?276:260)+
            " samples=33 horizontalBlocks="+r216PhaseDistance[phase]+
            " meanBlocksPerTick="+mean);
        if(phase==5) {
            final double[] avg=new double[6];
            for(int i=0;i<6;i++)
                avg[i]=r216PhaseDistance[i]/r216PhaseSamples[i];
            final double sword65=avg[0]/avg[1];
            final double sword40=avg[2]/avg[1];
            final double apple45=avg[3]/avg[4];
            final double apple80=avg[5]/avg[4];
            final double vanilla=avg[4]/avg[1];
            if(Math.abs(sword65-3.25)>0.70 ||
               Math.abs(sword40-2.0)>0.55 ||
               Math.abs(apple45-2.25)>0.60 ||
               Math.abs(apple80-4.0)>0.85 ||
               Math.abs(vanilla-1.0)>0.25)
                throw new IllegalStateException(
                    "R217 natural in-world movement category/multiplier mismatch"+
                    " sword65="+sword65+" sword40="+sword40+
                    " apple45="+apple45+" apple80="+apple80+
                    " vanillaCrossItem="+vanilla);
            r214ReceiptEmitted=true;
            System.out.println("[SlinkyWinkyPreview] " +
                "R217_REAL_SWORD_CONSUMABLE_DISPLACEMENT_PASS "+
                "samplesPerPhase=33 callbacks="+r214HookAdmitted+
                " ownedFieldWrites="+r214HookWrites+
                " sword65Ratio="+sword65+
                " sword40Ratio="+sword40+
                " apple45Ratio="+apple45+
                " apple80Ratio="+apple80+
                " vanillaCrossItemRatio="+vanilla+
                " movementClass=MovementInputFromOptions"+
                " keySource=synthetic_local_binding");
            return;
        }

        // Native module settings and selection are changed using the
        // exact owner-thread JNI control interface, never a GUI demo
        // field. Changing held items is restricted to the test inventory.
        final int next=phase+1;
        if(next==1 || next==4)
            sprintProbe.setModuleEnabled(SprintObservationProbe.NO_SLOW,false);
        else if(next==2) {
            sprintProbe.setNoSlowSettings(1.0f,0.4f,0.45f);
            sprintProbe.setModuleEnabled(SprintObservationProbe.NO_SLOW,true);
        } else if(next==3) {
            // The previous frame is still processing the diamond sword;
            // use the new apple only for the following native callback.
            player.stopUsingItem();
            player.inventory.setInventorySlotContents(0,
                new ItemStack(Items.apple));
            sprintProbe.setNoSlowSettings(1.0f,0.4f,0.45f);
            sprintProbe.setModuleEnabled(SprintObservationProbe.NO_SLOW,true);
        } else if(next==5) {
            sprintProbe.setNoSlowSettings(1.0f,0.4f,0.8f);
            sprintProbe.setModuleEnabled(SprintObservationProbe.NO_SLOW,true);
        }
        r214Phase=next;
        r214PhaseCount=0;
        System.out.println("[SlinkyWinkyPreview] R217_CATEGORY_NEXT_PHASE "+
            "stage="+next+" item="+(next<3?"sword":"apple")+
            " requestedMultiplier="+r217ExpectedForward());
    }

    // Forge LivingUpdateEvent is BEFORE player onLivingUpdate, so it is
    // unsuitable for applying No Slow. It IS valid for the isolated R214
    // fixture to make the player use a local bow immediately before the
    // vanilla method refreshes input and applies its 0.2f slowdown.
    @SubscribeEvent public void onR214PreLivingUpdate(
            LivingEvent.LivingUpdateEvent event) {
        if(!R214_LOCAL_NO_SLOW_TEST || !enabled ||
           !r214InputFixtureArmed || r214ReceiptEmitted)
            return;
        Minecraft mc=Minecraft.getMinecraft();
        if(mc.theWorld==null || mc.thePlayer==null ||
           event.entityLiving!=mc.thePlayer || !mc.theWorld.isRemote ||
           !mc.isSingleplayer()) return;
        ItemStack held=mc.thePlayer.getHeldItem();
        if(held==null || (R217_ITEM_CATEGORIES_TEST ?
             (r214Phase<3 ? held.getItem()!=Items.diamond_sword :
                              held.getItem()!=Items.apple) :
             held.getItem()!=Items.bow))
            throw new IllegalStateException(
                "R214/R217 local item-use fixture lost selected item");
        mc.thePlayer.setItemInUse(held,held.getMaxItemUseDuration());
        if(R215_VANILLA_INPUT_TEST) {
            KeyBinding.setKeyBindState(
                mc.gameSettings.keyBindForward.getKeyCode(),true);
            if(!(mc.thePlayer.movementInput instanceof MovementInputFromOptions))
                throw new IllegalStateException("R215 local vanilla input replaced");
        }
        ++r214PreLivingItemUseRequests;
        if(r214PreLivingItemUseRequests==1 ||
           r214PreLivingItemUseRequests==20)
            System.out.println("[SlinkyWinkyPreview] " +
                "R214_PRE_LIVING_ITEM_USE_FIXTURE " +
                "calls="+r214PreLivingItemUseRequests+
                " usingNow="+mc.thePlayer.isUsingItem());
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
            if(R212_ACTION_TEST) {
                if(R213_LOCAL_ACTION_ACCEPTANCE)
                    prepareR213JumpFixture(mc);
                sampleOwnedModuleActions(mc,frame);
                if(R213_LOCAL_ACTION_ACCEPTANCE)
                    verifyR213JumpFixture(mc);
            } else {
                sampleReadOnlySprint(mc,frame);
                if(R224_LOCAL_STRAFE_TEST)
                    sampleR224ReadOnlyStrafe(mc,frame);
                if(R237_CLUTCH_READONLY)
                    sampleR237ReadOnlyClutch(mc,frame);
            }
        } catch (RuntimeException | LinkageError problem) {
            try {stop();} catch (RuntimeException ignored) { }
            status="Preview stopped after player-tick JNI failure";
            if(R209_PLAYER_TICK_TEST)
                System.err.println("[SlinkyWinkyPreview] R209_PLAYER_TICK_FAIL "+problem);
            if(R224_LOCAL_STRAFE_TEST)
                System.err.println("[SlinkyWinkyPreview] R224_READONLY_STRAFE_FAIL "+problem);
        }
    }

    // R213's acceptance manipulates only the freshly created TEMP
    // singleplayer player's private cooldown as a controlled fixture.
    // This proves the live Forge field write, not natural repeated jumping.
    private void prepareR213JumpFixture(Minecraft mc) {
        ++r213InWorldTicks;
        if(r213InWorldTicks==5 || r213InWorldTicks==10 ||
           r213InWorldTicks==15) {
            final ForgeModuleActionPort port=new ForgeModuleActionPort(mc);
            if(r213InWorldTicks==10)
                sprintProbe.setModuleEnabled(
                    SprintObservationProbe.NO_JUMP_DELAY,false);
            else if(r213InWorldTicks==15)
                sprintProbe.setModuleEnabled(
                    SprintObservationProbe.NO_JUMP_DELAY,true);
            port.writeJumpTicks(0); // known safe baseline, no jump input
            // A positive fixture uses the private Field through the
            // same explicitly versioned game port only in R213.
            ForgeModuleActionPort.setAcceptanceJumpTicks(mc,5);
        }
    }

    private void verifyR213JumpFixture(Minecraft mc) {
        if(r213InWorldTicks!=5 && r213InWorldTicks!=10 &&
           r213InWorldTicks!=15) return;
        final ForgeModuleActionPort port=new ForgeModuleActionPort(mc);
        final int observed=port.readJumpTicks();
        if(r213InWorldTicks==5 && observed==0){
            r213JumpEnabledVerified=true;
            System.out.println("[SlinkyWinkyPreview] R213_JUMP_ENABLED_ZERO_PASS");
        } else if(r213InWorldTicks==10 && observed==5){
            r213JumpDisabledVerified=true;
            System.out.println("[SlinkyWinkyPreview] R213_JUMP_DISABLED_NO_WRITE_PASS");
        } else if(r213InWorldTicks==15 && observed==0){
            r213JumpReenabledVerified=true;
            System.out.println("[SlinkyWinkyPreview] R213_JUMP_REENABLED_ZERO_PASS");
        } else {
            throw new IllegalStateException(
                "R213 live cooldown mismatch tick="+r213InWorldTicks+
                " observed="+observed);
        }
        if(r213InWorldTicks==15) {
            if(!r213JumpEnabledVerified || !r213JumpDisabledVerified ||
               !r213JumpReenabledVerified)
                throw new IllegalStateException("R213 incomplete action receipt");
            System.out.println("[SlinkyWinkyPreview] " +
                "R213_LOCAL_JUMP_ACTION_ACCEPTANCE_PASS " +
                "enabled_zero=1 disabled_preserved=1 reenabled_zero=1 " +
                "no_slow_phase=blocked_late_tick");
        }
    }

    /**
     * R212: an isolated, explicitly opt-in local action trial.
     * No live anti-cheat/server integration, synthetic keys or packets.
     * Full actions are only permitted in a local offline test profile.
     * Forge PlayerTickEvent.END is not authenticated original native
     * event-21 timing and still requires separate gameplay acceptance.
     */
    /**
     * A real, source-ordered 167-cell block-ID observation. Does NOT
     * execute Clutch's planner, raycast, mouse, rotation or placement.
     * Native R235 still needs a two-phase ownership-checked JNI handoff.
     */
    private void sampleR237ReadOnlyClutch(Minecraft mc,GameHost.Frame frame) {
        if(r237Samples>=20)return;
        if(!mc.isSingleplayer() || mc.theWorld==null ||
            mc.thePlayer==null || !mc.theWorld.isRemote ||
            frame.worldToken!=mc.theWorld || frame.playerToken!=mc.thePlayer)
            throw new IllegalStateException("R237 read-only world admission denied");
        if(r237Sampler==null || r237World!=mc.theWorld ||
            r237Player!=mc.thePlayer) {
            if(r239Native!=null){r239Native.close();r239Native=null;}
            if(r237Sampler!=null)r237Sampler.close();
            r237Sampler=new ClutchWorldSampler();
            r239Native=new ClutchNativePreview();
            r237World=mc.theWorld;r237Player=mc.thePlayer;
            ++r237Generation;r237Tick=0;
        }
        final net.minecraft.util.AxisAlignedBB box=
            mc.thePlayer.getEntityBoundingBox();
        final ClutchWorldSampler.Request request=new ClutchWorldSampler.Request(
            r237Generation,++r237Tick,
            new double[]{box.minX,box.minY,box.minZ},
            new double[]{box.maxX,box.maxY,box.maxZ},
            new double[]{frame.eyeX,frame.eyeY,frame.eyeZ},
            7,0.0f,null);
        final ClutchWorldSampler.Snapshot snapshot=r237Sampler.capture(
            new ForgeClutchWorldPort(mc),request);
        for(int id:snapshot.candidateBlockIds())if(id!=0)++r237NonAir;
        // R239: actual Java 8/JNI receipt into R235 native Clutch
        // value-only planner. Never send its candidate to gameplay.
        final ClutchNativePreview.Decision plan=r239Native.observe(
            request,snapshot,frame.yaw,frame.pitch,300.0f,1.0f,
            System.nanoTime()*1e-9);
        if(!plan.admitted)throw new IllegalStateException(
            "R239 native Clutch decision not admitted");
        if(plan.found) {
            ++r239NativePlans;
            // R241 exposes the source-computed aim, but never applies it.
            if(!Double.isFinite(plan.aimX) || !Double.isFinite(plan.aimY) ||
               !Double.isFinite(plan.aimZ) ||
               !Double.isFinite(plan.yawDelta) ||
               !Double.isFinite(plan.pitchDelta))
                throw new IllegalStateException("Invalid native aim receipt");
            ++r241AimReceipts;
        }
        ++r237Samples;
        if(r237Samples==20)
            System.out.println("[SlinkyWinkyPreview] " +
                "R237_REAL_WORLD_CLUTCH_167_READONLY_PASS samples=20" +
                " candidateReads=3340 nonAir="+r237NonAir+
                " nativePlans="+r239NativePlans+
                " aimReceipts="+r241AimReceipts+
                " gameplayWrites=0 raycasts=0 source=Forge_PlayerTick_END" +
                "_JNI_NATIVE_R235");

    }

    private void sampleOwnedModuleActions(Minecraft mc, GameHost.Frame frame) {
        if(!R212_ACTION_TEST || moduleActions==null || sprintProbe==null ||
           mc.theWorld==null || mc.thePlayer==null || !mc.isSingleplayer())
            throw new IllegalStateException("R212 action test lacks a local world");
        if(sprintWorld!=frame.worldToken || sprintPlayer!=frame.playerToken){
            sprintWorld=frame.worldToken;
            sprintPlayer=frame.playerToken;
            ++sprintGeneration;
        }
        final ForgeModuleActionPort port=new ForgeModuleActionPort(mc);
        final boolean using=mc.thePlayer.isUsingItem();
        final ItemStack held=mc.thePlayer.getHeldItem();
        final int itemId=held==null ? -1 : Item.getIdFromItem(held.getItem());
        final int[] flags={3,frame.guiOpen?1:0,frame.sprinting?1:0,
            using?1:0,SPRINT_GUI?1:0,SPRINT_ITEM?1:0,
            SPRINT_BACK?1:0,SPRINT_SIDE?1:0};
        final float[] movement={
            mc.thePlayer.movementInput.moveForward,
            mc.thePlayer.movementInput.moveStrafe,
            mc.thePlayer.capabilities.getWalkSpeed()};
        final int[] context={1,Math.max(0,port.readJumpTicks()),
            using?1:0,held!=null?1:0,itemId};
        final SprintObservationProbe.ModuleDecision decision =
            sprintProbe.observeModules(
                new long[]{R214_LOCAL_NO_SLOW_TEST ?
                    nextR214MonotonicMs() : frame.monotonicMs,sprintGeneration},flags,
                new double[]{frame.eyeX,frame.eyeY,frame.eyeZ,0,0,0},
                new float[]{frame.yaw,frame.pitch},movement,context,
                new float[]{1,1,1});
        if(!decision.sprint.admitted || !decision.sprint.worldReady)
            throw new IllegalStateException("R212 native module frame rejected");
        final SprintObservationProbe.ControlState jump =
            sprintProbe.moduleState(SprintObservationProbe.NO_JUMP_DELAY);
        final SprintObservationProbe.ControlState slow =
            sprintProbe.moduleState(SprintObservationProbe.NO_SLOW);
        final OwnedModuleActions.Result applied=moduleActions.apply(
            new OwnedModuleActions.Input(
                true, true, sprintGeneration,
                slow.activeInWorld, decision.replaceInput,
                decision.proposedForward, decision.proposedStrafe,
                jump.activeInWorld, decision.hasJumpTicks,
                decision.proposedJumpTicks,
                false /* Forge PlayerTick.END is too late for No Slow */),port);
        if(!applied.admitted)
            throw new IllegalStateException("R212 admitted local action rejected");
        if(applied.noSlowWritten)
            throw new IllegalStateException("R213 unverified No Slow phase wrote game input");
        status="R212/213 LOCAL TEST | jump="+applied.jumpTicksWritten+
            " slow="+applied.noSlowWritten+
            " lateSlowBlocked="+applied.noSlowBlockedByPhase+
            " | no packets or keys";
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

    /**
     * R224: real Forge PlayerTickEvent.END, actual vanilla movementInput
     * and EntityPlayerSP motion. The local forward key is deliberately
     * synthetic and TEMP-only; the Strafe *policy* remains read-only.
     * This observes Minecraft's tick, NOT the original Slinky event21/22
     * dispatch or an accepted runtime motion write.
     */
    private void sampleR224ReadOnlyStrafe(Minecraft mc,GameHost.Frame frame) {
        if(!R224_LOCAL_STRAFE_TEST || !r224ForwardArmed ||
           r224ReceiptEmitted)return;
        if(mc.theWorld!=frame.worldToken || mc.thePlayer!=frame.playerToken ||
           !mc.theWorld.isRemote || !mc.isSingleplayer() ||
           !(mc.thePlayer.movementInput instanceof MovementInputFromOptions))
            throw new IllegalStateException("R224 player/world identity lost");
        final float forward=mc.thePlayer.movementInput.moveForward;
        final float sideways=mc.thePlayer.movementInput.moveStrafe;
        final double mx=mc.thePlayer.motionX;
        final double my=mc.thePlayer.motionY;
        final double mz=mc.thePlayer.motionZ;
        final long tick=++r224WorldSamples;
        SprintObservationProbe.StrafeDecision decision=
            sprintProbe.observeStrafe(
                sprintGeneration,tick,true,true,frame.sprinting,
                forward,sideways,frame.yaw,mc.thePlayer.onGround,
                mc.thePlayer.movementInput.jump,mx,my,mz);
        if(!decision.admitted)
            throw new IllegalStateException("R224 native world/tick refused");
        if(Math.hypot(mx,mz)>0.0001 && forward!=0.0f)
            ++r224MotionSamples;
        if(decision.hasProposal) {
            if(!Double.isFinite(decision.proposedX) ||
               !Double.isFinite(decision.proposedZ) ||
               decision.proposedY!=my)
                throw new IllegalStateException("R224 invalid native motion");
            ++r224Proposals;
        }
        r224LastX=mc.thePlayer.posX;
        r224LastZ=mc.thePlayer.posZ;
        final double horizontalDistance=Math.hypot(
            r224LastX-r224StartX,r224LastZ-r224StartZ);
        if(tick>=50 && r224MotionSamples>=10 &&
           r224Proposals>=10 && horizontalDistance>0.1) {
            r224ReceiptEmitted=true;
            System.out.println("[SlinkyWinkyPreview] "+
                "R224_REAL_PLAYER_TICK_READONLY_STRAFE_JNI_PASS"+
                " samples="+r224WorldSamples+
                " movingSamples="+r224MotionSamples+
                " proposedSamples="+r224Proposals+
                " distance="+horizontalDistance+
                " gameMotionWrites=0"+
                " source=Forge_PlayerTick_END_and_vanilla_motion");
        } else if(tick>=150) {
            throw new IllegalStateException(
                "R224 real in-world motion/proposals not observed "+
                "samples="+tick+" motion="+r224MotionSamples+
                " proposals="+r224Proposals+
                " distance="+horizontalDistance);
        }
    }

    @SubscribeEvent public void onOverlay(RenderGameOverlayEvent.Text event) {
        Minecraft mc=Minecraft.getMinecraft();
        if(mc.fontRendererObj!=null) mc.fontRendererObj.drawStringWithShadow(
            "SlinkyWinky: "+status,4,4,0xFFDDDDDD);
    }
}
