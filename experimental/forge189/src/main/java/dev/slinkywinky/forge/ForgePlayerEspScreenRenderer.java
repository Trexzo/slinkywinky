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
}
