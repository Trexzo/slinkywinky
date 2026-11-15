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
import org.lwjgl.opengl.Display;

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
    private static boolean sourceMouseMode() {
        return Boolean.getBoolean("slinkywinky.r215SourceMouseAuto") ||
               Boolean.getBoolean("slinkywinky.r215SourceMouseManual");
    }

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
                assertAuthoritativeSwitchVisual(1,false);
                if(mc.currentScreen!=screen || screen.actionCount()!=0)
                    throw new IllegalStateException("R212 synthetic click precondition");
                if(Boolean.getBoolean("slinkywinky.r215SourceMouseAuto"))
                    screen.disposableInvokePaintedNativeToggle();
                else screen.disposableInvokeVanillaMouseClicked();
                if(lastRequestId!=1L || !requestedSprint ||
                   screen.actionCount()!=1 ||
                   mc.currentScreen!=screen)
                    throw new IllegalStateException("R212 vanilla mouse dispatch");
                System.out.println("R212_REAL_FORGE_GUI_BUTTON_ACTION_REQUESTED");
                stage=2;
            } else if(stage==2 && nativeSprint!=null && ticks>=
                      (Boolean.getBoolean("slinkywinky.r215SourceMouseAuto") ? 30 : 14)) {
                assertAuthoritativeSwitchVisual(2,true);
                if(Boolean.getBoolean("slinkywinky.r215SourceMouseAuto"))
                    screen.disposableInvokePaintedNativeToggle();
                else screen.disposableInvokeVanillaMouseClicked();
                if(lastRequestId!=2 || requestedSprint ||
                   screen.actionCount()!=2 || !screen.showsNativeAck(2,false,false))
                    throw new IllegalStateException("R213 native disable/GUI state");
                System.out.println("R213_REAL_JNI_SPRINT_DISABLE_AND_RENDER_RECEIPT_PASS");
                stage=3;
            } else if(stage==3 && nativeSprint!=null && ticks>=
                      (Boolean.getBoolean("slinkywinky.r215SourceMouseAuto") ? 50 : 16)) {
                assertAuthoritativeSwitchVisual(3,false);
                if(Boolean.getBoolean("slinkywinky.r215SourceMouseAuto"))
                    screen.disposableInvokePaintedNativeToggle();
                else screen.disposableInvokeVanillaMouseClicked();
                if(lastRequestId!=3 || !requestedSprint ||
                   screen.actionCount()!=3 || !screen.showsNativeAck(3,true,false))
                    throw new IllegalStateException("R213 native reenable/GUI state");
                System.out.println("R213_REAL_JNI_SPRINT_REENABLE_AND_RENDER_RECEIPT_PASS");
                stage=4;
            } else if((stage==2 && nativeSprint==null && ticks>=16) ||
                      (stage==4 && nativeSprint!=null && ticks>=
                          (Boolean.getBoolean("slinkywinky.r215SourceMouseAuto") ? 70 : 20))) {
                assertAuthoritativeSwitchVisual(4,true);
                // R218: a held source click must cancel on vanilla Escape
                // without dispatch, even though JNI and preview are active.
                final boolean assertHeldClose=
                    Boolean.getBoolean("slinkywinky.r215SourceMouseAuto");
                if(assertHeldClose)screen.disposableHoldSourceBeforeEscape();
                // ESC is sent to the OWN screen's real vanilla key handler.
                // This is a scripted in-process invocation, NOT physical input.
                screen.disposableInvokeVanillaEscape();
                if(assertHeldClose) {
                    if(!screen.heldCanceledOnClose() || lastRequestId!=3)
                        throw new IllegalStateException("R218 held Escape leaked request");
                    System.out.println("R218_HELD_CLICK_ESCAPE_CLOSE_CANCELED_PASS");
                }
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
            } else if(stage==5 && ticks>=
                (Boolean.getBoolean("slinkywinky.r215SourceMouseAuto") ? 74 : 24)) {
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
                if(Boolean.getBoolean("slinkywinky.r215SourceMouseManual")) {
                    nativeSprint=new R213NativeSprintBridge();
                    R213NativeSprintBridge.Ack initial=nativeSprint.snapshot();
                    if(initial.requested || initial.active)
                        throw new IllegalStateException("R215 manual initial owner state");
                    nativeGl11=new R214NativeMenuOverlay(initial);
                    System.out.println("R215_MANUAL_GL11_SOURCE_MOUSE_READY");
                }
                screen=new ProbeScreen(this,savedScreen);
                mc.displayGuiScreen(screen);
                if(mc.currentScreen!=screen)
                    throw new IllegalStateException("R212 manual GUI not installed");
                stage=1;
                System.out.println("R212_MANUAL_OWNED_GUI_READY");
            } else if(stage==1 && mc.currentScreen!=screen) {
                if(nativeGl11!=null) {
                    nativeGl11.close();
                    nativeGl11=null;
                }
                if(nativeSprint!=null) {
                    nativeSprint.close();
                    nativeSprint=null;
                }
                screen=null;
                finished=true;
                System.out.println("R212_MANUAL_GUI_CLOSED_VANILLA_RESTORED");
            } else if(stage==1 && sourceMouseMode() &&
                      nativeGl11!=null) {
                // R218: cancel BOTH the native press and Java screen's
                // held edge on observed window focus loss. No raw queue hooks.
                screen.observeFocus(Display.isActive());
            }
        }
    }

    private void assertAuthoritativeSwitchVisual(long expectedSequence,
                                                  boolean expectedRequested) {
        if(!Boolean.getBoolean("slinkywinky.r215SourceMouseAuto") ||
           nativeGl11==null)return;
        R214NativeMenuOverlay.Visual visual=nativeGl11.visual();
        if(!visual.matches(expectedSequence,expectedRequested))
            throw new IllegalStateException("R216 source-painted switch pixel mismatch seq="+
                visual.sequence+" requested="+visual.requested+
                " rgb="+Integer.toHexString(visual.rgb)+
                " preview="+visual.previewFlag);
        System.out.println("R216_GL11_ACKNOWLEDGED_SPRINT_SWITCH_PIXEL_PASS seq="+
            visual.sequence+" requested="+visual.requested+
            " rgb="+String.format("%06X",visual.rgb)+
            " preview=false frames="+visual.frames);
        System.out.println("R217_SINGLE_PASS_GL11_ACKNOWLEDGED_SWITCH_PASS seq="+
            visual.sequence+" calls="+nativeGl11.paintedFrames()+
            " nativeFrames="+visual.frames+" extra=1");
    }
    private void requestedFromOwnedScreen(ProbeScreen source,
                                          R214NativeMenuOverlay.Request exactFromMouse) {
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
                if(sourceMouseMode() && exactFromMouse==null)
                    throw new IllegalStateException("R215 native painted hit required");
                exact=exactFromMouse!=null ? exactFromMouse :
                    nativeGl11.request(mc.displayWidth,mc.displayHeight);
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
                if(exactFromMouse!=null)
                    System.out.println("R215_EXACT_SOURCE_PAIRED_MOUSE_ACK_PASS id="+
                        nextId+" requested="+v.requested+" active="+v.active+
                        " activeRows="+v.activeRows+" frames="+v.drawCount);
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
        private boolean sourcePressed;
        private boolean foregroundObserved=true;
        private boolean heldCancelledOnClose;
        private int focusHeldCancels;
        private long lastPointerClock;

        ProbeScreen(R212ForgeGuiInputProof host,GuiScreen previous) {
            this.host=host;
            this.previous=previous;
        }
        @Override public void initGui() {
            owner=Thread.currentThread();
            buttonList.clear();
            sprint=new GuiButton(212, width/2-110, height/2-18,
                                 220,24,"Sprint | request ONLY");
            if(sourceMouseMode())sprint.visible=false;
            buttonList.add(sprint);
            if(sourceMouseMode())
                buttonList.add(new GuiButton(213,width-125,
                    height-26,120,20,"Close / restore"));
            else
                buttonList.add(new GuiButton(213,width/2-110,
                    height/2+16,220,24,"Close / restore vanilla"));
        }
        private boolean owned() {
            return owner==Thread.currentThread() &&
                   mc!=null && mc.currentScreen==this;
        }
        private long nextPointerTime() {
            long now=System.nanoTime()/1000000L;
            if(now<=lastPointerClock)now=lastPointerClock+1;
            lastPointerClock=now;
            return now;
        }
        // R218: takes an actual Display.isActive sample in MANUAL mode.
        // Automated acceptance explicitly feeds false/true without claiming
        // OS-level focus transitions. Always cancel the full Java+JNI pair.
        void observeFocus(boolean foreground) {
            if(!owned())throw new IllegalStateException("R218 lost focus owner");
            if(!foreground) {
                if(sourcePressed && host.nativeGl11!=null) {
                    host.nativeGl11.cancelMouse();
                    sourcePressed=false;
                    ++focusHeldCancels;
                    if(Boolean.getBoolean("slinkywinky.r215SourceMouseManual"))
                        System.out.println(
                            "R218_MANUAL_DISPLAY_INACTIVE_HELD_SOURCE_CANCELED");
                }
                foregroundObserved=false;
            } else if(!foregroundObserved) {
                foregroundObserved=true;
                if(Boolean.getBoolean("slinkywinky.r215SourceMouseManual"))
                    System.out.println("R218_MANUAL_DISPLAY_FOREGROUND_REOBSERVED");
            }
        }
        private boolean manualFocusAdmitted() {
            if(!Boolean.getBoolean("slinkywinky.r215SourceMouseManual"))
                return true; // scripted auto test has no physical focus claim
            final boolean foreground=Display.isActive();
            observeFocus(foreground);
            return foreground && foregroundObserved;
        }
        private int rawX(int x) {return (int)((long)x*mc.displayWidth/width);}
        private int rawY(int y) {return (int)((long)y*mc.displayHeight/height);}
        @Override protected void mouseClicked(int x,int y,int button)
                throws IOException {
            if(sourceMouseMode() && host.nativeGl11!=null && button==0) {
                if(!owned())throw new IllegalStateException("R215 mouse owner lost");
                // A manual input event must be observed in the foreground;
                // a duplicate Down must NEVER replace an unfinished edge.
                if(!manualFocusAdmitted())return;
                if(sourcePressed)return;
                if(host.nativeGl11.mouseDown(mc.displayWidth,mc.displayHeight,
                    rawX(x),rawY(y),nextPointerTime())) {
                    sourcePressed=true;
                    if(Boolean.getBoolean("slinkywinky.r215SourceMouseManual"))
                        System.out.println("R218_MANUAL_GUIScreen_SOURCE_DOWN_ACCEPTED");
                    return;
                }
            }
            super.mouseClicked(x,y,button);
        }
        @Override protected void mouseReleased(int x,int y,int button) {
            if(sourceMouseMode() && sourcePressed && host.nativeGl11!=null) {
                if(!owned())throw new IllegalStateException("R215 release owner lost");
                if(!manualFocusAdmitted())return; // canceled full pair
                sourcePressed=false;
                R214NativeMenuOverlay.Request req=button==0 ?
                    host.nativeGl11.mouseUp(mc.displayWidth,mc.displayHeight,
                        rawX(x),rawY(y),nextPointerTime()) : null;
                if(button!=0)host.nativeGl11.cancelMouse();
                if(req!=null) {
                    actions++;
                    host.requestedFromOwnedScreen(this,req);
                    if(Boolean.getBoolean("slinkywinky.r215SourceMouseManual"))
                        System.out.println(
                            "R218_MANUAL_GUIScreen_SOURCE_RELEASE_JNI_ACK id="+
                            host.lastRequestId);
                }
                return;
            }
            super.mouseReleased(x,y,button);
        }
        @Override public void onGuiClosed() {
            if(host.nativeGl11!=null && sourceMouseMode()) {
                heldCancelledOnClose=sourcePressed;
                host.nativeGl11.cancelMouse();
            }
            sourcePressed=false;
            foregroundObserved=false;
            super.onGuiClosed();
        }
        boolean heldCanceledOnClose() {return heldCancelledOnClose && !sourcePressed;}
        @Override protected void actionPerformed(GuiButton button)
                throws IOException {
            if(!owned())throw new IllegalStateException("R212 lost screen owner");
            if(button==sprint && button.id==212) {
                actions++;
                host.requestedFromOwnedScreen(this,null);
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
            final int statusY=sourceMouseMode()?height-55:height/2-56;
            drawCenteredString(fontRendererObj,
                "R215 Minecraft-owned source mouse | NO GAME ACTIONS",
                width/2,statusY,0xEEEEEE);
            drawCenteredString(fontRendererObj,
                nativeAck==null ? "No acknowledged native state yet" :
                "PR2 JNI ACK #"+nativeAck.id+
                    " Sprint requested="+nativeAck.requested+
                    " active="+nativeAck.active+" (no world)",
                width/2,statusY+16,0xAAAAAA);
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
        void disposableInvokePaintedNativeToggle() {
            if(!owned() || host.nativeGl11==null || !sourceMouseMode())
                throw new IllegalStateException("R215 painted source test absent");
            int[] px=host.nativeGl11.fixtureSourceTogglePoint(
                mc.displayWidth,mc.displayHeight);
            int sx=(int)(((long)px[0]*width+mc.displayWidth/2)/mc.displayWidth);
            int sy=(int)(((long)px[1]*height+mc.displayHeight/2)/mc.displayHeight);
            try {
                if(actions==0) {
                    // Orphan release and non-target down may not request.
                    mouseReleased(sx,sy,0);
                    mouseClicked(0,0,0);
                    mouseReleased(sx,sy,0);
                    if(actions!=0 || host.lastRequestId!=0)
                        throw new IllegalStateException("R215 orphan/non-target accepted");
                    // Wrong release location cancels a genuine held press.
                    mouseClicked(sx,sy,0);
                    mouseReleased(0,0,0);
                    if(actions!=0 || host.lastRequestId!=0)
                        throw new IllegalStateException("R215 mismatched release accepted");
                    System.out.println("R215_SOURCE_MOUSE_NEGATIVE_EDGES_REFUSED_PASS");
                    // R218: this is a *scripted* focus notification, not an
                    // observed OS transition or injected hardware event.
                    // A native held gesture must die with Java's held flag.
                    mouseClicked(sx,sy,0);
                    if(!sourcePressed)
                        throw new IllegalStateException("R218 held press missing");
                    mouseClicked(sx,sy,0); // duplicate Down cannot rearm
                    if(!sourcePressed)
                        throw new IllegalStateException("R218 duplicate Down changed hold");
                    observeFocus(false);
                    if(sourcePressed || focusHeldCancels!=1)
                        throw new IllegalStateException("R218 focus loss not canceled");
                    observeFocus(true);
                    mouseReleased(sx,sy,0); // orphan Up after restoration
                    if(actions!=0 || host.lastRequestId!=0)
                        throw new IllegalStateException("R218 stale release requested");
                    System.out.println("R218_FOCUS_LOSS_STALE_RELEASE_REFUSED_PASS");
                }
                mouseClicked(sx,sy,0);
                mouseReleased(sx,sy,0);
                if(!showsNativeAck(actions,host.requestedSprint,false))
                    throw new IllegalStateException("R215 real source toggle not acknowledged");
            } catch(IOException e) {throw new IllegalStateException(e);}
        }
        void disposableHoldSourceBeforeEscape() {
            if(!owned() || host.nativeGl11==null || !sourceMouseMode())
                throw new IllegalStateException("R218 escape cancel requires source");
            int[] px=host.nativeGl11.fixtureSourceTogglePoint(
                mc.displayWidth,mc.displayHeight);
            int sx=(int)(((long)px[0]*width+mc.displayWidth/2)/mc.displayWidth);
            int sy=(int)(((long)px[1]*height+mc.displayHeight/2)/mc.displayHeight);
            final int before=actions;
            try {mouseClicked(sx,sy,0);}
            catch(IOException e) {throw new IllegalStateException(e);}
            if(!sourcePressed || actions!=before)
                throw new IllegalStateException("R218 expected held source gesture");
        }
        void disposableInvokeVanillaEscape() {
            if(!owned())throw new IllegalStateException("R212 missing GUI");
            try {keyTyped((char)0,Keyboard.KEY_ESCAPE);}
            catch(IOException e) {throw new IllegalStateException(e);}
        }
    }
}
