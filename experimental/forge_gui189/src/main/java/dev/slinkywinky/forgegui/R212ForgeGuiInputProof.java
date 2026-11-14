package dev.slinkywinky.forgegui;

import java.io.File;
import java.io.IOException;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.GuiButton;
import net.minecraft.client.gui.GuiMainMenu;
import net.minecraft.client.gui.GuiScreen;
import net.minecraftforge.fml.common.FMLCommonHandler;
import net.minecraftforge.fml.common.Mod;
import net.minecraftforge.fml.common.event.FMLInitializationEvent;
import net.minecraftforge.fml.common.eventhandler.SubscribeEvent;
import net.minecraftforge.fml.common.gameevent.TickEvent;
import org.lwjgl.input.Keyboard;

/**
 * R212: real Minecraft 1.8.9's OWN GuiScreen dispatcher, as an independently
 * opt-in, TEMP-only proof. No LWJGL interception, no native module action,
 * no SlinkyWinky injector, no player/world/packet setters. This is an
 * input-ownership alternative to unsafe Mouse.next/Keyboard.next rewriting.
 */
@Mod(modid="slinkyguiinputproof",name="Slinky GUI Input Proof",
     version="0.1.0",clientSideOnly=true)
public final class R212ForgeGuiInputProof {
    private static final String OPT_IN="slinkywinky.r212GuiInputProof";
    private static final String AUTO_TEST="slinkywinky.r212GuiAutoProof";
    private static final String MANUAL_TEST="slinkywinky.r212GuiManualProof";
    private Thread clientOwner;
    private ProbeScreen screen;
    private int ticks,stage;
    private GuiScreen savedScreen;
    private boolean finished;
    private long lastRequestId;
    private boolean requestedSprint;

    @Mod.EventHandler public void initialize(FMLInitializationEvent unused) {
        FMLCommonHandler.instance().bus().register(this);
        System.out.println("[R212GuiProof] loaded, dormant unless TEMP-only opt-in");
    }
    private static boolean admitted(Minecraft mc) {
        if(!Boolean.getBoolean(OPT_IN) || mc==null ||
           mc.getSession()==null ||
           !"R212GuiOffline".equals(mc.getSession().getUsername()))
            return false;
        try {
            File game=mc.mcDataDir.getCanonicalFile();
            File tmp=new File(System.getProperty("java.io.tmpdir")).getCanonicalFile();
            String path=game.getPath().replace('\\','/');
            String temp=tmp.getPath().replace('\\','/');
            return path.startsWith(temp+"/slinkywinky-r212-gui-") &&
                   !path.equals(temp) && mc.theWorld==null &&
                   mc.thePlayer==null;
        } catch(IOException badPath) {
            return false;
        }
    }
    @SubscribeEvent public void clientTick(TickEvent.ClientTickEvent event) {
        if(event.phase!=TickEvent.Phase.END || finished)return;
        final Minecraft mc=Minecraft.getMinecraft();
        if(!admitted(mc))return;
        if(clientOwner==null)clientOwner=Thread.currentThread();
        if(clientOwner!=Thread.currentThread())
            throw new IllegalStateException("R212 wrong Minecraft GUI owner");
        ++ticks;
        final boolean automatic=Boolean.getBoolean(AUTO_TEST);
        if(automatic) {
            if(stage==0 && ticks>=8 && mc.currentScreen instanceof GuiMainMenu) {
                savedScreen=mc.currentScreen;
                screen=new ProbeScreen(this,savedScreen);
                mc.displayGuiScreen(screen);
                if(mc.currentScreen!=screen)
                    throw new IllegalStateException("R212 GuiScreen not installed");
                System.out.println("R212_REAL_FORGE_GUI_SCREEN_INSTALLED");
                stage=1;
            } else if(stage==1 && ticks>=12) {
                if(mc.currentScreen!=screen || screen.actionCount()!=0)
                    throw new IllegalStateException("R212 synthetic click precondition");
                screen.disposableInvokeVanillaMouseClicked();
                if(lastRequestId!=1L || !requestedSprint ||
                   screen.actionCount()!=1 ||
                   mc.currentScreen!=screen)
                    throw new IllegalStateException("R212 vanilla mouse dispatch");
                System.out.println("R212_REAL_FORGE_GUI_BUTTON_ACTION_REQUESTED");
                stage=2;
            } else if(stage==2 && ticks>=16) {
                // ESC is sent to the OWN screen's real vanilla key handler.
                // This is a scripted in-process invocation, NOT physical input.
                screen.disposableInvokeVanillaEscape();
                if(mc.currentScreen!=savedScreen || screen.actionCount()!=1)
                    throw new IllegalStateException("R212 GUI restore failed");
                screen=null;
                System.out.println("R212_REAL_FORGE_GUI_ESC_RESTORES_PREVIOUS_SCREEN");
                stage=3;
                System.out.println(
                    "R212_FORGE189_OWN_GUIScreen_MOUSE_AND_ESCAPE_PASS");
            } else if(stage==3 && ticks>=20) {
                // Stop ONLY the isolated test client after the vanilla
                // previous screen is restored. No external PID kill or
                // other running Minecraft instance is touched.
                finished=true;
                System.out.println("R212_ISOLATED_CLIENT_CLEAN_SHUTDOWN");
                FMLCommonHandler.instance().exitJava(0,false);
            }
        } else if(Boolean.getBoolean(MANUAL_TEST)) {
            // An optional interactive test is opened ONLY on the disposable
            // title screen; all physical clicks/keys remain Minecraft-owned.
            if(stage==0 && ticks>=8 && mc.currentScreen instanceof GuiMainMenu) {
                savedScreen=mc.currentScreen;
                screen=new ProbeScreen(this,savedScreen);
                mc.displayGuiScreen(screen);
                if(mc.currentScreen!=screen)
                    throw new IllegalStateException("R212 manual GUI not installed");
                stage=1;
                System.out.println("R212_MANUAL_OWNED_GUI_READY");
            } else if(stage==1 && mc.currentScreen!=screen) {
                screen=null;
                finished=true;
                System.out.println("R212_MANUAL_GUI_CLOSED_VANILLA_RESTORED");
            }
        }
    }

    private void requestedFromOwnedScreen(ProbeScreen source) {
        if(Thread.currentThread()!=clientOwner ||
           source==null || source!=screen ||
           Minecraft.getMinecraft().currentScreen!=source)
            throw new IllegalStateException("R212 unowned GUI request");
        requestedSprint=!requestedSprint;
        ++lastRequestId;
        // Intentionally NOT a native ModuleControlState acknowledgement.
        // Rendering an active HUD row here would misrepresent runtime state.
        System.out.println("[R212GuiProof] request-only canonical=Sprint id="+
                           lastRequestId+" requested="+requestedSprint+
                           " active=false");
    }

    private static final class ProbeScreen extends GuiScreen {
        private final R212ForgeGuiInputProof host;
        private final GuiScreen previous;
        private Thread owner;
        private GuiButton sprint;
        private int actions;

        ProbeScreen(R212ForgeGuiInputProof host,GuiScreen previous) {
            this.host=host;
            this.previous=previous;
        }
        @Override public void initGui() {
            owner=Thread.currentThread();
            buttonList.clear();
            sprint=new GuiButton(212, width/2-110, height/2-18,
                                 220,24,"Sprint | request ONLY");
            buttonList.add(sprint);
            buttonList.add(new GuiButton(213,width/2-110,
                                 height/2+16,220,24,"Close / restore vanilla"));
        }
        private boolean owned() {
            return owner==Thread.currentThread() &&
                   mc!=null && mc.currentScreen==this;
        }
        @Override protected void actionPerformed(GuiButton button)
                throws IOException {
            if(!owned())throw new IllegalStateException("R212 lost screen owner");
            if(button==sprint && button.id==212) {
                actions++;
                host.requestedFromOwnedScreen(this);
            } else if(button!=null && button.id==213)
                mc.displayGuiScreen(previous);
        }
        @Override protected void keyTyped(char ch,int code)
                throws IOException {
            if(!owned())throw new IllegalStateException("R212 lost keyboard owner");
            if(code==Keyboard.KEY_ESCAPE)mc.displayGuiScreen(previous);
            else super.keyTyped(ch,code);
        }
        @Override public void drawScreen(int mouseX,int mouseY,float partial) {
            drawDefaultBackground();
            drawCenteredString(fontRendererObj,
                "R212 Minecraft-owned GUI input proof (NO GAME ACTIONS)",
                width/2,height/2-56,0xEEEEEE);
            drawCenteredString(fontRendererObj,
                "Requested Sprint is NOT active or acknowledged",
                width/2,height/2-38,0xAAAAAA);
            super.drawScreen(mouseX,mouseY,partial);
        }
        @Override public boolean doesGuiPauseGame() {return true;}
        int actionCount(){return actions;}
        void disposableInvokeVanillaMouseClicked() {
            if(!owned() || sprint==null)
                throw new IllegalStateException("R212 missing current GUI");
            try {
                mouseClicked(sprint.xPosition+sprint.width/2,
                             sprint.yPosition+sprint.height/2,0);
            } catch(IOException e) {throw new IllegalStateException(e);}
        }
        void disposableInvokeVanillaEscape() {
            if(!owned())throw new IllegalStateException("R212 missing GUI");
            try {keyTyped((char)0,Keyboard.KEY_ESCAPE);}
            catch(IOException e) {throw new IllegalStateException(e);}
        }
    }
}
