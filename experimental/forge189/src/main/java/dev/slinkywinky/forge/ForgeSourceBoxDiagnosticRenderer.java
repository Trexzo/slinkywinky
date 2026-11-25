package dev.slinkywinky.forge;

import dev.slinkywinky.SourceRuntimePreview;
import dev.slinkywinky.SourceRemotePositionHistory;
import net.minecraft.client.renderer.entity.RenderManager;
import org.lwjgl.opengl.GL11;

/**
 * R341: optional local-only debug GL adapter for the REAL native source e6
 * box proposals. Restores matrix and GL state. No packet, world or input writes.
 * These are current-client target boxes, NOT verified historical remote ghosts.
 */
public final class ForgeSourceBoxDiagnosticRenderer {
    private ForgeSourceBoxDiagnosticRenderer(){ }
    private static void line(double x,double y,double z,double a,double b,double c){
        GL11.glVertex3d(x,y,z);GL11.glVertex3d(a,b,c);
    }
    private static void unitCubeLines(){
        GL11.glBegin(GL11.GL_LINES);
        try{
            for(int a=-1;a<=1;a+=2)
                for(int b=-1;b<=1;b+=2){
                    line(-1,a,b,1,a,b);
                    line(a,-1,b,a,1,b);
                    line(a,b,-1,a,b,1);
                }
        }finally{GL11.glEnd();}
    }
    private static void one(SourceRuntimePreview.SourceBox b){
        if(b==null||!b.draw)return;
        if(b.filled||!Double.isFinite(b.x)||!Double.isFinite(b.y)||
           !Double.isFinite(b.z)||Math.abs(b.x)>10000000||
           Math.abs(b.y)>10000000||Math.abs(b.z)>10000000||
           b.scaleX<=0||b.scaleY<=0||b.scaleZ<=0||
           b.scaleX>128||b.scaleY>128||b.scaleZ>128||
           !Double.isFinite(b.rotationY)||!Double.isFinite(b.lineWidth)||
           b.lineWidth<=0||b.lineWidth>64)
            throw new IllegalStateException("Invalid native source e6 GL diagnostic");
        GL11.glPushMatrix();
        try{
            GL11.glTranslated(b.x,b.y,b.z);
            if(b.rotate)GL11.glRotated(b.rotationY,0,1,0);
            GL11.glScaled(b.scaleX,b.scaleY,b.scaleZ);
            GL11.glColor4d(b.red,b.green,b.blue,1);
            GL11.glLineWidth((float)b.lineWidth);
            unitCubeLines();
        }finally{GL11.glPopMatrix();}
    }
    /**
     * Separate golden diagnostic for historical vanilla-applied serverPos.
     * This bypasses a DISABLED native module's draw verdict only in the
     * explicitly opt-in R342 debug world, and is NOT original ghost parity.
     */
    public static void drawHistory(SourceRemotePositionHistory.Historical h,
                                   RenderManager camera){
        if(h==null||camera==null||h.current==null||
           h.ageNanos<SourceRemotePositionHistory.MIN_AGE_NANOS||
           h.ageNanos>SourceRemotePositionHistory.MAX_AGE_NANOS)
            throw new IllegalArgumentException("Missing or stale true server history");
        final double x=h.current.x()-camera.viewerPosX;
        final double y=h.current.y()-camera.viewerPosY+1.0;
        final double z=h.current.z()-camera.viewerPosZ;
        if(!Double.isFinite(x)||!Double.isFinite(y)||!Double.isFinite(z)||
           Math.abs(x)>10000000||Math.abs(y)>10000000||Math.abs(z)>10000000)
            throw new IllegalArgumentException("Invalid historical camera space");
        GL11.glPushAttrib(GL11.GL_ALL_ATTRIB_BITS);
        GL11.glPushMatrix();
        try{
            GL11.glDisable(GL11.GL_TEXTURE_2D);
            GL11.glDisable(GL11.GL_LIGHTING);
            GL11.glDisable(GL11.GL_DEPTH_TEST);
            GL11.glDepthMask(false);
            GL11.glLineWidth(1.5f);
            GL11.glColor4f(1.0f,0.72f,0.18f,1.0f);
            GL11.glTranslated(x,y,z);
            GL11.glScaled(0.3,1.0,0.3);
            unitCubeLines();
        }finally{
            GL11.glPopMatrix();
            GL11.glPopAttrib();
        }
    }
    public static void draw(SourceRuntimePreview.SourceBoxes boxes){
        if(boxes==null)return;
        if((boxes.backtrack==null)||(boxes.lagRange==null))
            throw new IllegalStateException("Missing source event6 module");
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
            one(boxes.backtrack);
            one(boxes.lagRange);
        }finally{
            GL11.glPopMatrix();
            GL11.glPopAttrib();
        }
    }
}
