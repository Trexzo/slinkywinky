package dev.slinkywinky;

/** R341: real Java8 JNI original source event6 dual boxes + owner fences. */
public final class SourceBoxNativeJniSmoke {
    static void yes(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    static void eq(double a,double b,String why){
        yes(Math.abs(a-b)<0.000001,why+" actual "+a);
    }
    static void denied(Runnable r,String why){
        boolean refused=false;
        try{r.run();}
        catch(IllegalArgumentException|IllegalStateException expected){refused=true;}
        yes(refused,why);
    }
    static double[] box(){
        return new double[]{8,0,2,10,0,2,4,1,0,0.5,0,90};
    }
    static final class Port implements SourceWorldReadOnlyHost.BoxPort {
        Object world=new Object(),player=new Object();
        boolean switchDuringRender=false,notReady=false;
        public boolean admittedClientThread(){return true;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public SourceWorldReadOnlyHost.Frame capture(){
            return SourceWorldReadOnlyHostSmoke.frame();
        }
        public SourceWorldReadOnlyHost.RenderFrame captureRender(double dt,float p){
            return null;
        }
        public SourceWorldReadOnlyHost.BoxFrame captureBoxes(float partial){
            if(notReady)return null;
            if(switchDuringRender)world=new Object();
            return new SourceWorldReadOnlyHost.BoxFrame(box());
        }
    }
    public static void main(String[] args)throws Exception{
        SourceRuntimePreview nativeOwner=new SourceRuntimePreview();
        final long w=111,p=222,g=1;
        // Test-only synthetic valid source reach eligibility: the actual
        // Forge R337 tick deliberately supplies false until reconstructed.
        nativeOwner.observe(w,p,g,1,SourceRuntimePreview.BACKTRACK_TICK,
            new int[]{1,1,1,100,90,21,42,1,1,0,1,0},
            new double[]{9,0,120,3,0,6,10});
        nativeOwner.observe(w,p,g,1,SourceRuntimePreview.LAG_RANGE_TICK,
            new int[]{1,1,1,0,0,0,1,0,0,0,0,1},
            new double[]{9,10,10,0,0,150});
        SourceRuntimePreview.SourceBoxes before=
            nativeOwner.observeBoxes(w,p,g,1,box());
        yes(before.backtrack.draw&&!before.lagRange.draw,
            "Source Backtrack e6 active, Lag Range not moving closer");
        eq(before.backtrack.x,5,"Interpolated camera-relative e6 x");
        eq(before.backtrack.y,0,"Interpolated camera-relative e6 center y");
        eq(before.backtrack.z,2,"Interpolated camera-relative e6 z");
        eq(before.backtrack.rotationY,135,"Original box source yaw interpolation");
        eq(before.backtrack.scaleX,0.3,"Original box half-width");
        eq(before.backtrack.scaleY,1.0,"Original box half-height");

        nativeOwner.observe(w,p,g,1,SourceRuntimePreview.LAG_RANGE_PACKET,
            new int[]{1,1,1,1},new double[]{0,1,0,0});
        nativeOwner.observe(w,p,g,2,SourceRuntimePreview.LAG_RANGE_TICK,
            new int[]{1,1,1,0,0,0,1,0,0,0,0,1},
            new double[]{9,10,10,0,0,150});
        SourceRuntimePreview.SourceBoxes both=
            nativeOwner.observeBoxes(w,p,g,2,box());
        yes(both.backtrack.draw&&both.lagRange.draw,
            "Both candidate source e6 handlers active in one JNI delivery");
        yes(!both.lagRange.rotate&&both.backtrack.rotate,
            "Only Backtrack source supports model yaw rotation");
        eq(both.lagRange.x,5,"Lag Range matches shared camera snapshot");
        yes(both.lagRange.green!=both.backtrack.green,
            "Debug colors distinguished, not asserted original");
        nativeOwner.setEnabled(w,p,g,2,SourceRuntimePreview.LAG_RANGE,false);
        SourceRuntimePreview.SourceBoxes disabled=
            nativeOwner.observeBoxes(w,p,g,2,box());
        yes(disabled.backtrack.draw&&!disabled.lagRange.draw,
            "Disabling one module does not crash the paired native event6");
        denied(()->nativeOwner.observeBoxes(w+1,p,g,2,box()),
               "No e6 world switch in old generation");
        denied(()->nativeOwner.observeBoxes(w,p,g,2,
               new double[]{1}),"Wrong box ABI denied");
        final boolean[] foreign={false};
        Thread thread=new Thread(()->{
            try{nativeOwner.observeBoxes(w,p,g,2,box());}
            catch(IllegalStateException expected){foreign[0]=true;}
        });
        thread.start();thread.join();
        yes(foreign[0],"Foreign thread cannot access native e6");
        nativeOwner.close();

        final SourceWorldReadOnlyHost host=new SourceWorldReadOnlyHost();
        final Port port=new Port();
        yes(host.observeBoxes(port,0.5f)==null,
            "No geometry callback before a valid client tick");
        host.observe(port);
        SourceRuntimePreview.SourceBoxes inactive=host.observeBoxes(port,0.5f);
        yes(inactive!=null&&!inactive.backtrack.draw&&!inactive.lagRange.draw,
            "Live-shaped Forge source flags fail closed with no remote box");
        port.world=new Object();
        yes(host.observeBoxes(port,0.5f)==null&&host.lastBoxes()==null,
            "World switch invalidates held geometry receipt");
        host.observe(port);
        port.notReady=true;
        yes(host.observeBoxes(port,0.5f)==null,
            "Missing target must not fabricate a render proposal");
        port.notReady=false;port.switchDuringRender=true;
        denied(()->host.observeBoxes(port,0.5f),
               "World switching midway through e6 fails closed");
        denied(()->host.observe(port),
               "Native source owner permanently retires on mixed snapshot");
        System.out.println("R341_JAVA8_NATIVE_SOURCE_EVENT6_BOX_PASS");
    }
}
