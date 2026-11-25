package dev.slinkywinky;

/** R337 Java8 real JNI client tick owner and world transition smoke. */
public final class SourceWorldReadOnlyHostSmoke {
    static void ok(boolean v,String message){if(!v)throw new AssertionError(message);}
    static void deny(Runnable r,String reason){
        boolean threw=false;
        try{r.run();}catch(IllegalArgumentException|IllegalStateException expected){threw=true;}
        ok(threw,reason);
    }
    static SourceWorldReadOnlyHost.Frame frame(){
        return new SourceWorldReadOnlyHost.Frame(
            new int[][]{
                {1,1,1,100,90,21,42,1,0,0,0,1},
                {1,1,1,0,0,0,0,0,0,0,0,0},
                {1,1,1,1,1,0,0,1,0,1,0,1,1,1,0},
                {1,0,0,0,0,1,0,0,0,0,1,1}
            },
            new double[][]{
                {9,0,120,3,0,6,10},
                {9,10,10,0,0,150},
                {9,0,1,5,3,2,0.5},
                {4}
            });
    }
    static final class Port implements SourceWorldReadOnlyHost.RenderPort {
        Object world=new Object(),player=new Object();
        boolean owner=true,swapWorld=false;
        SourceWorldReadOnlyHost.Frame current=frame();
        public boolean admittedClientThread(){return owner;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public SourceWorldReadOnlyHost.Frame capture(){
            if(swapWorld)world=new Object();
            return current;
        }
        public SourceWorldReadOnlyHost.RenderFrame captureRender(double delta,
                                                                   float partial){
            if(swapWorld)world=new Object();
            return new SourceWorldReadOnlyHost.RenderFrame(
                new double[]{delta,partial,0,1.62,0,0,0,0,0,0,2.8,
                             0,0,2.8,0,0,1,0,0.1,0},true);
        }
    }
    public static void main(String[] args)throws Exception{
        SourceWorldReadOnlyHost host=new SourceWorldReadOnlyHost();
        final Port port=new Port();
        SourceWorldReadOnlyHost.TickReceipt first=host.observe(port);
        ok(first!=null&&first.generation==1&&first.tick==1,"JNI first world tick");
        ok(first.backtrack.sourceEvent==18&&first.lagRange.sourceEvent==18&&
           first.timerRange.sourceEvent==0&&first.invWalk.sourceEvent==25,
           "Four original-table event subscriptions");
        ok(first.backtrack.at(3)==0,"Unknown original reach admission never guessed");
        ok(first.lagRange.at(3)==1,"Live-shaped Lag Range JNI source snapshot");
        ok(first.invWalk.at(2)==1,"Inventory callback executed");
        SourceRuntimePreview.Receipt rendering=host.observeRender(port,0.01,0f);
        ok(rendering!=null&&rendering.sourceEvent==3&&rendering.at(3)==1,
           "Real JNI timer raycast from render snapshot");
        ok(rendering.at(6)==1,"Native timer frame reports elapsed tick proposal");
        SourceWorldReadOnlyHost.TickReceipt second=host.observe(port);
        ok(second.generation==1&&second.tick==2,"Same owner keeps generation");
        port.player=new Object();
        SourceWorldReadOnlyHost.TickReceipt third=host.observe(port);
        ok(third.generation==2&&third.tick==1,"New player advances generation");
        port.world=null;
        ok(host.observeRender(port,0.01,0f)==null,
           "Lost world cannot dispatch render callback");
        ok(host.observe(port)==null&&host.last()==null,"Disconnected owner retires");
        port.world=new Object();
        SourceWorldReadOnlyHost.TickReceipt fourth=host.observe(port);
        ok(fourth.generation==3&&fourth.tick==1,"Relogin resets native source state");
        final boolean[] foreign={false};
        Thread t=new Thread(()->{
            try{host.observe(port);}catch(IllegalStateException expected){foreign[0]=true;}
        });
        t.start();t.join();
        ok(foreign[0],"Wrong thread rejected");
        host.close();
        host.close();
        deny(()->host.observe(port),"Closed source session rejected");
        deny(()->host.observeRender(port,0.01,0f),
             "Closed session cannot dispatch render");

        SourceWorldReadOnlyHost broken=new SourceWorldReadOnlyHost();
        port.swapWorld=true;
        deny(()->broken.observe(port),"Midread world replacement rejected");
        port.swapWorld=false;
        deny(()->broken.observe(port),"Failed snapshot retired native session");

        final SourceWorldReadOnlyHost.Frame original=frame();
        int[][] bits={
            {1,1,1,100,90,21,42,1,0,0,0,1},
            {1,1,1,0,0,0,0,0,0,0,0,0},
            {1,1,1,1,1,0,0,1,0,1,0,1,1,1,0},
            {1,0,0,0,0,1,0,0,0,0,1,1}
        };
        double[][] values={
            {9,0,120,3,0,6,10},{9,10,10,0,0,150},
            {9,0,1,5,3,2,0.5},{4}
        };
        SourceWorldReadOnlyHost.Frame copied=new SourceWorldReadOnlyHost.Frame(bits,values);
        bits[0][8]=1;values[0][0]=Double.NaN;
        ok(original!=null&&copied!=null,"Snapshot defensively copied");
        port.current=copied;
        SourceWorldReadOnlyHost safe=new SourceWorldReadOnlyHost();
        SourceWorldReadOnlyHost.TickReceipt receipt=safe.observe(port);
        ok(receipt.backtrack.at(3)==0,"Caller mutation cannot forge reach admission");
        safe.close();
        System.out.println("R337_JAVA8_REAL_WORLD_SOURCE_JNI_PASS");
    }
}
