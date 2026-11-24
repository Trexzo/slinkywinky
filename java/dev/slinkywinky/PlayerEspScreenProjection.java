package dev.slinkywinky;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * R306 independent, testable OpenGL column-major world-to-screen mapping.
 *
 * Consumes ONLY already native-selected R299 PlayerEspController screen rows
 * admitted after source event 7. The matrix pair is captured from the actual
 * Forge RenderWorldLast camera; overlay display is in scaled GUI coordinates.
 * A fully/partially near-clipped box is conservatively rejected, not
 * fabricated. The source client's original box geometry is still unknown.
 */
public final class PlayerEspScreenProjection {
    private PlayerEspScreenProjection() { }
    public static final class Rect {
        public final double left,top,right,bottom;
        public final float health,absorption;
        public final int hurtTime;
        private Rect(double left,double top,double right,double bottom,
                     float health,float absorption,int hurtTime){
            this.left=left;this.top=top;this.right=right;this.bottom=bottom;
            this.health=health;this.absorption=absorption;this.hurtTime=hurtTime;
        }
    }
    public static final class Frame {
        public final long worldToken,playerToken,generation,tick;
        private final double[] combined=new double[16];
        public final int scaledWidth,scaledHeight;
        public Frame(long world,long player,long generation,long tick,
                     double[] model,double[] projection,int[] viewport,
                     int scaledWidth,int scaledHeight){
            if(world<=0||player<=0||world==player||generation<=0||tick<0||
               model==null||projection==null||viewport==null||
               model.length!=16||projection.length!=16||viewport.length!=4||
               viewport[0]!=0||viewport[1]!=0||
               viewport[2]<=0||viewport[3]<=0||
               scaledWidth<=0||scaledHeight<=0||
               scaledWidth>viewport[2]||scaledHeight>viewport[3])
                throw new IllegalArgumentException("R306 invalid world camera/viewport");
            for(int i=0;i<16;i++)
                if(!Double.isFinite(model[i])||!Double.isFinite(projection[i]))
                    throw new IllegalArgumentException("R306 nonfinite GL matrix");
            // Standard OpenGL column-major MVP = projection * modelview.
            for(int col=0;col<4;col++)
                for(int row=0;row<4;row++)
                    for(int k=0;k<4;k++)
                        combined[4*col+row]+=
                            projection[4*k+row]*model[4*col+k];
            for(double v:combined)
                if(!Double.isFinite(v))
                    throw new IllegalArgumentException("R306 nonfinite composition");
            worldToken=world;playerToken=player;
            this.generation=generation;this.tick=tick;
            this.scaledWidth=scaledWidth;this.scaledHeight=scaledHeight;
        }
        private double[] projectPoint(double x,double y,double z){
            final double cx=combined[0]*x+combined[4]*y+combined[8]*z+combined[12];
            final double cy=combined[1]*x+combined[5]*y+combined[9]*z+combined[13];
            final double cz=combined[2]*x+combined[6]*y+combined[10]*z+combined[14];
            final double cw=combined[3]*x+combined[7]*y+combined[11]*z+combined[15];
            if(!Double.isFinite(cx)||!Double.isFinite(cy)||!Double.isFinite(cz)||
               !Double.isFinite(cw)||cw<=0.000001||cz< -cw||cz>cw)
                return null; // Never project points behind/through the camera.
            final double sx=(cx/cw+1.0)*0.5*scaledWidth;
            final double sy=(1.0-cy/cw)*0.5*scaledHeight;
            return Double.isFinite(sx)&&Double.isFinite(sy)?
                new double[]{sx,sy}:null;
        }
        public List<Rect> project(EspNativeReadOnlyPreview.ScreenPlan nativeRows){
            if(nativeRows==null||nativeRows.worldToken!=worldToken||
               nativeRows.playerToken!=playerToken||
               nativeRows.generation!=generation||nativeRows.tick!=tick||
               nativeRows.players==null||nativeRows.players.size()>100)
                throw new IllegalStateException("R306 stale/foreign screen rows");
            List<Rect> out=new ArrayList<Rect>();
            for(EspNativeReadOnlyPreview.ScreenPlayer p:nativeRows.players){
                if(p==null||!Double.isFinite(p.x)||!Double.isFinite(p.y)||
                   !Double.isFinite(p.z)||!Float.isFinite(p.health)||
                   p.health<0||p.health>1||!Float.isFinite(p.absorption)||
                   p.absorption<0||p.absorption>1||p.hurtTime<0)
                    throw new IllegalStateException("R306 invalid screen row");
                double minX=Double.POSITIVE_INFINITY;
                double maxX=Double.NEGATIVE_INFINITY;
                double minY=Double.POSITIVE_INFINITY;
                double maxY=Double.NEGATIVE_INFINITY;
                boolean visible=true;
                // Diagnostic human box dimensions are not original settings.
                for(int dx=-1;dx<=1&&visible;dx+=2)
                    for(int dz=-1;dz<=1&&visible;dz+=2)
                        for(int dy=0;dy<=1;dy++){
                            double[] pt=projectPoint(
                                p.x+dx*0.30,p.y+dy*1.80,p.z+dz*0.30);
                            if(pt==null){visible=false;break;}
                            minX=Math.min(minX,pt[0]);maxX=Math.max(maxX,pt[0]);
                            minY=Math.min(minY,pt[1]);maxY=Math.max(maxY,pt[1]);
                        }
                if(!visible||maxX<=0||maxY<=0||
                   minX>=scaledWidth||minY>=scaledHeight)continue;
                minX=Math.max(0,minX);maxX=Math.min(scaledWidth,maxX);
                minY=Math.max(0,minY);maxY=Math.min(scaledHeight,maxY);
                if(!(maxX>minX&&maxY>minY))continue;
                out.add(new Rect(minX,minY,maxX,maxY,
                                 p.health,p.absorption,p.hurtTime));
            }
            return Collections.unmodifiableList(out);
        }
    }
}
