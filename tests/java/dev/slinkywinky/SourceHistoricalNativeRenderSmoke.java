package dev.slinkywinky;

/**
 * R342: historical vanilla-applied serverPos snapshot reaches REAL native
 * original source event6 callbacks, never a current-target invention.
 */
public final class SourceHistoricalNativeRenderSmoke {
    static void yes(boolean b,String msg){if(!b)throw new AssertionError(msg);}
    static void denied(Runnable r,String why){
        boolean threw=false;
        try{r.run();}catch(IllegalArgumentException|IllegalStateException expected){
            threw=true;
        }
        yes(threw,why);
    }
    static final class Port implements SourceWorldReadOnlyHost.HistoryPort {
        Object world=new Object(),player=new Object();
        boolean switchAfterCapture;
        int lastNativeX=256,lastNativeY=640,lastNativeZ=64;
        public boolean admittedClientThread(){return true;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public SourceWorldReadOnlyHost.Frame capture(){
            return SourceWorldReadOnlyHostSmoke.frame();
        }
        public SourceWorldReadOnlyHost.RemoteTarget captureRemoteTarget(){
            return new SourceWorldReadOnlyHost.RemoteTarget(71,1001,1002,
                lastNativeX,lastNativeY,lastNativeZ);
        }
        public SourceWorldReadOnlyHost.RenderFrame captureRender(double dt,float partial){
            return null;
        }
        public SourceWorldReadOnlyHost.BoxFrame captureBoxes(float partial){
            return null; // R341 live position fallback is NOT used here.
        }
        public SourceWorldReadOnlyHost.BoxFrame captureHistoricalBoxes(float partial,
                      SourceRemotePositionHistory.Historical historic){
            if(historic==null)throw new AssertionError("No authenticated history");
            if(switchAfterCapture)world=new Object();
            return new SourceWorldReadOnlyHost.BoxFrame(
                new double[]{historic.previous.x(),historic.previous.y(),
                    historic.previous.z(),historic.current.x(),
                    historic.current.y(),historic.current.z(),
                    4,1,0,partial,0,0});
        }
    }
    public static void main(String[] args)throws Exception{
        SourceWorldReadOnlyHost host=new SourceWorldReadOnlyHost();
        Port port=new Port();
        yes(host.observeHistoricalBoxes(port,0.5f)==null,
            "Cannot render historical source before world admission");
        SourceWorldReadOnlyHost.TickReceipt first=host.observe(port);
        yes(first.generation==1,"Source host tick admitted");
        long now=System.nanoTime();
        SourceRemotePositionHistory history=host.history();
        history.resetWorld(); // deterministic older input fixture
        history.applied(71,1001,1002,160,640,64,now-300_000_000L);
        history.applied(71,1001,1002,192,640,64,now-180_000_000L);
        history.applied(71,1001,1002,256,640,64,now-10_000_000L);
        SourceRuntimePreview.SourceBoxes boxes=host.observeHistoricalBoxes(port,0.5f);
        yes(boxes!=null&&boxes.generation==first.generation&&boxes.tick==first.tick,
            "Historical positions observed by same native JNI owner and tick");
        SourceRemotePositionHistory.Historical match=host.lastHistoryBox();
        yes(match!=null&&match.previous.serverX==160&&
            match.current.serverX==192,
            "Native source receives older serverPos rather than current 256");
        yes(match.ageNanos>=SourceRemotePositionHistory.MIN_AGE_NANOS,
            "No historical lookahead or time travel");
        yes(!boxes.backtrack.draw&&!boxes.lagRange.draw,
            "Original source render disabled when eligibility unproven");

        final boolean[] badThread={false};
        Thread foreign=new Thread(()->{
            try{host.observeHistoricalBoxes(port,0.5f);}
            catch(IllegalStateException expected){badThread[0]=true;}
        });
        foreign.start();foreign.join();
        yes(badThread[0],"History render cannot run on Netty");
        port.world=new Object();
        yes(host.observeHistoricalBoxes(port,0.5f)==null,
            "World switch retires old remote positions");
        yes(host.history().tracked()==0&&host.lastHistoryBox()==null,
            "World switch clears selected historical draw receipt");
        SourceWorldReadOnlyHost.TickReceipt next=host.observe(port);
        yes(next.generation==2,"New native source world epoch");
        host.close();

        SourceWorldReadOnlyHost mixed=new SourceWorldReadOnlyHost();
        Port broken=new Port();
        mixed.observe(broken);
        SourceRemotePositionHistory h=mixed.history();
        h.resetWorld();
        h.applied(71,1001,1002,160,640,64,System.nanoTime()-200_000_000L);
        broken.switchAfterCapture=true;
        denied(()->mixed.observeHistoricalBoxes(broken,0.5f),
            "Changed world during historical capture fails closed");
        denied(()->mixed.observe(broken),
            "Mixed-world source JNI handle permanently retired");
        System.out.println("R342_JAVA8_HISTORICAL_SOURCE_EVENT6_JNI_PASS");
    }
}
