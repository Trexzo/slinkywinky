package dev.slinkywinky.forge;

import dev.slinkywinky.GameHost;
import dev.slinkywinky.HostSettings;
import dev.slinkywinky.NativeEngine;
import dev.slinkywinky.OwnedModuleActions;
import dev.slinkywinky.SprintObservationProbe;
import dev.slinkywinky.ClutchWorldSampler;
import dev.slinkywinky.ClutchNativePreview;
import dev.slinkywinky.ClutchRayReceiptStatus;
import dev.slinkywinky.ClutchFaceSampler;
import dev.slinkywinky.ClutchFaceReceiptStatus;
import dev.slinkywinky.ClutchCadenceObserver;
import dev.slinkywinky.RelationshipRosterSampler;
import dev.slinkywinky.EspWorldSnapshotSampler;
import dev.slinkywinky.EspNativeReadOnlyPreview;
import dev.slinkywinky.SourceWorldReadOnlyHost;
import dev.slinkywinky.SourceRuntimePreview;
import dev.slinkywinky.SourcePacketReadOnlyQueue;
import net.minecraftforge.client.event.RenderWorldLastEvent;
import dev.slinkywinky.RelationshipRosterNativePreview;
import dev.slinkywinky.RosterIsolationGate;
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
    // R245: opt-in real Forge world ray observations; never execute Clutch.
    private ClutchWorldSampler.Pos r245Previous;
    private double[] r245PreviousAim;
    private int r245Raycasts,r245Retained,r245PriorRequests;
    // R251: opt-in previous native plan -> real cursor block/face receipt.
    private ClutchFaceSampler r251Sampler;
    private int r251PriorSamples,r251HitBlocks,r251Matches;
    private boolean r251PriorCentered;
    private ClutchCadenceObserver r252Cadence;
    private RelationshipRosterSampler r278RosterSampler;
    private int r278RosterSamples;
    private EspWorldSnapshotSampler r300EspSampler;
    private EspNativeReadOnlyPreview r301EspNative;
    // R337: opt-in real Forge tick -> Java8/JNI -> native source controllers.
    private SourceWorldReadOnlyHost r337SourceHost;
    private int r337SourceSamples;
    private ForgeSourcePacketTap r339Tap;
    private int r339PacketBatches;
    private long r339PacketObservations;
    private int r341BoxFrames;
    private boolean r341SubmittedBox;
    private long r338LastRenderNanos;
    private int r338SourceRenderSamples;
    private Object r300World,r300Owner;
    private int r300EspSamples,r301EspSamples;
    private boolean r302ReceiptEmitted,r302CleanupDone,r302StartupFailed;
    private Object r302World,r302Player;
    private long r302Generation;
    private EspNativeReadOnlyPreview.Result r303LastReceipt;
    private Object r303World,r303Player;
    private int r303RenderFrames;
    private boolean r303Started,r303SawDraw;
    private boolean r304Started,r304StartupFailed,r304SawItem,r304SawPlayer;
    private int r304RenderFrames;
    private boolean r305Started,r305Failed;
    private int r305WorldFrames,r305ScreenFrames,r305Collections;
    private long r305WorldPhaseGeneration=-1,r305WorldPhaseTick=-1;
    private ForgePlayerEspScreenRenderer.CapturedWorldFrame r306Camera;
    private boolean r306Failed,r306SawVisible;
    private int r306ScreenFrames;
    // R283: strictly read-only native source decisions; no Minecraft writes.
    private RelationshipRosterNativePreview r283Native;
    private RelationshipRosterNativePreview.Settings r283Settings;
    private int r283NativeSamples;
    private boolean r289ReceiptEmitted,r289CleanupDone,r289StartupFailed;
    private int r252Due,r252PressProposals;
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
    private static final boolean R245_CLUTCH_PRIOR_READONLY =
        Boolean.getBoolean("slinkywinky.r245ClutchPriorReadOnlyProbe");
    private static final boolean R251_CLUTCH_FACE_READONLY =
        Boolean.getBoolean("slinkywinky.r251ClutchCursorFaceReadOnlyProbe");
    private static final boolean R252_CLUTCH_CADENCE_READONLY =
        Boolean.getBoolean("slinkywinky.r252ClutchCadenceReadOnlyProbe");
    private static final boolean R278_ROSTER_READONLY =
        Boolean.getBoolean("slinkywinky.r278RosterReadOnlyProbe");
    private static final boolean R283_NATIVE_ROSTER_READONLY =
        Boolean.getBoolean("slinkywinky.r283NativeRosterReadOnlyProbe");
    private static final boolean R289_ROSTER_LOCAL_WORLD_TEST =
        Boolean.getBoolean("slinkywinky.r289RosterLocalWorldTest");
    private static final boolean R300_ESP_READONLY =
        Boolean.getBoolean("slinkywinky.r300EspReadOnlyProbe");
    private static final boolean R301_NATIVE_ESP_READONLY =
        Boolean.getBoolean("slinkywinky.r301NativeEspReadOnlyProbe");
    private static final boolean R337_SOURCE_READONLY =
        Boolean.getBoolean("slinkywinky.r337NativeSourceTickReadOnly");
    private static final boolean R339_PACKET_READONLY =
        Boolean.getBoolean("slinkywinky.r339PacketReadOnly");
    private static final boolean R341_SOURCE_BOX_DIAGNOSTIC =
        Boolean.getBoolean("slinkywinky.r341SourceBoxDiagnostic");
    private static final boolean R302_LOCAL_WORLD_TEST =
        Boolean.getBoolean("slinkywinky.r302EspLocalWorldTest");
    private static final boolean R303_CHEST_RENDER =
        Boolean.getBoolean("slinkywinky.r303ChestRenderProbe");
    private static final boolean R303_CHEST_LOCAL_WORLD_TEST =
        Boolean.getBoolean("slinkywinky.r303ChestLocalWorldTest");
    private static final boolean R304_ENTITY_RENDER =
        Boolean.getBoolean("slinkywinky.r304EntityRenderProbe");
    private static final boolean R304_LOCAL_WORLD_TEST =
        Boolean.getBoolean("slinkywinky.r304EntityLocalWorldTest");
    private static final boolean R305_SOURCE_PHASES =
        Boolean.getBoolean("slinkywinky.r305SourcePhaseTest");
    private static final boolean R305_LOCAL_WORLD_TEST =
        Boolean.getBoolean("slinkywinky.r305SourcePhaseLocalWorldTest");
    private static final boolean R306_PLAYER_2D =
        Boolean.getBoolean("slinkywinky.r306PlayerScreenRenderProbe");
    private static final boolean R306_LOCAL_WORLD_TEST =
        Boolean.getBoolean("slinkywinky.r306PlayerScreenLocalWorldTest");
    private static final boolean R307_SOURCE_SCREEN =
        Boolean.getBoolean("slinkywinky.r307SourceScreenGeometryTest");
    private static final boolean R307_LOCAL_WORLD_TEST =
        Boolean.getBoolean("slinkywinky.r307SourceScreenLocalWorldTest");
    private static final int R283_UUID_MODE =
        Integer.getInteger("slinkywinky.r283AntiBotUuidMode",0).intValue();
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
        if(R245_CLUTCH_PRIOR_READONLY && !R237_CLUTCH_READONLY)
            throw new IllegalStateException(
                "R245 previous Clutch observation requires R237 offline admission");
        if(R251_CLUTCH_FACE_READONLY &&
           (!R245_CLUTCH_PRIOR_READONLY || !R237_CLUTCH_READONLY))
            throw new IllegalStateException(
                "R251 cursor-face needs both R237 and R245 offline admission");
        if(R252_CLUTCH_CADENCE_READONLY && !R251_CLUTCH_FACE_READONLY)
            throw new IllegalStateException(
                "R252 source cadence requires R251 isolated face observations");
        if(R278_ROSTER_READONLY && !R237_CLUTCH_READONLY) {
            // Independent roster-only profile. Retain identical TEMP,
            // dummy-identity and read-only gates without running R237's
            // unrelated 167-candidate Clutch world scan each tick.
            final Minecraft mc=Minecraft.getMinecraft();
            if(mc==null || mc.mcDataDir==null || mc.getSession()==null ||
               !RosterIsolationGate.admitted(SPRINT_OBSERVATION,
                   R212_ACTION_TEST,R208_LOCAL_WORLD_TEST,
                   mc.mcDataDir.getAbsolutePath(),
                   System.getProperty("java.io.tmpdir"),
                   mc.getSession().getUsername()))
                throw new IllegalStateException(
                    "R285 roster-only preview requires isolated TEMP R278Offline");
            System.out.println("[SlinkyWinkyPreview] "+
                "R285_INDEPENDENT_ROSTER_TEMP_GATE_PASS");
        }
        if(R283_NATIVE_ROSTER_READONLY &&
           (!R278_ROSTER_READONLY || R283_UUID_MODE<0 || R283_UUID_MODE>2))
            throw new IllegalStateException(
                "R283 native roster requires R278 source and valid UUID mode");
        if(R289_ROSTER_LOCAL_WORLD_TEST) {
            if(!R278_ROSTER_READONLY || !R283_NATIVE_ROSTER_READONLY ||
               R237_CLUTCH_READONLY || R208_LOCAL_WORLD_TEST ||
               R212_ACTION_TEST || !SPRINT_OBSERVATION)
                throw new IllegalStateException(
                    "R289 requires isolated R285 TEMP roster-only profile");
            Minecraft.getMinecraft().gameSettings.pauseOnLostFocus=false;
            System.out.println("[SlinkyWinkyPreview] "+
                "R289_ROSTER_WORLD_ISOLATION_GATE_PASS");
        }
        if(R302_LOCAL_WORLD_TEST && (!R300_ESP_READONLY ||
           !R301_NATIVE_ESP_READONLY || R212_ACTION_TEST ||
           R208_LOCAL_WORLD_TEST || R289_ROSTER_LOCAL_WORLD_TEST ||
           R237_CLUTCH_READONLY || R278_ROSTER_READONLY ||
           R283_NATIVE_ROSTER_READONLY))
            throw new IllegalStateException("R302 requires isolated R300/R301 only");
        // R303 is a debug-only 3D Chest draw path. It does not enable
        // source event 6/0 equivalence or any combat/gameplay writes.
        if(R303_CHEST_RENDER && (!R300_ESP_READONLY ||
           !R301_NATIVE_ESP_READONLY || R302_LOCAL_WORLD_TEST ||
           R212_ACTION_TEST || R208_LOCAL_WORLD_TEST ||
           R237_CLUTCH_READONLY || R278_ROSTER_READONLY ||
           R283_NATIVE_ROSTER_READONLY || R289_ROSTER_LOCAL_WORLD_TEST))
            throw new IllegalStateException("R303 requires isolated R300/R301 only");
        if(R303_CHEST_LOCAL_WORLD_TEST && !R303_CHEST_RENDER)
            throw new IllegalStateException("R303 local world requires chest render");
        if(R304_ENTITY_RENDER && (!R303_CHEST_RENDER||!R301_NATIVE_ESP_READONLY||
           !R300_ESP_READONLY||R302_LOCAL_WORLD_TEST||
           R303_CHEST_LOCAL_WORLD_TEST||R212_ACTION_TEST||
           R289_ROSTER_LOCAL_WORLD_TEST||R208_LOCAL_WORLD_TEST))
            throw new IllegalStateException("R304 requires exclusive R300/R301 native ESP");
        if(R304_LOCAL_WORLD_TEST && !R304_ENTITY_RENDER)
            throw new IllegalStateException("R304 world requires entity render");
        if(R305_SOURCE_PHASES &&
           (!R305_LOCAL_WORLD_TEST||!R304_ENTITY_RENDER||
            !R303_CHEST_RENDER||!R301_NATIVE_ESP_READONLY||
            !R300_ESP_READONLY||R304_LOCAL_WORLD_TEST||
            R303_CHEST_LOCAL_WORLD_TEST||R302_LOCAL_WORLD_TEST||
            R289_ROSTER_LOCAL_WORLD_TEST||R212_ACTION_TEST))
            throw new IllegalStateException("R305 source dispatch requires isolated ESP");
        if(R305_LOCAL_WORLD_TEST && !R305_SOURCE_PHASES)
            throw new IllegalStateException("R305 local world requires source dispatch");
        if(R306_PLAYER_2D&&(!R306_LOCAL_WORLD_TEST||!R305_SOURCE_PHASES||
           !R305_LOCAL_WORLD_TEST||!R304_ENTITY_RENDER||
           R304_LOCAL_WORLD_TEST||R303_CHEST_LOCAL_WORLD_TEST||
           R302_LOCAL_WORLD_TEST||R212_ACTION_TEST))
            throw new IllegalStateException("R306 2D requires isolated source event7");
        if(R306_LOCAL_WORLD_TEST&&!R306_PLAYER_2D)
            throw new IllegalStateException("R306 local world requires 2D");
        if(R307_SOURCE_SCREEN&&(!R307_LOCAL_WORLD_TEST||
           !R306_PLAYER_2D||!R306_LOCAL_WORLD_TEST||
           !R305_SOURCE_PHASES||!R305_LOCAL_WORLD_TEST))
            throw new IllegalStateException("R307 source geometry requires isolated event7");
        if(R307_LOCAL_WORLD_TEST&&!R307_SOURCE_SCREEN)
            throw new IllegalStateException("R307 local world requires source geometry");
        // R300: independent user-opted, read-only Forge world sampler.
        // Never expose raw world/player observations outside a TEMP profile.
        if(R301_NATIVE_ESP_READONLY && !R300_ESP_READONLY)
            throw new IllegalStateException("R301 requires R300 TEMP world admission");
        if(R300_ESP_READONLY) {
            final Minecraft mc=Minecraft.getMinecraft();
            if(mc==null||mc.mcDataDir==null||mc.getSession()==null)
                throw new IllegalStateException("R300 no isolated game");
            final String home=mc.mcDataDir.getAbsolutePath().replace('\\','/');
            final String temp=System.getProperty("java.io.tmpdir").replace('\\','/');
            if(!SPRINT_OBSERVATION||R212_ACTION_TEST||R208_LOCAL_WORLD_TEST||
               R237_CLUTCH_READONLY||R278_ROSTER_READONLY||
               R283_NATIVE_ROSTER_READONLY||R289_ROSTER_LOCAL_WORLD_TEST||
               !home.startsWith(temp)||
               !(R307_LOCAL_WORLD_TEST ?
                   home.matches("(?i)^.*[/]slinkywinky-r307-independent-forge-[A-Za-z0-9._-]+$") &&
                   "R307Offline".equals(mc.getSession().getUsername()) &&
                   new java.io.File(home).getAbsoluteFile().getParentFile().equals(
                       new java.io.File(temp).getAbsoluteFile()) :
                 R306_LOCAL_WORLD_TEST ?
                   home.matches("(?i)^.*[/]slinkywinky-r306-independent-forge-[A-Za-z0-9._-]+$") &&
                   "R306Offline".equals(mc.getSession().getUsername()) &&
                   new java.io.File(home).getAbsoluteFile().getParentFile().equals(
                       new java.io.File(temp).getAbsoluteFile()) :
                 R305_LOCAL_WORLD_TEST ?
                   home.matches("(?i)^.*[/]slinkywinky-r305-independent-forge-[A-Za-z0-9._-]+$") &&
                   "R305Offline".equals(mc.getSession().getUsername()) &&
                   new java.io.File(home).getAbsoluteFile().getParentFile().equals(
                       new java.io.File(temp).getAbsoluteFile()) :
                 R304_LOCAL_WORLD_TEST ?
                   home.matches("(?i)^.*[/]slinkywinky-r304-independent-forge-[A-Za-z0-9._-]+$") &&
                   "R304Offline".equals(mc.getSession().getUsername()) &&
                   new java.io.File(home).getAbsoluteFile().getParentFile().equals(
                       new java.io.File(temp).getAbsoluteFile()) :
                 R303_CHEST_LOCAL_WORLD_TEST ?
                   home.matches("(?i)^.*[/]slinkywinky-r303-independent-forge-[A-Za-z0-9._-]+$") &&
                   "R303Offline".equals(mc.getSession().getUsername()) &&
                   new java.io.File(home).getAbsoluteFile().getParentFile().equals(
                       new java.io.File(temp).getAbsoluteFile()) :
                 R302_LOCAL_WORLD_TEST ?
                   home.matches("(?i)^.*[/]slinkywinky-r302-independent-forge-[A-Za-z0-9._-]+$") &&
                   "R302Offline".equals(mc.getSession().getUsername()) &&
                   new java.io.File(home).getAbsoluteFile().getParentFile().equals(
                       new java.io.File(temp).getAbsoluteFile()) :
                   home.contains("/slinkywinky-r300-independent-forge-") &&
                   "R300Offline".equals(mc.getSession().getUsername())))
                throw new IllegalStateException("R300 TEMP-only read-only admission denied");
            if(R302_LOCAL_WORLD_TEST)
                System.out.println("[SlinkyWinkyPreview] R302_ESP_WORLD_ISOLATION_GATE_PASS");
            if(R303_CHEST_LOCAL_WORLD_TEST)
                System.out.println("[SlinkyWinkyPreview] R303_CHEST_WORLD_ISOLATION_GATE_PASS");
            if(R304_LOCAL_WORLD_TEST)
                System.out.println("[SlinkyWinkyPreview] R304_ENTITY_WORLD_ISOLATION_GATE_PASS");
            if(R305_LOCAL_WORLD_TEST)
                System.out.println("[SlinkyWinkyPreview] R305_SOURCE_PHASE_ISOLATION_GATE_PASS");
            if(R306_LOCAL_WORLD_TEST)
                System.out.println("[SlinkyWinkyPreview] R306_PLAYER_SCREEN_ISOLATION_GATE_PASS");
            if(R307_LOCAL_WORLD_TEST)
                System.out.println("[SlinkyWinkyPreview] R307_SOURCE_SCREEN_ISOLATION_GATE_PASS");
            System.out.println("[SlinkyWinkyPreview] R300_ESP_TEMP_GATE_PASS");
        }
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
        if(r251Sampler!=null) {r251Sampler.close();r251Sampler=null;}
        if(r252Cadence!=null) {r252Cadence.close();r252Cadence=null;}
        if(r283Native!=null) {
            r283Native.close();r283Native=null;r283Settings=null;
        }
        if(r278RosterSampler!=null) {
            r278RosterSampler.close();r278RosterSampler=null;
        }
        if(r301EspNative!=null) {r301EspNative.close();r301EspNative=null;}
        if(r339Tap!=null){
            try {r339Tap.close();}finally {r339Tap=null;}
        }
        if(r337SourceHost!=null){
            try {r337SourceHost.close();}
            catch(IllegalStateException alreadyRetired) {
                // A failed world admission already forced this preview closed.
            }
            finally {r337SourceHost=null;r338LastRenderNanos=0;}
        }
        if(r300EspSampler!=null) {r300EspSampler.close();r300EspSampler=null;}
        r302World=r302Player=null;
        r302Generation=0;
        r303LastReceipt=null;r303World=r303Player=null;
        r303RenderFrames=0;r303SawDraw=false;
        r304RenderFrames=0;r304SawItem=r304SawPlayer=false;
        r305WorldFrames=r305ScreenFrames=r305Collections=0;
        r305WorldPhaseGeneration=r305WorldPhaseTick=-1;
        r306Camera=null;r306ScreenFrames=0;r306SawVisible=false;
        r300World=r300Owner=null;
        r237World=r237Player=null;
        r245Previous=null;r245PreviousAim=null;
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
            if(R278_ROSTER_READONLY)
                r278RosterSampler=new RelationshipRosterSampler();
            if(R300_ESP_READONLY)
                r300EspSampler=new EspWorldSnapshotSampler();
            if(R301_NATIVE_ESP_READONLY)
                r301EspNative=new EspNativeReadOnlyPreview(R305_SOURCE_PHASES);
            if(R337_SOURCE_READONLY)
                r337SourceHost=new SourceWorldReadOnlyHost();
            if(R341_SOURCE_BOX_DIAGNOSTIC&&!R337_SOURCE_READONLY)
                throw new IllegalStateException(
                    "R341 diagnostic boxes require the read-only source owner");
            if(R339_PACKET_READONLY){
                if(!R337_SOURCE_READONLY || R212_ACTION_TEST)
                    throw new IllegalStateException(
                        "R339 packet observation requires read-only R337 without action mode");
                r339Tap=new ForgeSourcePacketTap();
            }
            if(R283_NATIVE_ROSTER_READONLY) {
                r283Native=new RelationshipRosterNativePreview();
                r283Settings=new RelationshipRosterNativePreview.Settings(
                    false,R283_UUID_MODE,false,false,0f,false);
            }
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
            r245Raycasts=r245Retained=r245PriorRequests=0;
            r251PriorSamples=r251HitBlocks=r251Matches=0;
            r252Due=r252PressProposals=0;
            r278RosterSamples=0;
            r300EspSamples=r301EspSamples=0;
            r337SourceSamples=0;
            r339PacketBatches=0;
            r339PacketObservations=0;
            r341BoxFrames=0;
            r341SubmittedBox=false;
            r338LastRenderNanos=0;
            r338SourceRenderSamples=0;
            r303RenderFrames=0;r303SawDraw=false;
            r304RenderFrames=0;r304SawItem=r304SawPlayer=false;
            r305WorldFrames=r305ScreenFrames=r305Collections=0;
            r305WorldPhaseGeneration=r305WorldPhaseTick=-1;
            r306Camera=null;r306ScreenFrames=0;r306SawVisible=false;
            r303LastReceipt=null;r303World=r303Player=null;
            r283NativeSamples=0;
            r251PriorCentered=false;
            if(r251Sampler!=null){r251Sampler.close();r251Sampler=null;}
            r245Previous=null;r245PreviousAim=null;
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
            if(R289_ROSTER_LOCAL_WORLD_TEST) {
                r289StartupFailed=true;
                System.err.println("[SlinkyWinkyPreview] R289_PREVIEW_START_FAIL "+error);
            }
            if(R302_LOCAL_WORLD_TEST) {
                r302StartupFailed=true;
                System.err.println("[SlinkyWinkyPreview] R302_ESP_PREVIEW_START_FAIL "+error);
            }
            if(R304_LOCAL_WORLD_TEST) {
                r304StartupFailed=true;
                System.err.println("[SlinkyWinkyPreview] R304_ENTITY_PREVIEW_START_FAIL "+error);
            }
            if(R305_LOCAL_WORLD_TEST) {
                r305Failed=true;
                System.err.println("[SlinkyWinkyPreview] R305_SOURCE_PHASE_START_FAIL "+error);
            }
            if(R306_LOCAL_WORLD_TEST) {
                r306Failed=true;
                System.err.println("[SlinkyWinkyPreview] R306_PLAYER_SCREEN_START_FAIL "+error);
            }
        }
    }

    @SubscribeEvent public void onClientTick(TickEvent.ClientTickEvent event) {
        if(event.phase!=TickEvent.Phase.END) return;
        if(toggle.isPressed()) {
            if(enabled) {stop();status="F9: enable read-only preview";}
            else startPreview();
        }

        final Minecraft mc=Minecraft.getMinecraft();
        // R337 is independent of the local-world test autostart logic.
        // It never creates a world or modifies the input, timer or packets.
        if(enabled && R337_SOURCE_READONLY && r337SourceHost!=null){
            try {
                // R339: Netty-only capture; no native JNI on a network loop.
                // Channel/world swap must retire the old C++ controller epoch.
                if(r339Tap!=null&&r339Tap.reconcile(mc))
                    r337SourceHost.retire();
                if(mc.theWorld!=null&&mc.thePlayer!=null&&!mc.isSingleplayer()){
                    // Multiplayer observation is NOT an admitted source scope.
                    r337SourceHost.retire();
                }else{
                    final ForgeSourceWorldPort port=new ForgeSourceWorldPort(
                        mc,R341_SOURCE_BOX_DIAGNOSTIC);
                    final SourceWorldReadOnlyHost.TickReceipt received=
                        r337SourceHost.observe(port);
                    if(received!=null && ++r337SourceSamples==1)
                        System.out.println("[SlinkyWinkyPreview] "+
                            "R337_REAL_FORGE_SOURCE_TICK_OBSERVED "+
                            "generation="+received.generation+
                            " tick="+received.tick+
                            " reachUnverified=1 gameWrites=0");
                    if(received!=null&&r339Tap!=null){
                        final SourcePacketReadOnlyQueue incoming=r339Tap.ingress();
                        if(incoming!=null){
                            final SourceWorldReadOnlyHost.PacketReceipt batch=
                                r337SourceHost.observePackets(port,incoming);
                            if(batch!=null){
                                r339PacketObservations+=batch.incoming+batch.outgoing;
                                if(batch.callbacks>0&&++r339PacketBatches==1)
                                    System.out.println("[SlinkyWinkyPreview] "+
                                        "R339_REAL_NETTY_READONLY_JNI_PACKET_OBSERVED "+
                                        "callbacks="+batch.callbacks+
                                        " generation="+batch.generation+
                                        " gameWrites=0 packetsForwarded=1");
                            }
                        }
                    }
                }
            }catch(RuntimeException | LinkageError sourceError){
                System.err.println("[SlinkyWinkyPreview] "+
                    "R337_SOURCE_CLIENT_ADMISSION_FAIL "+sourceError);
                try {stop();}catch(RuntimeException closeFailure){
                    System.err.println("[SlinkyWinkyPreview] "+
                        "R337_SOURCE_CLOSE_FAIL "+closeFailure);
                }
                status="R337 read-only source observer stopped after failure";
            }
        }
        // A disconnected or replaced owner invalidates all R300 receipts.
        if(r300EspSampler!=null &&
           (mc.theWorld==null||mc.thePlayer==null||
            (r300World!=null && (mc.theWorld!=r300World||
                                 mc.thePlayer!=r300Owner)))) {
            if(R305_SOURCE_PHASES&&r301EspNative!=null&&r303LastReceipt!=null) {
                final EspNativeReadOnlyPreview.Result cleared=
                    r301EspNative.sourceEvent(r303LastReceipt,26);
                if(cleared.chestRows!=0||cleared.itemRows!=0||
                   cleared.playerRows!=0)
                    throw new IllegalStateException("R305 native event26 missed rows");
                System.out.println("[SlinkyWinkyPreview] "+
                    "R305_SOURCE_EVENT26_UNLOAD_PASS rows=0 gameWrites=0");
            }
            r300EspSampler.retire();
            r300World=r300Owner=null;
            r303LastReceipt=null;r303World=r303Player=null;
            r306Camera=null;
        }
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
            !R214_LOCAL_NO_SLOW_TEST && !R289_ROSTER_LOCAL_WORLD_TEST &&
            !R302_LOCAL_WORLD_TEST && !R303_CHEST_LOCAL_WORLD_TEST &&
            !R304_LOCAL_WORLD_TEST && !R305_LOCAL_WORLD_TEST &&
            !R307_LOCAL_WORLD_TEST) return;
        if (!r208WorldRequested &&
            mc.theWorld==null && mc.currentScreen instanceof GuiMainMenu &&
            ++r208MenuTicks >= 30) {
            r208WorldRequested=true;
            System.out.println("[SlinkyWinkyPreview] " +
                (R307_LOCAL_WORLD_TEST ? "R307_LOCAL_WORLD_REQUESTED" :
                 R306_LOCAL_WORLD_TEST ? "R306_LOCAL_WORLD_REQUESTED" :
                 R305_LOCAL_WORLD_TEST ? "R305_LOCAL_WORLD_REQUESTED" :
                 R304_LOCAL_WORLD_TEST ? "R304_LOCAL_WORLD_REQUESTED" :
                 R303_CHEST_LOCAL_WORLD_TEST ? "R303_LOCAL_WORLD_REQUESTED" :
                 R302_LOCAL_WORLD_TEST ? "R302_LOCAL_WORLD_REQUESTED" :
                 R289_ROSTER_LOCAL_WORLD_TEST ? "R289_LOCAL_WORLD_REQUESTED" :
                 R224_LOCAL_STRAFE_TEST ? "R224_LOCAL_WORLD_REQUESTED" :
                 R217_ITEM_CATEGORIES_TEST ? "R217_LOCAL_WORLD_REQUESTED" :
                    R216_DISPLACEMENT_TEST ? "R216_LOCAL_WORLD_REQUESTED" :
                    R215_VANILLA_INPUT_TEST ? "R215_LOCAL_WORLD_REQUESTED" :
                    R214_LOCAL_NO_SLOW_TEST ? "R214_LOCAL_WORLD_REQUESTED" :
                    R213_LOCAL_ACTION_ACCEPTANCE ? "R213_LOCAL_WORLD_REQUESTED" :
                    R209_PLAYER_TICK_TEST ? "R209_LOCAL_WORLD_REQUESTED" :
                                            "R208_LOCAL_WORLD_REQUESTED"));
            // Opt-in writes a new save ONLY inside the isolated TEMP gameDir.
            final String worldName = R307_LOCAL_WORLD_TEST ?
                "R307-local-only" : R306_LOCAL_WORLD_TEST ?
                "R306-local-only" : R305_LOCAL_WORLD_TEST ?
                "R305-local-only" : R304_LOCAL_WORLD_TEST ?
                "R304-local-only" : R303_CHEST_LOCAL_WORLD_TEST ?
                "R303-local-only" : R302_LOCAL_WORLD_TEST ?
                "R302-local-only" : R289_ROSTER_LOCAL_WORLD_TEST ?
                "R289-local-only" : R224_LOCAL_STRAFE_TEST ?
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
                    (R307_LOCAL_WORLD_TEST ? "R307_LOCAL_WORLD_PLAYER_READY_PASS" :
                     R306_LOCAL_WORLD_TEST ? "R306_LOCAL_WORLD_PLAYER_READY_PASS" :
                     R305_LOCAL_WORLD_TEST ? "R305_LOCAL_WORLD_PLAYER_READY_PASS" :
                     R304_LOCAL_WORLD_TEST ? "R304_LOCAL_WORLD_PLAYER_READY_PASS" :
                     R303_CHEST_LOCAL_WORLD_TEST ? "R303_LOCAL_WORLD_PLAYER_READY_PASS" :
                     R302_LOCAL_WORLD_TEST ? "R302_LOCAL_WORLD_PLAYER_READY_PASS" :
                     R289_ROSTER_LOCAL_WORLD_TEST ? "R289_LOCAL_WORLD_PLAYER_READY_PASS" :
                     R224_LOCAL_STRAFE_TEST ? "R224_LOCAL_WORLD_PLAYER_READY_PASS" :
                     R209_PLAYER_TICK_TEST ? "R209_LOCAL_WORLD_PLAYER_READY_PASS" :
                                            "R208_LOCAL_WORLD_PLAYER_READY_PASS"));
            }
            if (!enabled &&
                !(R214_LOCAL_NO_SLOW_TEST && r214ReceiptEmitted) &&
                !(R224_LOCAL_STRAFE_TEST && r224ReceiptEmitted) &&
                !(R289_ROSTER_LOCAL_WORLD_TEST &&
                  (r289ReceiptEmitted || r289StartupFailed)) &&
                !(R302_LOCAL_WORLD_TEST &&
                  (r302ReceiptEmitted || r302StartupFailed)) &&
                !(R303_CHEST_LOCAL_WORLD_TEST && r303Started) &&
                !(R304_LOCAL_WORLD_TEST && (r304Started||r304StartupFailed)) &&
                !(R305_LOCAL_WORLD_TEST && (r305Started||r305Failed))) {
                if(R303_CHEST_LOCAL_WORLD_TEST)r303Started=true;
                if(R304_LOCAL_WORLD_TEST)r304Started=true;
                if(R305_LOCAL_WORLD_TEST)r305Started=true;
                startPreview();
            } // completed TEMP-only tests never auto-restart
            if(R302_LOCAL_WORLD_TEST && r302ReceiptEmitted && !r302CleanupDone) {
                r302CleanupDone=true;
                if(enabled)stop();
                if(enabled||r301EspNative!=null||r300EspSampler!=null||
                   sprintProbe!=null||host!=null)
                    throw new IllegalStateException("R302 retained JNI owner");
                System.out.println("[SlinkyWinkyPreview] "+
                    "R302_ESP_NATIVE_CLEANUP_PASS "+
                    "native_owner_closed=1 preview_closed=1 auto_restart=0 gameWrites=0");
            }
            if(R289_ROSTER_LOCAL_WORLD_TEST && r289ReceiptEmitted &&
               !r289CleanupDone) {
                r289CleanupDone=true;
                if(enabled)stop();
                if(enabled || r283Native!=null || r278RosterSampler!=null ||
                   sprintProbe!=null || host!=null)
                    throw new IllegalStateException("R289 retained JNI owner");
                System.out.println("[SlinkyWinkyPreview] "+
                    "R289_ROSTER_NATIVE_CLEANUP_PASS "+
                    "native_owner_closed=1 preview_closed=1 "+
                    "auto_restart=0 gameWrites=0");
            }
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
        if(R337_SOURCE_READONLY&&r337SourceHost!=null){
            try{
                if(mc.theWorld!=null&&mc.thePlayer!=null&&mc.isSingleplayer()){
                    final long now=System.nanoTime();
                    final double delta=r338LastRenderNanos>0&&
                        now>=r338LastRenderNanos?
                        (now-r338LastRenderNanos)/1000000000d:0;
                    r338LastRenderNanos=now;
                    final SourceRuntimePreview.Receipt result=
                        r337SourceHost.observeRender(new ForgeSourceWorldPort(mc),
                            Math.min(0.25d,delta),event.renderTickTime);
                    if(result!=null&&++r338SourceRenderSamples==1)
                        System.out.println("[SlinkyWinkyPreview] "+
                            "R338_REAL_FORGE_TIMER_SOURCE_FRAME_OBSERVED "+
                            "worldGeneration="+result.generation+
                            " event="+result.sourceEvent+" gameWrites=0");
                }else r338LastRenderNanos=0;
            }catch(RuntimeException | LinkageError sourceError){
                System.err.println("[SlinkyWinkyPreview] "+
                    "R338_SOURCE_RENDER_OBSERVATION_FAIL "+sourceError);
                try{stop();}catch(RuntimeException closeError){
                    System.err.println("[SlinkyWinkyPreview] "+
                        "R338_SOURCE_RENDER_CLOSE_FAIL "+closeError);
                }
                return;
            }
        }
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
                if(R278_ROSTER_READONLY)
                    sampleR278ReadOnlyRoster(mc);
                if(R300_ESP_READONLY)
                    sampleR300ReadOnlyEsp(mc);
            }
        } catch (RuntimeException | LinkageError problem) {
            try {stop();} catch (RuntimeException ignored) { }
            status="Preview stopped after player-tick JNI failure";
            if(R209_PLAYER_TICK_TEST)
                System.err.println("[SlinkyWinkyPreview] R209_PLAYER_TICK_FAIL "+problem);
            if(R224_LOCAL_STRAFE_TEST)
                System.err.println("[SlinkyWinkyPreview] R224_READONLY_STRAFE_FAIL "+problem);
            if(R278_ROSTER_READONLY)
                System.err.println("[SlinkyWinkyPreview] R278_ROSTER_READONLY_FAIL "+problem);
            if(R300_ESP_READONLY)
                System.err.println("[SlinkyWinkyPreview] R300_ESP_READONLY_FAIL "+problem);
            if(R301_NATIVE_ESP_READONLY)
                System.err.println("[SlinkyWinkyPreview] R301_NATIVE_ESP_READONLY_FAIL "+problem);
            if(R302_LOCAL_WORLD_TEST) {
                r302StartupFailed=true;
                System.err.println("[SlinkyWinkyPreview] R302_ESP_WORLD_FAIL "+problem);
            }
            if(R283_NATIVE_ROSTER_READONLY)
                System.err.println("[SlinkyWinkyPreview] R283_NATIVE_ROSTER_READONLY_FAIL "+problem);
            if(R289_ROSTER_LOCAL_WORLD_TEST) {
                r289StartupFailed=true;
                System.err.println("[SlinkyWinkyPreview] R289_ROSTER_WORLD_FAIL "+problem);
            }
            if(R305_SOURCE_PHASES) {
                r305Failed=true;
                System.err.println("[SlinkyWinkyPreview] R305_SOURCE_DISPATCH_FAIL "+problem);
            }
        }
    }

    // R300: source-order raw observations only, no GL, Java action or JNI.
    // Forge PlayerTick.END is diagnostic cadence, not original ESP event 18/0.
    private void sampleR300ReadOnlyEsp(Minecraft mc) {
        if(r300EspSampler==null||mc.theWorld==null||mc.thePlayer==null||
           !mc.theWorld.isRemote||!mc.isSingleplayer())
            throw new IllegalStateException("R300 world not ready");
        final EspWorldSnapshotSampler.Receipt receipt=
            r300EspSampler.observe(new ForgeEspWorldSnapshotPort(mc),
                                   mc.thePlayer.ticksExisted);
        if(receipt.worldToken<=0||receipt.playerToken<=0||
           receipt.worldToken==receipt.playerToken||receipt.players.isEmpty())
            throw new IllegalStateException("R300 invalid ownership receipt");
        r300World=mc.theWorld;r300Owner=mc.thePlayer;
        if(R302_LOCAL_WORLD_TEST) {
            if(r302World==null){
                r302World=mc.theWorld;r302Player=mc.thePlayer;
                r302Generation=receipt.generation;
            } else if(r302World!=mc.theWorld||r302Player!=mc.thePlayer||
                      r302Generation!=receipt.generation)
                throw new IllegalStateException("R302 world changed during acceptance");
        }
        if(R301_NATIVE_ESP_READONLY) {
            if(r301EspNative==null)
                throw new IllegalStateException("R301 native session absent");
            if(R306_PLAYER_2D)r306Camera=null; // No cross-tick matrix reuse.
            EspNativeReadOnlyPreview.Result nativeReceipt=
                r301EspNative.observe(receipt,32.0f,32.0f,32.0f);
            if(R305_SOURCE_PHASES) {
                final EspNativeReadOnlyPreview.Result source0=
                    r301EspNative.sourceEvent(nativeReceipt,0);
                if(source0.chestRows<0||source0.itemRows!=-1||
                   source0.playerRows!=-1)
                    throw new IllegalStateException("R305 incorrect Chest event0");
                nativeReceipt=r301EspNative.sourceEvent(source0,18);
                if(nativeReceipt.itemRows<0||nativeReceipt.playerRows<0)
                    throw new IllegalStateException("R305 incomplete event18");
                ++r305Collections;
                if(r305Collections==30)
                    System.out.println("[SlinkyWinkyPreview] "+
                        "R305_REAL_NATIVE_SOURCE_0_18_PASS ticks="+
                        r305Collections+" gameWrites=0"+
                        " host=Forge_PlayerTick_END_not_original_dispatch");
            }
            if(nativeReceipt.worldToken!=receipt.worldToken||
               nativeReceipt.playerToken!=receipt.playerToken||
               nativeReceipt.generation!=receipt.generation||
               nativeReceipt.tick!=receipt.tick||
               nativeReceipt.rawTiles!=receipt.tiles.size()||
               nativeReceipt.rawDrops!=receipt.drops.size()||
               nativeReceipt.rawPlayers!=receipt.players.size())
                throw new IllegalStateException("R301 native provenance diverged");
            if(R303_CHEST_RENDER){
                r303LastReceipt=nativeReceipt;
                r303World=mc.theWorld;r303Player=mc.thePlayer;
            }
            ++r301EspSamples;
            if(R302_LOCAL_WORLD_TEST && r301EspSamples==32) {
                if(nativeReceipt.rawPlayers<1 ||
                   nativeReceipt.playerRows>=nativeReceipt.rawPlayers ||
                   nativeReceipt.chestRows>nativeReceipt.rawTiles ||
                   nativeReceipt.itemRows>nativeReceipt.rawDrops ||
                   nativeReceipt.generation!=r302Generation)
                    throw new IllegalStateException("R302 impossible ESP rows");
                r302ReceiptEmitted=true;
                System.out.println("[SlinkyWinkyPreview] "+
                    "R302_REAL_FORGE_ESP_NATIVE_JNI_PASS "+
                    "samples="+r301EspSamples+
                    " clientTicks="+r208WorldClientTicks+
                    " rawTiles="+nativeReceipt.rawTiles+
                    " rawDrops="+nativeReceipt.rawDrops+
                    " rawPlayers="+nativeReceipt.rawPlayers+
                    " chestRows="+nativeReceipt.chestRows+
                    " itemRows="+nativeReceipt.itemRows+
                    " playerRows="+nativeReceipt.playerRows+
                    " generation="+nativeReceipt.generation+
                    " gameWrites=0 source=Forge_PlayerTick_END_JNI");
            }
            if(r301EspSamples==20)
                System.out.println("[SlinkyWinkyPreview] "+
                    "R301_REAL_NATIVE_ESP_WORLD_RECEIPT "+
                    "samples="+r301EspSamples+
                    " chestRows="+nativeReceipt.chestRows+
                    " itemRows="+nativeReceipt.itemRows+
                    " playerRows="+nativeReceipt.playerRows+
                    " generation="+nativeReceipt.generation+
                    " writes=0 rendering=0");
        }
        ++r300EspSamples;
        if(r300EspSamples==20)
            System.out.println("[SlinkyWinkyPreview] "+
                "R300_FORGE_ESP_WORLD_SNAPSHOT_RECEIPT "+
                "snapshots="+r300EspSamples+
                " tiles="+receipt.tiles.size()+
                " drops="+receipt.drops.size()+
                " players="+receipt.players.size()+
                " generation="+receipt.generation+
                " writes=0 cadence=Forge_PlayerTick_END");
    }

    // R278 real local singleplayer roster receipt. No relationship writes.
    private void sampleR278ReadOnlyRoster(Minecraft mc) {
        if(r278RosterSampler==null || mc.theWorld==null ||
           mc.thePlayer==null || !mc.theWorld.isRemote ||
           !mc.isSingleplayer())
            throw new IllegalStateException("R278 roster not local/ready");
        final RelationshipRosterSampler.Receipt receipt=
            r278RosterSampler.observe(new ForgeRelationshipRosterPort(mc),
                                      mc.thePlayer.ticksExisted);
        if(receipt.worldToken<=0 || receipt.ownerToken<=0 ||
           receipt.worldToken==receipt.ownerToken)
            throw new IllegalStateException("R278 invalid owner receipt");
        ++r278RosterSamples;
        if(r278RosterSamples==20)
            System.out.println("[SlinkyWinkyPreview] "+
                "R278_FORGE_ROSTER_READONLY_RECEIPT "+
                "samples="+r278RosterSamples+
                " remoteCount="+receipt.remote.size()+
                " generation="+receipt.generation+
                " writes=0 source=Forge_PlayerTick_END");
        if(R283_NATIVE_ROSTER_READONLY) {
            if(r283Native==null || r283Settings==null)
                throw new IllegalStateException("R283 native session not admitted");
            final RelationshipRosterNativePreview.Result proposal=
                r283Native.observe(receipt,r283Settings);
            if(proposal.worldToken!=receipt.worldToken ||
               proposal.ownerToken!=receipt.ownerToken ||
               proposal.generation!=receipt.generation ||
               proposal.tick!=receipt.tick ||
               proposal.remote.size()!=receipt.remote.size())
                throw new IllegalStateException("R283 native provenance mismatch");
            int rejected=0;
            for(int i=0;i<proposal.remote.size();++i) {
                if(!proposal.remote.get(i).uuid.equals(receipt.remote.get(i).uuid))
                    throw new IllegalStateException("R283 native UUID mismatch");
                if(proposal.remote.get(i).rejected)++rejected;
            }
            ++r283NativeSamples;
            if(r283NativeSamples==20)
                System.out.println("[SlinkyWinkyPreview] "+
                    "R283_FORGE_NATIVE_ANTIBOT_READONLY_RECEIPT "+
                    "samples="+r283NativeSamples+
                    " remoteCount="+proposal.remote.size()+
                    " rejectedCount="+rejected+
                    " generation="+proposal.generation+
                    " proposalsOnly=1 gameWrites=0"+
                    " source=Forge_PlayerTick_END_JNI");
            if(R289_ROSTER_LOCAL_WORLD_TEST && r283NativeSamples==32) {
                r289ReceiptEmitted=true;
                System.out.println("[SlinkyWinkyPreview] "+
                    "R289_REAL_WORLD_ANTIBOT_NATIVE_JNI_PASS "+
                    "samples="+r283NativeSamples+
                    " clientTicks="+r208WorldClientTicks+
                    " remoteCount="+proposal.remote.size()+
                    " generation="+proposal.generation+
                    " gameWrites=0 source=Forge_PlayerTick_END_JNI");
            }
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
     * R237/R239/R241 real-world source-ordered read-only Clutch preview.
     * R245 explicitly opts into native prior ray preflight and a real
     * Minecraft block ray on the same client thread. Never rotates,
     * places blocks, presses keys, sends packets or modifies the world.
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
            // R246: never aggregate live-ray evidence across world/player
            // owners; each completed 20-tick certificate is one identity.
            r237Samples=r237NonAir=r239NativePlans=r241AimReceipts=0;
            r245Raycasts=r245Retained=r245PriorRequests=0;
            r251PriorSamples=r251HitBlocks=r251Matches=0;
            r252Due=r252PressProposals=0;
            if(r251Sampler!=null)r251Sampler.close();
            if(r252Cadence!=null)r252Cadence.close();
            r252Cadence=R252_CLUTCH_CADENCE_READONLY?
                new ClutchCadenceObserver():null;
            r251Sampler=R251_CLUTCH_FACE_READONLY?new ClutchFaceSampler():null;
            r251PriorCentered=false;
            r237Sampler=new ClutchWorldSampler();
            r239Native=new ClutchNativePreview();
            r237World=mc.theWorld;r237Player=mc.thePlayer;
            ++r237Generation;r237Tick=0;
            r245Previous=null;r245PreviousAim=null;
        }
        final net.minecraft.util.AxisAlignedBB box=
            mc.thePlayer.getEntityBoundingBox();
        final boolean hasPrior=R245_CLUTCH_PRIOR_READONLY &&
            r245Previous!=null && r245PreviousAim!=null;
        final ClutchWorldSampler.Request request=new ClutchWorldSampler.Request(
            r237Generation,++r237Tick,
            new double[]{box.minX,box.minY,box.minZ},
            new double[]{box.maxX,box.maxY,box.maxZ},
            new double[]{frame.eyeX,frame.eyeY,frame.eyeZ},
            7,0.0f,hasPrior?r245Previous:null);
        final ForgeClutchWorldPort worldPort=new ForgeClutchWorldPort(mc);
        // R251 samples only an actual previous native candidate, never a
        // synthesized cursor target. PlayerTick.END is observational timing.
        ClutchFaceSampler.Receipt cursor=null;
        if(R251_CLUTCH_FACE_READONLY && hasPrior) {
            if(r251Sampler==null)throw new IllegalStateException(
                "Missing R251 face owner");
            cursor=r251Sampler.capture(
                worldPort,new ClutchFaceSampler.Request(
                    r237Generation,r237Tick,
                    new ClutchFaceSampler.Pos(r245Previous.x,r245Previous.y,
                        r245Previous.z),r251PriorCentered));
            ++r251PriorSamples;
            if(cursor.hitBlock)++r251HitBlocks;
            if(cursor.matchesPlannedFace())++r251Matches;
        }
        if(R252_CLUTCH_CADENCE_READONLY) {
            if(r252Cadence==null)throw new IllegalStateException(
                "R252 cadence owner unavailable");
            final ClutchCadenceObserver.Step proposal=r252Cadence.advance(
                worldPort,r237Generation,r237Tick,10.0f,
                mc.inGameHasFocus && mc.currentScreen==null,
                hasPrior?new ClutchFaceSampler.Pos(
                    r245Previous.x,r245Previous.y,r245Previous.z):null,
                r251PriorCentered,cursor);
            if(proposal.timerDue)++r252Due;
            if(proposal.wouldPress)++r252PressProposals;
        }
        // Keep the real Minecraft world/raycast adapter unchanged and
        // count only source-authorized rayTrace calls for diagnostics.
        final ClutchWorldSampler.Port port;
        if(R245_CLUTCH_PRIOR_READONLY) {
            port=new ClutchWorldSampler.Port() {
                @Override public boolean onClientThread() {
                    return worldPort.onClientThread();
                }
                @Override public Object worldIdentity() {
                    return worldPort.worldIdentity();
                }
                @Override public Object playerIdentity() {
                    return worldPort.playerIdentity();
                }
                @Override public int blockId(int x,int y,int z) {
                    return worldPort.blockId(x,y,z);
                }
                @Override public ClutchWorldSampler.Pos rayTrace(
                        double[] from,double[] to) {
                    ++r245Raycasts;
                    return worldPort.rayTrace(from,to);
                }
            };
        } else port=worldPort;
        final ClutchWorldSampler.Snapshot snapshot=r237Sampler.capture(
            port,request);
        for(int id:snapshot.candidateBlockIds())if(id!=0)++r237NonAir;
        // One native planner receipt per observed Minecraft player tick.
        // This never acts on the returned candidate or rotation.
        final double clock=System.nanoTime()*1e-9;
        final ClutchNativePreview.Decision plan;
        if(hasPrior) {
            ++r245PriorRequests;
            plan=r239Native.observePrior(r237Sampler,port,request,snapshot,
                r245PreviousAim,frame.yaw,frame.pitch,300.0f,1.0f,clock);
        } else {
            plan=r239Native.observe(request,snapshot,frame.yaw,frame.pitch,
                300.0f,1.0f,clock);
        }
        if(R245_CLUTCH_PRIOR_READONLY) {
            if(plan.preservedPrior)++r245Retained;
            r245Previous=plan.found?
                new ClutchWorldSampler.Pos(
                    plan.blockX,plan.blockY,plan.blockZ):null;
            r245PreviousAim=plan.found?
                new double[]{plan.aimX,plan.aimY,plan.aimZ}:null;
            r251PriorCentered=plan.found && plan.faceUp;
        }
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
        if(r237Samples==20) {
            // R246: a full candidate scan without any real prior ray is
            // INCONCLUSIVE for R245, not a false-positive ray PASS.
            final String marker=R245_CLUTCH_PRIOR_READONLY ?
                ClutchRayReceiptStatus.classify(
                    r237Samples,r239NativePlans,r241AimReceipts,
                    r245PriorRequests,r245Raycasts,r245Retained) :
                "R237_REAL_WORLD_CLUTCH_167_READONLY_PASS";
            if(R252_CLUTCH_CADENCE_READONLY) {
                if(r252PressProposals>r252Due || r252Due>r237Samples)
                    throw new IllegalStateException("R252 invalid proposal counts");
                System.out.println("[SlinkyWinkyPreview] " +
                    (r252PressProposals>0?
                        "R252_CLUTCH_WOULD_PRESS_READONLY_NOT_PLACEMENT" :
                        "R252_CLUTCH_CADENCE_INCONCLUSIVE_NO_PRESS")+
                    " ticks="+r237Samples+" timerDue="+r252Due+
                    " pressProposals="+r252PressProposals+
                    " cpsFixture=10 actions=0 source=Forge_PlayerTick_END");
            }
            if(R251_CLUTCH_FACE_READONLY)
                System.out.println("[SlinkyWinkyPreview] " +
                    ClutchFaceReceiptStatus.classify(r237Samples,
                        r251PriorSamples,r251HitBlocks,r251Matches)+
                    " cursorReads="+r251PriorSamples+
                    " hitBlocks="+r251HitBlocks+
                    " targetFaceMatches="+r251Matches+
                    " actions=0 source=Forge_PlayerTick_END_objectMouseOver");
            System.out.println("[SlinkyWinkyPreview] " + marker +
                " samples=20" +
                " candidateReads=3340 nonAir="+r237NonAir+
                " nativePlans="+r239NativePlans+
                " aimReceipts="+r241AimReceipts+
                (R245_CLUTCH_PRIOR_READONLY?
                    " gameplayWrites=0 raycasts="+r245Raycasts+
                    " priorRequests="+r245PriorRequests+
                    " retainedPrior="+r245Retained+
                    " source=Forge_PlayerTick_END_JNI_NATIVE_R244" :
                    " gameplayWrites=0 raycasts=0 source=Forge_PlayerTick_END" +
                    "_JNI_NATIVE_R235"));
        }
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

    /**
     * R303 prototype: real Forge 1.8.9 RenderWorldLast, not an authenticated
     * original Slinky event-6 callback. Only consumes the R297 native C++
     * center translations; Java only submits debug line geometry. No
     * entity/world/input/packet writes and no vanilla injector dependency.
     */
    /**
     * R341 opt-in original source event6 geometry through REAL JNI,
     * rendered only in a local client world on Forge RenderWorldLastEvent.
     * Positions are live target coordinates, not historical held positions.
     */
    @SubscribeEvent public void onR341SourceWorldRender(RenderWorldLastEvent event) {
        if(!R341_SOURCE_BOX_DIAGNOSTIC||!R337_SOURCE_READONLY||
           !enabled||r337SourceHost==null)return;
        final Minecraft mc=Minecraft.getMinecraft();
        if(!mc.isCallingFromMinecraftThread()||mc.theWorld==null||
           mc.thePlayer==null||!mc.theWorld.isRemote||!mc.isSingleplayer())
            return;
        try {
            final SourceRuntimePreview.SourceBoxes geometry=
                r337SourceHost.observeBoxes(
                    new ForgeSourceWorldPort(mc,true),event.partialTicks);
            if(geometry==null)return;
            ++r341BoxFrames;
            if(mc.theWorld==null||mc.thePlayer==null||!mc.isSingleplayer())
                throw new IllegalStateException("R341 source world replaced before GL");
            ForgeSourceBoxDiagnosticRenderer.draw(geometry);
            if(!r341SubmittedBox &&
               (geometry.backtrack.draw||geometry.lagRange.draw)) {
                r341SubmittedBox=true;
                System.out.println("[SlinkyWinkyPreview] "+
                    "R341_REAL_JNI_SOURCE_EVENT6_GL_SUBMITTED "+
                    "generation="+geometry.generation+
                    " source=current_target_diagnostic"+
                    " remote_ghost_parity_unverified=1");
            }
        }catch(RuntimeException | LinkageError invalid) {
            System.err.println("[SlinkyWinkyPreview] R341_SOURCE_E6_RENDER_FAIL "+invalid);
            try{stop();}catch(RuntimeException cleanup){
                System.err.println("[SlinkyWinkyPreview] R341_SOURCE_E6_CLOSE_FAIL "+cleanup);
            }
        }
    }

    @SubscribeEvent public void onR303ChestWorldRender(RenderWorldLastEvent event) {
        if(!R303_CHEST_RENDER||!enabled||r301EspNative==null||
           r303LastReceipt==null)return;
        final Minecraft mc=Minecraft.getMinecraft();
        if(!mc.isCallingFromMinecraftThread()||mc.theWorld==null||
           mc.thePlayer==null||mc.theWorld!=r303World||
           mc.thePlayer!=r303Player||!mc.theWorld.isRemote||
           !mc.isSingleplayer())return;
        try {
            final net.minecraft.client.renderer.entity.RenderManager rm=mc.getRenderManager();
            if(rm==null)throw new IllegalStateException("R303 camera unavailable");
            if(R305_SOURCE_PHASES) {
                r303LastReceipt=r301EspNative.sourceEvent(r303LastReceipt,6);
                ++r305WorldFrames;
                r305WorldPhaseTick=r303LastReceipt.tick;
                r305WorldPhaseGeneration=r303LastReceipt.generation;
                if(R306_PLAYER_2D)
                    r306Camera=ForgePlayerEspScreenRenderer.capture(
                        mc,r303LastReceipt,rm,event.partialTicks);
                if(r305WorldFrames==30)
                    System.out.println("[SlinkyWinkyPreview] "+
                        "R305_REAL_NATIVE_SOURCE_EVENT6_PASS frames="+
                        r305WorldFrames+" gameWrites=0");
            }
            EspNativeReadOnlyPreview.ChestPlan plan=r301EspNative.chestPlan(
                r303LastReceipt,rm.viewerPosX,rm.viewerPosY,rm.viewerPosZ);
            if(mc.theWorld!=r303World||mc.thePlayer!=r303Player)
                throw new IllegalStateException("R303 switched game world during render");
            ForgeChestEspDebugRenderer.draw(plan);
            if(R304_ENTITY_RENDER) {
                final EspNativeReadOnlyPreview.EntityPlan entities=
                    r301EspNative.entityPlan(r303LastReceipt,
                        rm.viewerPosX,rm.viewerPosY,rm.viewerPosZ,event.partialTicks);
                if(mc.theWorld!=r303World||mc.thePlayer!=r303Player)
                    throw new IllegalStateException("R304 owner changed in render");
                ForgeEntityEspDebugRenderer.draw(entities,!R306_PLAYER_2D);
                ++r304RenderFrames;
                if(!r304SawItem) {
                    for(EspNativeReadOnlyPreview.ItemDraw item:entities.items)
                        if(item.quantity>0) {
                            r304SawItem=true;
                            System.out.println("[SlinkyWinkyPreview] "+
                                "R304_ITEM_NATIVE_RENDER_SUBMITTED rows="+
                                entities.items.size()+" quantity="+item.quantity+
                                " writes=0 diagnostic=1");
                            break;
                        }
                }
                if(!entities.players.isEmpty()&&!r304SawPlayer) {
                    r304SawPlayer=true;
                    System.out.println("[SlinkyWinkyPreview] "+
                        "R304_PLAYER_NATIVE_RENDER_SUBMITTED rows="+
                        entities.players.size()+" writes=0 diagnostic=1");
                }
                if(r304RenderFrames==40)
                    System.out.println("[SlinkyWinkyPreview] "+
                        "R304_ENTITY_RENDER_CALLBACK_PASS frames="+r304RenderFrames+
                        " itemGeometry="+(r304SawItem?1:0)+
                        " playerGeometry="+(r304SawPlayer?1:0)+
                        " gameWrites=0");
            }
            ++r303RenderFrames;
            if(!plan.draws.isEmpty()&&!r303SawDraw){
                r303SawDraw=true;
                System.out.println("[SlinkyWinkyPreview] "+
                    "R303_CHEST_LIVE_DRAW_SUBMITTED rows="+plan.draws.size()+
                    " generation="+plan.generation+
                    " source=real_native_R297_render_world_last"+
                    " writes=0 visual_parity_unverified=1");
            }
            if(r303RenderFrames==40)
                System.out.println("[SlinkyWinkyPreview] "+
                    "R303_CHEST_RENDER_CALLBACK_PASS frames="+r303RenderFrames+
                    " latestRows="+plan.draws.size()+
                    " hasGeometry="+(r303SawDraw?1:0)+
                    " gameWrites=0");
        }catch(RuntimeException | LinkageError problem) {
            try {stop();}catch(RuntimeException ignored) { }
            status="Native ESP renderer refused stale or invalid frame";
            if(R305_SOURCE_PHASES) {
                r305Failed=true;
                System.err.println("[SlinkyWinkyPreview] R305_SOURCE_RENDER_FAIL "+problem);
            } else if(R304_ENTITY_RENDER) {
                r304StartupFailed=true;
                System.err.println("[SlinkyWinkyPreview] R304_ENTITY_RENDER_FAIL "+problem);
            } else
                System.err.println("[SlinkyWinkyPreview] R303_CHEST_RENDER_FAIL "+problem);
        }
    }

    @SubscribeEvent public void onOverlay(RenderGameOverlayEvent.Text event) {
        Minecraft mc=Minecraft.getMinecraft();
        if(R305_SOURCE_PHASES&&enabled&&r301EspNative!=null&&
           r303LastReceipt!=null&&mc.theWorld==r303World&&
           mc.thePlayer==r303Player&&mc.isSingleplayer()&&
           r305WorldFrames>0&&
           r303LastReceipt.tick==r305WorldPhaseTick&&
           r303LastReceipt.generation==r305WorldPhaseGeneration) {
            try {
                r303LastReceipt=r301EspNative.sourceEvent(r303LastReceipt,7);
                if(R306_PLAYER_2D){
                    if(r306Camera==null)
                        throw new IllegalStateException("R306 no matching real world camera");
                    final EspNativeReadOnlyPreview.ScreenPlan source=
                        r301EspNative.playerScreenPlan(r303LastReceipt,
                            r306Camera.viewerX,r306Camera.viewerY,
                            r306Camera.viewerZ,r306Camera.partialTicks);
                    final java.util.List<dev.slinkywinky.PlayerEspScreenProjection.Rect> rects=
                        R307_SOURCE_SCREEN ?
                            r306Camera.camera.projectSource(source) :
                            r306Camera.camera.project(source);
                    if(R307_SOURCE_SCREEN)
                        ForgePlayerEspScreenRenderer.drawSource(
                            rects,(float)r306Camera.partialTicks);
                    else
                        ForgePlayerEspScreenRenderer.draw(rects);
                    ++r306ScreenFrames;
                    if(!rects.isEmpty()&&!r306SawVisible) {
                        r306SawVisible=true;
                        System.out.println("[SlinkyWinkyPreview] "+
                            (R307_SOURCE_SCREEN ?
                             "R307_SOURCE_PLAYER_BOX_SUBMITTED rectangles=" :
                             "R306_REAL_NATIVE_2D_PLAYER_SUBMITTED rectangles=")+
                            rects.size()+" sourceRows="+source.players.size()+
                            " projection=forge_world_matrix gameWrites=0"+
                            " originalPixelsVerified=0");
                    }
                    if(r306ScreenFrames==40)
                        System.out.println("[SlinkyWinkyPreview] "+
                            (R307_SOURCE_SCREEN ?
                             "R307_SOURCE_SCREEN_CALLBACK_PASS frames=" :
                             "R306_PLAYER_SCREEN_CALLBACK_PASS frames=")+r306ScreenFrames+
                            " projectedOnce="+(r306SawVisible?1:0)+
                            " gameWrites=0");
                }
                ++r305ScreenFrames;
                if(r305ScreenFrames==30)
                    System.out.println("[SlinkyWinkyPreview] "+
                        "R305_REAL_NATIVE_SOURCE_EVENT7_PASS overlays="+
                        r305ScreenFrames+
                        " player2DVisualParity=unverified gameWrites=0");
            } catch(RuntimeException | LinkageError problem) {
                r305Failed=true;
                if(R306_PLAYER_2D)r306Failed=true;
                try{stop();}catch(RuntimeException ignored){}
                System.err.println("[SlinkyWinkyPreview] "+
                    (R306_PLAYER_2D?"R306_PLAYER_SCREEN_FAIL ":"R305_SOURCE_EVENT7_FAIL ")+problem);
            }
        }
        if(mc.fontRendererObj!=null) mc.fontRendererObj.drawStringWithShadow(
            "SlinkyWinky: "+status,4,4,0xFFDDDDDD);
    }
}
