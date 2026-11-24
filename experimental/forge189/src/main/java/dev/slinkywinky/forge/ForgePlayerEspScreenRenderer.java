package dev.slinkywinky.forge;

import dev.slinkywinky.EspNativeReadOnlyPreview;
import dev.slinkywinky.PlayerEspScreenProjection;
import net.minecraft.client.Minecraft;
import net.minecraft.client.gui.ScaledResolution;
import net.minecraft.client.renderer.entity.RenderManager;
import org.lwjgl.BufferUtils;
import org.lwjgl.opengl.GL11;

import java.nio.FloatBuffer;
import java.nio.IntBuffer;
import java.util.List;

/**
 * R306 diagnostic screen-space Player ESP only. The real Forge world
 * callback captures OpenGL modelview/projection/viewport BEFORE the
 * R303/R304 debug drawing, and Forge's 2D Text overlay consumes a
 * same-owner, same-tick, event7-native-authorized PlayerESP ScreenPlan.
 * These graphics are NOT the recovered commercial Slinky pixels.
 */
public final class ForgePlayerEspScreenRenderer {
    private ForgePlayerEspScreenRenderer() { }
    private static final FloatBuffer MODEL=BufferUtils.createFloatBuffer(16);
    private static final FloatBuffer PROJECTION=BufferUtils.createFloatBuffer(16);
    private static final IntBuffer VIEWPORT=BufferUtils.createIntBuffer(4);

    public static final class CapturedWorldFrame {
        public final PlayerEspScreenProjection.Frame camera;
        public final double viewerX,viewerY,viewerZ,partialTicks;
        private CapturedWorldFrame(PlayerEspScreenProjection.Frame camera,
                                   double x,double y,double z,double partial){
            this.camera=camera;viewerX=x;viewerY=y;viewerZ=z;partialTicks=partial;
        }
    }

    public static CapturedWorldFrame capture(Minecraft mc,
            EspNativeReadOnlyPreview.Result receipt,
            RenderManager render,double partialTicks){
        if(mc==null||receipt==null||render==null||
           !mc.isCallingFromMinecraftThread()||
           !Double.isFinite(partialTicks)||partialTicks<0||partialTicks>1||
           !Double.isFinite(render.viewerPosX)||
           !Double.isFinite(render.viewerPosY)||
           !Double.isFinite(render.viewerPosZ))
            throw new IllegalStateException("R306 invalid real render camera");
        MODEL.clear();PROJECTION.clear();VIEWPORT.clear();
        GL11.glGetFloat(GL11.GL_MODELVIEW_MATRIX,MODEL);
        GL11.glGetFloat(GL11.GL_PROJECTION_MATRIX,PROJECTION);
        GL11.glGetInteger(GL11.GL_VIEWPORT,VIEWPORT);
        MODEL.rewind();PROJECTION.rewind();VIEWPORT.rewind();
        double[] model=new double[16],projection=new double[16];
        int[] viewport=new int[4];
        for(int i=0;i<16;i++){
            model[i]=MODEL.get();
            projection[i]=PROJECTION.get();
        }
        VIEWPORT.get(viewport);
        ScaledResolution scaled=new ScaledResolution(mc);
        PlayerEspScreenProjection.Frame frame=new PlayerEspScreenProjection.Frame(
            receipt.worldToken,receipt.playerToken,receipt.generation,receipt.tick,
            model,projection,viewport,scaled.getScaledWidth(),scaled.getScaledHeight());
        return new CapturedWorldFrame(frame,render.viewerPosX,render.viewerPosY,
                                      render.viewerPosZ,partialTicks);
    }

    private static void line(double x1,double y1,double x2,double y2){
        GL11.glVertex2d(x1,y1);GL11.glVertex2d(x2,y2);
    }

    public static void draw(List<PlayerEspScreenProjection.Rect> rectangles){
        if(rectangles==null||rectangles.size()>100)
            throw new IllegalStateException("R306 invalid projected count");
        GL11.glPushAttrib(GL11.GL_ALL_ATTRIB_BITS);
        GL11.glPushMatrix();
        try{
            GL11.glDisable(GL11.GL_TEXTURE_2D);
            GL11.glDisable(GL11.GL_LIGHTING);
            GL11.glDisable(GL11.GL_CULL_FACE);
            GL11.glDisable(GL11.GL_DEPTH_TEST);
            GL11.glDepthMask(false);
            GL11.glEnable(GL11.GL_BLEND);
            GL11.glBlendFunc(GL11.GL_SRC_ALPHA,GL11.GL_ONE_MINUS_SRC_ALPHA);
            GL11.glLineWidth(1.25f);
            GL11.glBegin(GL11.GL_LINES);
            try {
                for(PlayerEspScreenProjection.Rect r:rectangles){
                    if(r==null||!Double.isFinite(r.left)||
                       !Double.isFinite(r.top)||!Double.isFinite(r.right)||
                       !Double.isFinite(r.bottom)||r.right<=r.left||
                       r.bottom<=r.top||!Float.isFinite(r.health)||
                       r.health<0||r.health>1)
                        throw new IllegalStateException("R306 invalid projected bounds");
                    if(r.hurtTime>0)GL11.glColor4f(1f,.40f,.30f,1f);
                    else GL11.glColor4f(.4f,.8f,1f,1f);
                    line(r.left,r.top,r.right,r.top);
                    line(r.right,r.top,r.right,r.bottom);
                    line(r.right,r.bottom,r.left,r.bottom);
                    line(r.left,r.bottom,r.left,r.top);
                    GL11.glColor4f(1f-r.health,r.health,.15f,1f);
                    line(r.left-2,r.bottom,
                         r.left-2,r.bottom-(r.bottom-r.top)*r.health);
                }
            } finally {GL11.glEnd();}
        } finally {
            GL11.glPopMatrix();
            GL11.glPopAttrib();
        }
    }
    /**
     * R307 separate opt-in source-geometry visualization: 8 original
     * recovered corners are projected by Frame.projectSource(). Source
     * HealthBar's three-stage black/yellow/health line positions and hurt
     * animation factor are preserved. RGB base colors and the line width
     * are DIAGNOSTIC because the user's original settings/name colors have
     * not been recovered into the JNI snapshot. Never claim pixel parity.
     */
    public static void drawSource(List<PlayerEspScreenProjection.Rect> rectangles,
                                  float partialTicks){
        if(rectangles==null||rectangles.size()>100||
           !Float.isFinite(partialTicks)||partialTicks<0||partialTicks>1)
            throw new IllegalStateException("R307 invalid source draw request");
        final float lineWidth=1.25f; // known diagnostic default, NOT recovered GUI setting.
        GL11.glPushAttrib(GL11.GL_ALL_ATTRIB_BITS);
        GL11.glPushMatrix();
        try{
            GL11.glDisable(GL11.GL_TEXTURE_2D);
            GL11.glDisable(GL11.GL_LIGHTING);
            GL11.glDisable(GL11.GL_CULL_FACE);
            GL11.glDisable(GL11.GL_DEPTH_TEST);
            GL11.glDepthMask(false);
            GL11.glEnable(GL11.GL_BLEND);
            GL11.glBlendFunc(GL11.GL_SRC_ALPHA,GL11.GL_ONE_MINUS_SRC_ALPHA);
            GL11.glLineWidth(lineWidth);
            GL11.glBegin(GL11.GL_LINES);
            try{
                for(PlayerEspScreenProjection.Rect r:rectangles){
                    if(r==null||!Double.isFinite(r.left)||!Double.isFinite(r.top)||
                       !Double.isFinite(r.right)||!Double.isFinite(r.bottom)||
                       r.right<=r.left||r.bottom<=r.top)
                        throw new IllegalStateException("R307 malformed projected box");
                    // The source hurt formula uses min(10,hurt-partial)*0.1.
                    // Opt-in diagnostic base cyan; original name/target colors
                    // and show_2d_box / hurt_effect settings remain unresolved.
                    float h=PlayerEspScreenProjection.sourceHurtMix(
                        r.hurtTime,partialTicks);
                    GL11.glColor4f(.4f+.6f*h,.8f*(1f-h),1f-h,1f);
                    line(r.left,r.top,r.right,r.top);
                    line(r.right,r.top,r.right,r.bottom);
                    line(r.right,r.bottom,r.left,r.bottom);
                    line(r.left,r.bottom,r.left,r.top);
                    for(PlayerEspScreenProjection.Segment seg:
                            PlayerEspScreenProjection.sourceHealthSegments(
                                r,lineWidth)){
                        GL11.glColor4f(seg.red,seg.green,seg.blue,1f);
                        line(seg.x,seg.y1,seg.x,seg.y2);
                    }
                }
            }finally{GL11.glEnd();}
        }finally{
            GL11.glPopMatrix();
            GL11.glPopAttrib();
        }
    }

    /**
     * R308 reconstructed source gating/color selection, consuming only the
     * row-aligned and exact-owner C++ native source StylePlan. The normal
     * R306/R307 Forge preview deliberately DOES NOT call this method until
     * original relationship/target provenance is available in the game.
     * Canvas graphics are approximated with GL11: no pixel-parity claim.
     */
    public static void drawStyled(
            java.util.List<PlayerEspScreenProjection.Rect> rectangles,
            EspNativeReadOnlyPreview.SourceStylePlan style,float partialTicks){
        if(rectangles==null||style==null||style.rows==null||
           rectangles.size()!=style.rows.size()||
           rectangles.size()>100||!Float.isFinite(partialTicks)||
           partialTicks<0||partialTicks>1||!Float.isFinite(style.lineWidth)||
           style.lineWidth<1||style.lineWidth>4)
            throw new IllegalStateException("R308 style source/row framing");
        GL11.glPushAttrib(GL11.GL_ALL_ATTRIB_BITS);
        GL11.glPushMatrix();
        try{
            GL11.glDisable(GL11.GL_TEXTURE_2D);
            GL11.glDisable(GL11.GL_LIGHTING);
            GL11.glDisable(GL11.GL_CULL_FACE);
            GL11.glDisable(GL11.GL_DEPTH_TEST);
            GL11.glDepthMask(false);
            GL11.glEnable(GL11.GL_BLEND);
            GL11.glBlendFunc(GL11.GL_SRC_ALPHA,GL11.GL_ONE_MINUS_SRC_ALPHA);
            GL11.glLineWidth(style.lineWidth);
            for(int i=0;i<rectangles.size();i++){
                final PlayerEspScreenProjection.Rect r=rectangles.get(i);
                final EspNativeReadOnlyPreview.SourceDrawStyle color=style.rows.get(i);
                if(r==null||color==null||!Double.isFinite(r.left)||
                   !Double.isFinite(r.right)||!Double.isFinite(r.top)||
                   !Double.isFinite(r.bottom)||r.right<=r.left||
                   r.bottom<=r.top)
                    throw new IllegalStateException("R308 style projection malformed");
                final boolean box=PlayerEspScreenProjection.sourceDrawBox(
                    style.showBox,color.target);
                final float[] channels=PlayerEspScreenProjection.sourceDrawColor(
                    color.red,color.green,color.blue,r.hurtTime,
                    partialTicks,style.hurtEffect);
                if(box && style.filled){
                    GL11.glColor4f(channels[0],channels[1],channels[2],.25f);
                    GL11.glBegin(GL11.GL_QUADS);
                    try{
                        GL11.glVertex2d(r.left,r.top);
                        GL11.glVertex2d(r.right,r.top);
                        GL11.glVertex2d(r.right,r.bottom);
                        GL11.glVertex2d(r.left,r.bottom);
                    }finally{GL11.glEnd();}
                }
                if(box||style.showHealth){
                    GL11.glBegin(GL11.GL_LINES);
                    try{
                        if(box){
                            GL11.glColor4f(channels[0],channels[1],channels[2],1f);
                            line(r.left,r.top,r.right,r.top);
                            line(r.right,r.top,r.right,r.bottom);
                            line(r.right,r.bottom,r.left,r.bottom);
                            line(r.left,r.bottom,r.left,r.top);
                        }
                        if(style.showHealth){
                            for(PlayerEspScreenProjection.Segment bar:
                                    PlayerEspScreenProjection.sourceHealthSegments(
                                        r,style.lineWidth)){
                                GL11.glColor4f(bar.red,bar.green,bar.blue,1f);
                                line(bar.x,bar.y1,bar.x,bar.y2);
                            }
                        }
                    }finally{GL11.glEnd();}
                }
            }
        }finally{
            GL11.glPopMatrix();
            GL11.glPopAttrib();
        }
    }

}
