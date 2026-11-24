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
    /** Recovered screenPlan condition: show_2d_box OR target override. */
    public static boolean sourceDrawBox(boolean showBox,boolean target){
        return showBox||target;
    }
    /** RenderMath.blend(sourceRGB, (1,0,0), hurt) exact float order. */
    public static strictfp float[] sourceDrawColor(float red,float green,float blue,
            int hurt,float partialTicks,boolean enabled){
        if(!Float.isFinite(red)||!Float.isFinite(green)||
           !Float.isFinite(blue)||red<0||red>1||green<0||green>1||
           blue<0||blue>1)
            throw new IllegalArgumentException("R308 invalid source RGB");
        final float t=enabled?sourceHurtMix(hurt,partialTicks):0f;
        return new float[]{
            red+t*(1f-red),green+t*(0f-green),blue+t*(0f-blue)
        };
    }

    /** R307 original Java PlayerEspModule.healthBar() line coordinates. */
    public static final class Segment {
        public final double x,y1,y2;
        public final float red,green,blue;
        private Segment(double x,double y1,double y2,float r,float g,float b){
            this.x=x;this.y1=y1;this.y2=y2;red=r;green=g;blue=b;
        }
    }
    public static float sourceHurtMix(int hurt,float partialTicks){
        if(hurt<0||!Float.isFinite(partialTicks)||partialTicks<0||
           partialTicks>1)throw new IllegalArgumentException("R307 invalid hurt frame");
        return hurt>0 ? Math.min(10f,(float)hurt-partialTicks)*.1f : 0f;
    }
    /**
     * RenderConstants.f(1378840)=-1, f(1378844)=-4,
     * f(1378852)=1, f(1378856)=0.67. Deliberately no
     * source-invented line width: caller supplies its configured width.
     */
    public static List<Segment> sourceHealthSegments(Rect r,float lineWidth){
        if(r==null||!Double.isFinite(r.left)||!Double.isFinite(r.top)||
           !Double.isFinite(r.right)||!Double.isFinite(r.bottom)||
           r.right<=r.left||r.bottom<=r.top||!Float.isFinite(r.health)||
           r.health<0||r.health>1||!Float.isFinite(r.absorption)||
           r.absorption<0||r.absorption>1||!Float.isFinite(lineWidth)||
           lineWidth<=0||lineWidth>16)
            throw new IllegalArgumentException("R307 source health geometry invalid");
        final double x=(float)r.left-lineWidth-1.0f-4.0f;
        float middle=(float)r.bottom+
                     r.health*((float)r.top-(float)r.bottom);
        List<Segment> result=new ArrayList<Segment>(3);
        // The source emits black from minY to the health interpolation.
        result.add(new Segment(x,r.top,middle,0f,0f,0f));
        if(r.absorption>0){
            final float split=middle+
                              r.absorption*((float)r.bottom-middle);
            result.add(new Segment(x,middle,split,1f,.67f,0f));
            middle=split;
        }
        result.add(new Segment(x,middle,r.bottom,
                               1f-r.health,r.health,0f));
        return Collections.unmodifiableList(result);
    }

    public static final class Frame {
        public final long worldToken,playerToken,generation,tick;
        private final double[] combined=new double[16];
        private final float[] sourceModel=new float[16];
        private final float[] sourceProjection=new float[16];
        // Verified shared binary-lineage RenderConstants.d 1378976..1379168:
        // the original source's 8 corner order, including first hard-coded z=-0.4.
        private static final double[][] SOURCE_CORNERS={
            {-.4,0,-.4},{-.4,0,.4},{.4,0,-.4},{.4,0,.4},
            {-.4,2,-.4},{-.4,2,.4},{.4,2,-.4},{.4,2,.4}
        };
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
            for(int i=0;i<16;i++){
                if(!Double.isFinite(model[i])||!Double.isFinite(projection[i]))
                    throw new IllegalArgumentException("R306 nonfinite GL matrix");
                sourceModel[i]=(float)model[i];
                sourceProjection[i]=(float)projection[i];
                if(!Float.isFinite(sourceModel[i])||
                   !Float.isFinite(sourceProjection[i]))
                    throw new IllegalArgumentException("R307 GL float matrix overflow");
            }
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
        /**
         * R307: projection operand ordering from the recovered original
         * RenderMath.project(), not the R306 double-precision MVP shortcut.
         * The source's clip-depth test is strictly (0,1). NaN/Inf points
         * are additionally rejected for safe GL drawing.
         */
        private static strictfp float sourceModel(float[] m,int i,float x,float y,float z){
            return m[i+8]*z+(m[i]*x+m[i+4]*y)+m[i+12];
        }
        private static strictfp float sourceRow(float[] m,int i,float x,float y,float z,float w){
            return m[i+12]*w+(m[i+8]*z+(m[i]*x+m[i+4]*y));
        }
        private strictfp float[] sourcePoint(double x,double y,double z){
            final float fx=(float)x,fy=(float)y,fz=(float)z;
            if(!Float.isFinite(fx)||!Float.isFinite(fy)||!Float.isFinite(fz))
                return null;
            final float mx=sourceModel(sourceModel,0,fx,fy,fz);
            final float my=sourceModel(sourceModel,1,fx,fy,fz);
            final float mz=sourceModel(sourceModel,2,fx,fy,fz);
            final float mw=fz*sourceModel[11]+
                (fx*sourceModel[3]+fy*sourceModel[7])+sourceModel[15];
            final float w=sourceRow(sourceProjection,3,mx,my,mz,mw);
            if(w==0.0f||!Float.isFinite(w))return null;
            final float reciprocal=1.0f/w*.5f;
            final float depth=sourceRow(sourceProjection,2,mx,my,mz,mw)
                              *reciprocal+.5f;
            if(!(depth>0.0f&&depth<1.0f))return null;
            final float py=sourceRow(sourceProjection,1,mx,my,mz,mw)
                           *reciprocal+.5f;
            final float px=(mw*sourceProjection[12]+
                (mz*sourceProjection[8]+
                 (mx*sourceProjection[0]+my*sourceProjection[4])))
                *reciprocal+.5f;
            final float sx=px*(float)scaledWidth;
            final float sy=(float)scaledHeight-py*(float)scaledHeight;
            return Float.isFinite(sx)&&Float.isFinite(sy)?
                   new float[]{sx,sy}:null;
        }
        public static boolean sourceVisibleBounds(float minX,float minY,
                                                  float maxX,float maxY,
                                                  float width,float height){
            // Original RenderMath.visibleBounds: top-left OR bottom-right
            // must be inside screen; not a rectangle intersection or clamp.
            return (!(0f>minX)&&!(0f>minY)&&!(minX>width)&&!(minY>height))||
                   (!(0f>maxX)&&!(maxX>width)&&!(0f>maxY)&&!(maxY>height));
        }
        /**
         * Source-compatible screen box *projection* only. No original
         * color, player identity, setting defaults or GL primitive is
         * invented. Preserves the source's no-clamp bounds visibility rule.
         */
        public List<Rect> projectSource(EspNativeReadOnlyPreview.ScreenPlan rows){
            if(rows==null||rows.worldToken!=worldToken||
               rows.playerToken!=playerToken||rows.generation!=generation||
               rows.tick!=tick||rows.players==null||rows.players.size()>100)
                throw new IllegalStateException("R307 source screen owner mismatch");
            final List<Rect> out=new ArrayList<Rect>();
            for(EspNativeReadOnlyPreview.ScreenPlayer p:rows.players){
                if(p==null||!Double.isFinite(p.x)||!Double.isFinite(p.y)||
                   !Double.isFinite(p.z)||!Float.isFinite(p.health)||
                   p.health<0||p.health>1||!Float.isFinite(p.absorption)||
                   p.absorption<0||p.absorption>1||p.hurtTime<0)
                    throw new IllegalStateException("R307 invalid screen source");
                float minX=0,maxX=0,minY=0,maxY=0;
                boolean valid=true;
                for(int i=0;i<SOURCE_CORNERS.length;i++){
                    double[] corner=SOURCE_CORNERS[i];
                    float[] pt=sourcePoint(p.x+corner[0],
                                           p.y+corner[1],p.z+corner[2]);
                    if(pt==null){valid=false;break;}
                    if(i==0){minX=maxX=pt[0];minY=maxY=pt[1];}
                    else {
                        // Original else-if min/max update order.
                        if(minX>pt[0])minX=pt[0];
                        else if(pt[0]>maxX)maxX=pt[0];
                        if(minY>pt[1])minY=pt[1];
                        else if(pt[1]>maxY)maxY=pt[1];
                    }
                }
                if(valid&&sourceVisibleBounds(minX,minY,maxX,maxY,
                                               scaledWidth,scaledHeight)&&
                   maxX>minX&&maxY>minY)
                    out.add(new Rect(minX,minY,maxX,maxY,
                                     p.health,p.absorption,p.hurtTime));
            }
            return Collections.unmodifiableList(out);
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
