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
        boolean isClient=true,stone=true,ground=false,priorStone=false;
        boolean rayThrows=false;
        int priorX=-1,rays=0;
        ClutchWorldSampler.Pos forcedHit=null;
        public boolean onClientThread(){return isClient;}
        public Object worldIdentity(){return world;}
        public Object playerIdentity(){return player;}
        public int blockId(int x,int y,int z) {
            if(ground && x==0 && y==63 && z==0)return 4;
            if(priorStone && x==priorX && y==63 && z==0)return 4;
            return stone && x==1 && y==63 && z==0?5:0;
        }
        public ClutchWorldSampler.Pos rayTrace(double[] from,double[] to) {
            ++rays;
            if(rayThrows)
                throw new IllegalStateException("Synthetic world ray failed");
            return forcedHit; // null is a verified absent ray hit
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
            check(!d.preservedPrior && d.aimX>=d.blockX &&
                d.aimX<=d.blockX+1 && d.aimY>=d.blockY &&
                d.aimY<=d.blockY+1 &&
                d.aimZ>=d.blockZ && d.aimZ<=d.blockZ+1 &&
                Math.abs(Math.abs(d.yawDelta)+Math.abs(d.pitchDelta)-
                    d.angularCost)<.02,
                "source-native aim and signed rotation in JNI receipt");
            // Fresh native source-walk owners use identical test seeds.
            // Validate that the JNI receipt preserves the deterministic
            // source calculation, not merely a block index.
            try(ClutchWorldSampler repeated=new ClutchWorldSampler();
                ClutchNativePreview repeatedNative=new ClutchNativePreview()){
                final ClutchWorldSampler.Request rr=request(1,0);
                final ClutchNativePreview.Decision again=repeatedNative.observe(
                    rr,repeated.capture(p,rr),-90,70,300,1,1.1);
                check(again.found && again.aimX==d.aimX &&
                    again.aimY==d.aimY && again.aimZ==d.aimZ &&
                    again.yawDelta==d.yawDelta &&
                    again.pitchDelta==d.pitchDelta &&
                    again.angularCost==d.angularCost,
                    "native source-walk aim replay parity");
            }

            p.stone=false;
            r=request(2,0);
            snap=sampler.capture(p,r);
            d=nativeOwner.observe(r,snap,-90,70,300,1,2.2);
            check(d.admitted && !d.found && d.visited==6 &&
                d.aimX==0 && d.aimY==0 && d.aimZ==0 &&
                d.yawDelta==0 && d.pitchDelta==0,
                "observed air produces no aim or placement");

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
        // R244: real Java8 Port -> source-native aim preflight -> typed
        // world ray -> same native owner final decision, no game writes.
        final Port priorPort=new Port();
        priorPort.stone=false;
        priorPort.priorStone=true;
        try(ClutchWorldSampler sampler=new ClutchWorldSampler();
            ClutchNativePreview nativeOwner=new ClutchNativePreview()) {
            final ClutchWorldSampler.Request r=
                new ClutchWorldSampler.Request(3,1,
                    new double[]{.2,64,.2},
                    new double[]{.8,65.8,.8},
                    new double[]{.5,65.5,.5},
                    0,0,new ClutchWorldSampler.Pos(-1,63,0));
            final ClutchWorldSampler.Snapshot s=sampler.capture(priorPort,r);
            final ClutchNativePreview.Decision d=nativeOwner.observePrior(
                sampler,priorPort,r,s,new double[]{0,63.5,.5},
                -90,70,300,1,1.1);
            check(d.admitted && d.found && d.preservedPrior &&
                d.blockX==-1 && d.blockY==63 && d.blockZ==0 &&
                priorPort.rays==1,"native prior retained after real null ray");
            // A changed prior coordinate must not attach the same captured
            // block IDs and ray to a different logical module request.
            final ClutchWorldSampler.Request substituted=
                new ClutchWorldSampler.Request(3,1,
                    new double[]{.2,64,.2},
                    new double[]{.8,65.8,.8},
                    new double[]{.5,65.5,.5},
                    0,0,new ClutchWorldSampler.Pos(20,63,0));
            badArgument(()->nativeOwner.observePrior(
                sampler,priorPort,substituted,s,new double[]{0,63.5,.5},
                -90,70,300,1,1.1),"foreign previous block");
            refused(()->nativeOwner.observePrior(
                sampler,priorPort,r,s,new double[]{0,63.5,.5},
                -90,70,300,1,1.1),"replayed native Clutch prior tick");
        }
        check(priorPort.rays==1,"replay must not request another ray");

        final Port blockedPrior=new Port();
        blockedPrior.stone=false;
        blockedPrior.priorStone=true;
        blockedPrior.forcedHit=new ClutchWorldSampler.Pos(2,63,0);
        try(ClutchWorldSampler sampler=new ClutchWorldSampler();
            ClutchNativePreview nativeOwner=new ClutchNativePreview()) {
            final ClutchWorldSampler.Request r=
                new ClutchWorldSampler.Request(3,1,
                    new double[]{.2,64,.2},
                    new double[]{.8,65.8,.8},
                    new double[]{.5,65.5,.5},
                    0,0,new ClutchWorldSampler.Pos(-1,63,0));
            final ClutchNativePreview.Decision d=nativeOwner.observePrior(
                sampler,blockedPrior,r,sampler.capture(blockedPrior,r),
                new double[]{0,63.5,.5},-90,70,300,1,1.1);
            check(d.admitted && !d.found && !d.preservedPrior &&
                blockedPrior.rays==1,"obstructing native Clutch ray revoked prior");
        }
        final Port farPrior=new Port();
        farPrior.stone=false;
        farPrior.priorStone=true;
        farPrior.priorX=20;
        try(ClutchWorldSampler sampler=new ClutchWorldSampler();
            ClutchNativePreview nativeOwner=new ClutchNativePreview()) {
            final ClutchWorldSampler.Request r=
                new ClutchWorldSampler.Request(3,1,
                    new double[]{.2,64,.2},
                    new double[]{.8,65.8,.8},
                    new double[]{.5,65.5,.5},
                    0,0,new ClutchWorldSampler.Pos(20,63,0));
            final ClutchNativePreview.Decision d=nativeOwner.observePrior(
                sampler,farPrior,r,sampler.capture(farPrior,r),
                new double[]{20,63.5,.5},-90,70,300,1,1.1);
            check(d.admitted && !d.found && !d.preservedPrior &&
                farPrior.rays==0,"unreachable prior must not query world ray");
        }
        final Port failedRay=new Port();
        failedRay.stone=false;
        failedRay.priorStone=true;
        failedRay.rayThrows=true;
        try(ClutchWorldSampler sampler=new ClutchWorldSampler();
            ClutchNativePreview nativeOwner=new ClutchNativePreview()) {
            final ClutchWorldSampler.Request r=
                new ClutchWorldSampler.Request(3,1,
                    new double[]{.2,64,.2},
                    new double[]{.8,65.8,.8},
                    new double[]{.5,65.5,.5},
                    0,0,new ClutchWorldSampler.Pos(-1,63,0));
            final ClutchWorldSampler.Snapshot s=sampler.capture(failedRay,r);
            refused(()->nativeOwner.observePrior(
                sampler,failedRay,r,s,new double[]{0,63.5,.5},
                -90,70,300,1,1.1),"ray exception quarantines native owner");
            refused(()->nativeOwner.observePrior(
                sampler,failedRay,r,s,new double[]{0,63.5,.5},
                -90,70,300,1,1.1),"failed-ray Clutch owner cannot resume");
            check(failedRay.rays==1,"failed prior ray cannot be replayed");
        }
        System.out.println("R244_JAVA8_NATIVE_CLUTCH_PRIOR_RAY_PASS");
        System.out.println("R239_JAVA8_NATIVE_CLUTCH_SOURCE_PLANNER_PASS");
    }
}
