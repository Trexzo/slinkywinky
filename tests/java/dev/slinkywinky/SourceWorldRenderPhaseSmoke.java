package dev.slinkywinky;

/** R338 render-phase ownership and real JNI predictive ray regression. */
public final class SourceWorldRenderPhaseSmoke {
    static void yes(boolean ok,String why){if(!ok)throw new AssertionError(why);}
    static void deny(Runnable r,String why){
        boolean refused=false;
        try{r.run();}catch(IllegalArgumentException|IllegalStateException expected){
            refused=true;
        }
        yes(refused,why);
    }
    static final class Port implements SourceWorldReadOnlyHost.RenderPort {
        Object world=new Object(),player=new Object();
        boolean missingTarget=false,swapDuringRender=false,invalidClock=false;
        final SourceWorldReadOnlyHost.Frame initial=SourceWorldReadOnlyHostSmoke.frame();
        public boolean admittedClientThread(){return true;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public SourceWorldReadOnlyHost.Frame capture(){return initial;}
        public SourceWorldReadOnlyHost.RenderFrame captureRender(
                double dt,float partial){
            if(missingTarget)return null;
            if(swapDuringRender)world=new Object();
            final double[] raw={invalidClock?Double.NaN:dt,partial,
                0,1.62,0,0,0,0,0,0,2.8,0,0,2.8,0,0,1,0,0.1,0};
            return new SourceWorldReadOnlyHost.RenderFrame(raw,true);
        }
    }
    public static void main(String[] args)throws Exception{
        Port port=new Port();
        SourceWorldReadOnlyHost h=new SourceWorldReadOnlyHost();
        yes(h.observeRender(port,0.01,0f)==null,
            "No native frame before owner tick");
        h.observe(port);
        port.missingTarget=true;
        yes(h.observeRender(port,0.01,0f)==null,
            "No target means no synthetic native AABB input");
        port.missingTarget=false;
        SourceRuntimePreview.Receipt ray=h.observeRender(port,0.01,0f);
        yes(ray!=null&&ray.sourceEvent==3&&ray.at(3)==1&&
            ray.at(6)==1,"Real JNI render frame predicts source ray burst");
        yes(h.lastRender()==ray,"Render receipt owned by same native session");
        final boolean[] foreign={false};
        Thread foreignThread=new Thread(()->{
            try{h.observeRender(port,0.01,0f);}
            catch(IllegalStateException expected){foreign[0]=true;}
        });
        foreignThread.start();foreignThread.join();
        yes(foreign[0],"No foreign-thread render callbacks");

        port.world=new Object();
        yes(h.observeRender(port,0.01,0f)==null,
            "World replacement refuses stale render-frame observation");
        yes(h.lastRender()==null&&h.last()==null,
            "World replacement retires all outstanding frame receipts");
        SourceWorldReadOnlyHost.TickReceipt epoch=h.observe(port);
        yes(epoch.generation==2&&epoch.tick==1,
            "New world tick obtains next source generation");
        h.close();
        h.close();

        // The immutable frame copies caller bytes before exposing a JNI call.
        double[] raw={0.01,0,0,1.62,0,0,0,0,0,0,2.8,0,0,2.8,
                      0,0,1,0,0.1,0};
        SourceWorldReadOnlyHost.RenderFrame copied=
            new SourceWorldReadOnlyHost.RenderFrame(raw,true);
        raw[0]=Double.NaN;
        yes(copied.values[0]==0.01,"Defensive copy of render camera frame");

        SourceWorldReadOnlyHost broken=new SourceWorldReadOnlyHost();
        port.invalidClock=true;
        broken.observe(port);
        deny(()->broken.observeRender(port,0.01,0f),
             "Invalid render clock rejected before native replay");
        deny(()->broken.observe(port),
             "Render failure permanently retires native observer");

        SourceWorldReadOnlyHost switched=new SourceWorldReadOnlyHost();
        port.invalidClock=false;port.swapDuringRender=true;
        switched.observe(port);
        deny(()->switched.observeRender(port,0.01,0f),
             "Midrender owner replacement fails closed");
        deny(()->switched.observe(port),
             "Native handle closed after mixed-world render");
        System.out.println("R338_JAVA8_REAL_RENDER_SOURCE_JNI_PASS");
    }
}
