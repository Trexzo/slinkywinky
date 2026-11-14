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
    private R213NativeSprintBridge nativeSprint;
    private R214NativeMenuOverlay nativeGl11;

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
                if(Boolean.getBoolean("slinkywinky.r213NativeSprintBridge")) {
                    nativeSprint=new R213NativeSprintBridge();
                    R213NativeSprintBridge.Ack initial=nativeSprint.snapshot();
                    if(initial.requested || initial.active || initial.id!=0)
                        throw new IllegalStateException("R213 initial native state");
                    System.out.println("R213_REAL_NATIVE_SPRINT_INITIAL_OFF_PASS");
                    if(Boolean.getBoolean("slinkywinky.r214NativeGuiOverlay")) {
                        nativeGl11=new R214NativeMenuOverlay(initial);
                        R214NativeMenuOverlay.View v=nativeGl11.view();
                        if(v.sourceSequence!=1 || v.requested || v.active ||
                           v.activeRows!=0)
                            throw new IllegalStateException("R214 source initial mismatch");
                        System.out.println("R214_NATIVE_GL11_SOURCE_INITIAL_OFF_PASS");
                    }
                }
                screen=new ProbeScreen(this,savedScreen);
                mc.displayGuiScreen(screen);
                if(mc.currentScreen!=screen)
                    throw new IllegalStateException("R212 GuiScreen not installed");
                System.out.println("R212_REAL_FORGE_GUI_SCREEN_INSTALLED");
                stage=1;
            } else if(stage==1 && ticks>=12) {
                if(nativeGl11!=null && nativeGl11.paintedFrames()==0)
                    throw new IllegalStateException("R214 GL11 menu never painted");
                if(mc.currentScreen!=screen || screen.actionCount()!=0)
                    throw new IllegalStateException("R212 synthetic click precondition");
                screen.disposableInvokeVanillaMouseClicked();
                if(lastRequestId!=1L || !requestedSprint ||
                   screen.actionCount()!=1 ||
                   mc.currentScreen!=screen)
                    throw new IllegalStateException("R212 vanilla mouse dispatch");
                System.out.println("R212_REAL_FORGE_GUI_BUTTON_ACTION_REQUESTED");
                stage=2;
            } else if(stage==2 && nativeSprint!=null && ticks>=14) {
                screen.disposableInvokeVanillaMouseClicked();
                if(lastRequestId!=2 || requestedSprint ||
                   screen.actionCount()!=2 || !screen.showsNativeAck(2,false,false))
                    throw new IllegalStateException("R213 native disable/GUI state");
                System.out.println("R213_REAL_JNI_SPRINT_DISABLE_AND_RENDER_RECEIPT_PASS");
                stage=3;
            } else if(stage==3 && nativeSprint!=null && ticks>=16) {
                screen.disposableInvokeVanillaMouseClicked();
                if(lastRequestId!=3 || !requestedSprint ||
                   screen.actionCount()!=3 || !screen.showsNativeAck(3,true,false))
                    throw new IllegalStateException("R213 native reenable/GUI state");
                System.out.println("R213_REAL_JNI_SPRINT_REENABLE_AND_RENDER_RECEIPT_PASS");
                stage=4;
            } else if((stage==2 && nativeSprint==null && ticks>=16) ||
                      (stage==4 && nativeSprint!=null && ticks>=20)) {
                // ESC is sent to the OWN screen's real vanilla key handler.
                // This is a scripted in-process invocation, NOT physical input.
                screen.disposableInvokeVanillaEscape();
                if(mc.currentScreen!=savedScreen || screen.actionCount()!=
                   (nativeSprint==null?1:3))
                    throw new IllegalStateException("R212 GUI restore failed");
                if(nativeSprint!=null) {
                    R213NativeSprintBridge.Ack ack=nativeSprint.snapshot();
                    if(ack.id!=3 || !ack.requested || ack.active)
                        throw new IllegalStateException("R213 native final state");
                    if(nativeGl11!=null) {
                        R214NativeMenuOverlay.View v=nativeGl11.view();
                        if(v.sourceSequence!=4 || !v.requested || v.active ||
                           v.activeRows!=0 || v.drawCount<1)
                            throw new IllegalStateException("R214 final native source mismatch");
                        nativeGl11.close();
                        nativeGl11=null;
                        System.out.println("R214_NATIVE_GL11_GUI_AND_FEED_CLOSED_PASS");
                    }
                    nativeSprint.close();
                    nativeSprint=null;
                    System.out.println("R213_REAL_JNI_SPRINT_OWNER_CLOSE_PASS");
                }
                screen=null;
                System.out.println("R212_REAL_FORGE_GUI_ESC_RESTORES_PREVIOUS_SCREEN");
                stage=5;
                System.out.println(
                    "R212_FORGE189_OWN_GUIScreen_MOUSE_AND_ESCAPE_PASS");
            } else if(stage==5 && ticks>=24) {
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
        final long nextId=lastRequestId+1;
        final boolean target=!requestedSprint;
        if(nativeSprint!=null) {
            R214NativeMenuOverlay.Request exact=null;
            if(nativeGl11!=null) {
                final Minecraft mc=Minecraft.getMinecraft();
                exact=nativeGl11.request(mc.displayWidth,mc.displayHeight);
                if(exact.id!=nextId || exact.enable!=target ||
                   exact.observedSequence!=nextId)
                    throw new IllegalStateException("R214 painted source request mismatch");
            }
            R213NativeSprintBridge.Ack ack=nativeSprint.request(
                nextId,"Sprint",target);
            if(nativeGl11!=null) {
                nativeGl11.acknowledge(exact,ack);
                R214NativeMenuOverlay.View v=nativeGl11.view();
                if(v.sourceSequence!=nextId+1 || v.requested!=target ||
                   v.active || v.activeRows!=0 || v.drawCount<1)
                    throw new IllegalStateException("R214 native renderer source mismatch");
                System.out.println("R214_REAL_GL11_MENU_TO_JNI_TO_ARRAYLIST_FEED_PASS id="+
                    nextId+" requested="+v.requested+" active="+v.active+
                    " activeRows="+v.activeRows+" frames="+v.drawCount);
            }
            if(ack.id!=nextId || ack.requested!=target || ack.active)
                throw new IllegalStateException("R213 native acknowledgement mismatch");
            requestedSprint=ack.requested;
            lastRequestId=ack.id;
            source.showNativeAck(ack);
            System.out.println("R213_REAL_JNI_NATIVE_SPRINT_ACK id="+
                ack.id+" requested="+ack.requested+" active="+ack.active+
                " generation="+ack.generation);
        } else {
            requestedSprint=target;
            lastRequestId=nextId;
            // The legacy R212 acceptance stays request-only.
            System.out.println("[R212GuiProof] request-only canonical=Sprint id="+
                               lastRequestId+" requested="+requestedSprint+
                               " active=false");
        }
    }

    private static final class ProbeScreen extends GuiScreen {
        private final R212ForgeGuiInputProof host;
        private final GuiScreen previous;
        private Thread owner;
        private GuiButton sprint;
        private int actions;
        private R213NativeSprintBridge.Ack nativeAck;

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
            if(host.nativeGl11!=null) {
                if(!host.nativeGl11.paint(mc.displayWidth,mc.displayHeight))
                    throw new IllegalStateException("R214 actual WGL menu paint refused");
            }
            drawCenteredString(fontRendererObj,
                "R212 Minecraft-owned GUI input proof (NO GAME ACTIONS)",
                width/2,height/2-56,0xEEEEEE);
            drawCenteredString(fontRendererObj,
                nativeAck==null ? "GUI request preview only; no active runtime state" :
                "JNI ACK #"+nativeAck.id+" | runtime active="+nativeAck.active+
                    " (title screen has no world)",
                width/2,height/2-38,0xAAAAAA);
            super.drawScreen(mouseX,mouseY,partial);
        }
        void showNativeAck(R213NativeSprintBridge.Ack ack) {
            if(!owned() || sprint==null || ack==null || ack.id!=actions ||
               !"Sprint".equals(ack.module))
                throw new IllegalStateException("R213 GUI cannot display unowned ack");
            nativeAck=ack;
            sprint.displayString="Sprint | requested "+
                (ack.requested?"ON":"OFF")+" | active "+
                (ack.active?"ON":"OFF");
        }
        boolean showsNativeAck(long id,boolean requested,boolean active) {
            return owned() && nativeAck!=null && nativeAck.id==id &&
                nativeAck.requested==requested && nativeAck.active==active &&
                sprint.displayString.equals("Sprint | requested "+
                    (requested?"ON":"OFF")+" | active "+
                    (active?"ON":"OFF"));
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
