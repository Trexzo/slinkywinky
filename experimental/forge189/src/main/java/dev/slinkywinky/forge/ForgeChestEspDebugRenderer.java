package dev.slinkywinky.forge;

import dev.slinkywinky.EspNativeReadOnlyPreview;
import org.lwjgl.opengl.GL11;

/**
 * R303 opt-in diagnostic wire-box drawer only: the world-space centers,
 * block-type IDs and scale originate from R297's actual C++ draw plan.
 * RGB colors are intentionally diagnostic, not claimed original settings.
 * State and matrix stacks are restored even if a renderer step throws.
 */
public final class ForgeChestEspDebugRenderer {
    private ForgeChestEspDebugRenderer() { }

    private static void edge(double x1,double y1,double z1,
                             double x2,double y2,double z2) {
        GL11.glVertex3d(x1,y1,z1);
        GL11.glVertex3d(x2,y2,z2);
    }
    private static void outline(double half) {
        GL11.glBegin(GL11.GL_LINES);
        try {
            for(int a=-1;a<=1;a+=2)
                for(int b=-1;b<=1;b+=2) {
                    edge(-half,a*half,b*half,half,a*half,b*half);
                    edge(a*half,-half,b*half,a*half,half,b*half);
                    edge(a*half,b*half,-half,a*half,b*half,half);
                }
        } finally { GL11.glEnd(); }
    }
    private static void diagnosticColor(int blockId) {
        switch(blockId) {
            case 54:  GL11.glColor4f(0.30f,0.85f,0.30f,1.0f);break;
            case 130: GL11.glColor4f(0.75f,0.30f,0.90f,1.0f);break;
            case 146: GL11.glColor4f(0.95f,0.40f,0.35f,1.0f);break;
            case 138: GL11.glColor4f(0.30f,0.75f,0.95f,1.0f);break;
            default: throw new IllegalStateException("R303 unknown native block");
        }
    }
    public static void draw(EspNativeReadOnlyPreview.ChestPlan plan) {
        if(plan==null||plan.draws==null||plan.draws.size()>100||
           plan.scale!=0.5f||!Float.isFinite(plan.lineWidth)||
           plan.lineWidth<=0||plan.lineWidth>16)
            throw new IllegalStateException("R303 invalid source draw plan");
        // Never retain an OpenGL state change outside this event callback.
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
            GL11.glLineWidth(plan.lineWidth);
            for(EspNativeReadOnlyPreview.ChestDraw row:plan.draws) {
                if(row==null||!Double.isFinite(row.centerX)||
                   !Double.isFinite(row.centerY)||!Double.isFinite(row.centerZ)||
                   row.filled)
                    throw new IllegalStateException("R303 invalid outline row");
                GL11.glPushMatrix();
                try {
                    GL11.glTranslated(row.centerX,row.centerY,row.centerZ);
                    diagnosticColor(row.blockId);
                    outline(plan.scale);
                } finally { GL11.glPopMatrix(); }
            }
        } finally {
            GL11.glPopMatrix();
            GL11.glPopAttrib();
        }
    }
}
