package dev.slinkywinky;

/** R239 actual Java 8 JNI -> native C++ Clutch source planner, no Minecraft. */
public final class ClutchNativePreviewSmoke {
    private static void check(boolean condition,String reason) {
        if(!condition)throw new AssertionError(reason);
    }
    private static void refused(Runnable r,String reason) {
        try {r.run();throw new AssertionError("Unexpected admission: "+reason);}
        catch(IllegalStateException expected) { }
    }
    private static void badArgument(Runnable r,String reason) {
        try {r.run();}
        catch(IllegalArgumentException expected) {return;}
        throw new AssertionError("Unexpected argument admission: "+reason);
    }
    private static final class Port implements ClutchWorldSampler.Port {
        Object world=new Object(),player=new Object();
        boolean isClient=true,stone=true,ground=false;
        public boolean onClientThread(){return isClient;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public int blockId(int x,int y,int z) {
            if(ground && x==0 && y==63 && z==0)return 4;
            return stone && x==1 && y==63 && z==0?5:0;
        }
        public ClutchWorldSampler.Pos rayTrace(double[] from,double[] to) {
            throw new AssertionError("R239 must not perform a raycast");
        }
    }
    private static ClutchWorldSampler.Request request(long tick,float minHeight){
        return new ClutchWorldSampler.Request(3,tick,
            new double[]{.2,64,.2},
            new double[]{.8,65.8,.8},
            new double[]{.5,65.5,.5},1,minHeight,null);
    }
    public static void main(String[] args)throws Exception {
        Port p=new Port();
        try(ClutchWorldSampler sampler=new ClutchWorldSampler();
            ClutchNativePreview nativeOwner=new ClutchNativePreview()) {
            ClutchWorldSampler.Request r=request(1,0);
            ClutchWorldSampler.Snapshot snap=sampler.capture(p,r);
            ClutchNativePreview.Decision d=nativeOwner.observe(
                r,snap,-90,70,300,1,1.1);
            check(d.admitted && d.found && !d.faceUp &&
                d.sourceIndex==5 && d.blockX==1 &&
                d.blockY==63 && d.blockZ==0 && d.visited==6,
                "source-ordered native Clutch on actual JNI");
            // A forged request for the same tick must never pair the
            // original captured block IDs with different eye geometry.
            final ClutchWorldSampler.Snapshot first=snap;
            final ClutchWorldSampler.Request modifiedGeometry=
                new ClutchWorldSampler.Request(3,1,
                    new double[]{.2,64,.2},new double[]{.8,65.8,.8},
                    new double[]{.6,65.5,.5},1,0,null);
            badArgument(()->nativeOwner.observe(
                modifiedGeometry,first,-90,70,300,1,1.1),
                "same-tick geometry substitution");
            final ClutchWorldSampler.Request modifiedRange=
                new ClutchWorldSampler.Request(3,1,
                    new double[]{.2,64,.2},new double[]{.8,65.8,.8},
                    new double[]{.5,65.5,.5},0,0,null);
            badArgument(()->nativeOwner.observe(
                modifiedRange,first,-90,70,300,1,1.1),
                "same-tick range substitution");

            p.stone=false;
            r=request(2,0);
            snap=sampler.capture(p,r);
            d=nativeOwner.observe(r,snap,-90,70,300,1,2.2);
            check(d.admitted && !d.found && d.visited==6,
                "observed air produces no placement");

            final ClutchWorldSampler.Request replay=r;
            final ClutchWorldSampler.Snapshot prior=snap;
            refused(()->nativeOwner.observe(replay,prior,-90,70,300,1,2.2),
                "replayed native game tick");

            final boolean[] wrongThread={false};
            Thread worker=new Thread(()->{
                try {nativeOwner.observe(replay,prior,-90,70,300,1,2.2);}
                catch(IllegalStateException expected){wrongThread[0]=true;}
            });
            worker.start();worker.join();
            check(wrongThread[0],"Java owner denies wrong thread");

            // Deliberately create an independent sampler in the same JVM
            // with the same synthetic world/player and a newer tick. The
            // native owner must quarantine rather than accept the hardcoded
            // R239 identity (1,1) from a new observation session.
            try(ClutchWorldSampler other=new ClutchWorldSampler()) {
                final ClutchWorldSampler.Request foreign=request(3,0);
                final ClutchWorldSampler.Snapshot foreignSnapshot=
                    other.capture(p,foreign);
                refused(()->nativeOwner.observe(foreign,foreignSnapshot,
                    -90,70,300,1,3.3),"foreign sampler native tokens");
            }
            final ClutchWorldSampler.Request afterQuarantine=request(3,0);
            final ClutchWorldSampler.Snapshot originalSnapshot=
                sampler.capture(p,afterQuarantine);
            refused(()->nativeOwner.observe(afterQuarantine,originalSnapshot,
                -90,70,300,1,3.3),"native identity quarantine persists");
        }
        p=new Port();
        p.ground=true;
        try(ClutchWorldSampler sampler=new ClutchWorldSampler();
            ClutchNativePreview nativeOwner=new ClutchNativePreview()) {
            ClutchWorldSampler.Request r=request(1,2);
            ClutchWorldSampler.Snapshot snap=sampler.capture(p,r);
            ClutchNativePreview.Decision d=nativeOwner.observe(
                r,snap,-90,70,300,1,1.1);
            check(!d.found && d.visited==0,
                "observed blocking ground suppresses native plan");
        }
        System.out.println("R239_JAVA8_NATIVE_CLUTCH_SOURCE_PLANNER_PASS");
    }
}
