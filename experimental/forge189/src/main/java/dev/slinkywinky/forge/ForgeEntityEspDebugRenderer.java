package dev.slinkywinky.forge;

import dev.slinkywinky.EspNativeReadOnlyPreview;
import org.lwjgl.opengl.GL11;

/**
 * R304 opt-in diagnostic wire geometry for actual Forge RenderWorldLast.
 * Source R298/R299 C++ controls item counts, normalized player health
 * and farthest-first order. Diagnostic geometry/colors are NOT original
 * Slinky render settings; GL state is restored on every exit.
 */
public final class ForgeEntityEspDebugRenderer {
    private ForgeEntityEspDebugRenderer() { }

    private static void edge(double x1,double y1,double z1,
                             double x2,double y2,double z2) {
        GL11.glVertex3d(x1,y1,z1);
        GL11.glVertex3d(x2,y2,z2);
    }
    private static void playerBox() {
        final double h=.30,y0=.04,y1=1.80;
        GL11.glBegin(GL11.GL_LINES);
        try {
            for(int a=-1;a<=1;a+=2)
                for(int b=-1;b<=1;b+=2) {
                    edge(-h,y0,a*h,h,y0,a*h);
                    edge(-h,y1,a*h,h,y1,a*h);
                    edge(a*h,y0,-h,a*h,y0,h);
                    edge(a*h,y1,-h,a*h,y1,h);
                    edge(a*h,y0,b*h,a*h,y1,b*h);
                }
        } finally {GL11.glEnd();}
    }
    private static void itemMarker(int count) {
        if(count<=0)return; // R298 contributor row merged into another count
        double size=0.16+0.025*Math.min(8,count-1);
        GL11.glBegin(GL11.GL_LINES);
        try {
            edge(-size,0,0,size,0,0);
            edge(0,-size,0,0,size,0);
            edge(0,0,-size,0,0,size);
            edge(-size,0,-size,size,0,size);
            edge(-size,0,size,size,0,-size);
        } finally {GL11.glEnd();}
    }
    private static void healthBar(float h) {
        GL11.glColor4f(.15f,.15f,.15f,1f);
        GL11.glBegin(GL11.GL_LINES);
        try {edge(-.43,.04,0,-.43,1.84,0);}
        finally {GL11.glEnd();}
        GL11.glColor4f(1f-h,h,.2f,1f);
        GL11.glBegin(GL11.GL_LINES);
        try {edge(-.43,.04,0,-.43,.04+1.8*h,0);}
        finally {GL11.glEnd();}
    }
    public static void draw(EspNativeReadOnlyPreview.EntityPlan plan) {
        if(plan==null||plan.items==null||plan.players==null||
           plan.items.size()>100||plan.players.size()>100)
            throw new IllegalStateException("R304 invalid native plans");
        GL11.glPushAttrib(GL11.GL_ALL_ATTRIB_BITS);
        GL11.glPushMatrix();
        try {
            GL11.glDisable(GL11.GL_TEXTURE_2D);
            GL11.glDisable(GL11.GL_LIGHTING);
            GL11.glDisable(GL11.GL_CULL_FACE);
            GL11.glDisable(GL11.GL_DEPTH_TEST);
            GL11.glDepthMask(false);
            GL11.glEnable(GL11.GL_BLEND);
            GL11.glBlendFunc(GL11.GL_SRC_ALPHA,GL11.GL_ONE_MINUS_SRC_ALPHA);
            GL11.glLineWidth(1.35f);
            for(EspNativeReadOnlyPreview.ItemDraw i:plan.items) {
                if(i==null||!Double.isFinite(i.x)||!Double.isFinite(i.y)||
                   !Double.isFinite(i.z)||i.itemId<0||i.quantity<0||
                   i.quantity>99)
                    throw new IllegalStateException("R304 invalid item");
                if(i.quantity==0)continue;
                GL11.glPushMatrix();
                try {
                    GL11.glTranslated(i.x,i.y+.24,i.z);
                    GL11.glColor4f(1f,.78f,.2f,1f);
                    itemMarker(i.quantity);
                } finally {GL11.glPopMatrix();}
            }
            for(EspNativeReadOnlyPreview.PlayerDraw p:plan.players) {
                if(p==null||!Double.isFinite(p.x)||!Double.isFinite(p.y)||
                   !Double.isFinite(p.z)||!Float.isFinite(p.health)||
                   p.health<0||p.health>1||!Float.isFinite(p.absorption)||
                   p.absorption<0||p.absorption>1||p.hurtTime<0)
                    throw new IllegalStateException("R304 invalid player");
                GL11.glPushMatrix();
                try {
                    GL11.glTranslated(p.x,p.y,p.z);
                    if(p.hurtTime>0)GL11.glColor4f(1f,.3f,.25f,1f);
                    else GL11.glColor4f(.25f,.85f,1f,1f);
                    playerBox();
                    healthBar(p.health);
                } finally {GL11.glPopMatrix();}
            }
        } finally {
            GL11.glPopMatrix();
            GL11.glPopAttrib();
        }
    }
}
