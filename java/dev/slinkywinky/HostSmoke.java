package dev.slinkywinky;

/** Concrete integration contract smoke test using a fake game host, real JNI. */
public final class HostSmoke {
    private static void check(boolean pass, String message) {
        if (!pass) throw new AssertionError(message);
    }
    private static final class Sink implements GameHost.Actions {
        int applied, released, attacks, rotations;
        boolean throwOnce;
        @Override public void apply(NativeEngine.Decision decision) {
            if (throwOnce) { throwOnce=false; throw new IllegalStateException("game action failure"); }
            ++applied;
            if (decision.attackPulse) ++attacks;
            if (decision.hasRotation) ++rotations;
        }
        @Override public void releaseSynthetic() { ++released; }
    }
    private static GameHost.Frame frame(Object world, Object player, long time) {
        GameHost.Frame f=new GameHost.Frame();
        f.monotonicMs=time; f.worldToken=world;f.playerToken=player;
        f.weapon=true;f.targetPresent=true;f.targetAlive=true;f.targetVisible=true;
        f.targetX=2;f.yaw=0; f.pitch=0;
        f.settings=new float[] {1,0,12,1,0,4,15,15,90,1,0,3.1f,5,1,3.05f,3,0,1,0};
        return f;
    }
    public static void main(String[] unused) throws Exception {
        final Object worldA=new Object(),worldB=new Object(),playerA=new Object(),playerB=new Object();
        Sink actions=new Sink();
        GameHost host=new GameHost(new NativeEngine(new int[]{1,2,3,4}),actions);
        GameHost.Frame a=frame(worldA,playerA,100);
        HostSettings defaults = HostSettings.safeDefaults();
        check(defaults.toNativeArray()[HostSettings.CPS]==12,"reference CPS initial value");
        check(defaults.toNativeArray()[HostSettings.REACH_DISTANCE]==3.05f,"reference reach");
        check(defaults.toNativeArray()[HostSettings.CLICKER]==0,"explicit enablement required");
        boolean invalidSettings=false;
        try { defaults.withValue(HostSettings.CPS, -50); }
        catch (IllegalArgumentException expected) { invalidSettings=true; }
        check(invalidSettings,"invalid CPS rejected");
        float[] leaked=defaults.toNativeArray();leaked[HostSettings.CPS]=100;
        check(defaults.toNativeArray()[HostSettings.CPS]==12,"settings defensive copy");
        host.tick(a);
        check(host.generation()==1,"first world generation");
        check(actions.rotations==1 && actions.attacks==1,"decisions reached adapter");
        host.tick(frame(worldA,playerA,120));
        check(host.generation()==1,"same player generation remains");
        host.tick(frame(worldA,playerB,140));
        check(host.generation()==2,"player swap isolated");
        host.tick(frame(worldB,playerB,160));
        check(host.generation()==3,"world swap isolated");
        host.tick(frame(null,null,180));
        check(actions.released>=4,"world leave releases input");
        host.tick(frame(worldA,playerA,200));
        check(host.generation()==4,"world reentry increments generation");
        final boolean[] rejected={false};
        Thread foreign=new Thread(() -> {
            try { host.tick(frame(worldA,playerA,210)); }
            catch (IllegalStateException e) { rejected[0]=true; }
        });
        foreign.start();foreign.join();
        check(rejected[0],"foreign thread rejected");
        actions.throwOnce=true;
        try { host.tick(frame(worldA,playerA,220));throw new AssertionError("must throw"); }
        catch (IllegalStateException expected) { }
        check(actions.released>=5,"action failure released inputs");
        host.tick(frame(worldA,playerA,240));
        GameHost.Frame bad=frame(worldA,playerA,250);
        bad.settings[HostSettings.CPS]=Float.NaN;
        boolean rejectedSettings=false;
        try {host.tick(bad);}catch(IllegalArgumentException expected){rejectedSettings=true;}
        check(rejectedSettings && actions.released>=6,"nonfinite settings fail closed");
        check(host.generation()==5,"recovery requires new generation");
        try { host.tick(frame(worldA,playerA,230));throw new AssertionError("time reversal must reject"); }
        catch (IllegalArgumentException expected) { }
        check(actions.released>=6,"reversal fail closed");
        host.close();
        check(actions.released>=7,"close releases synthetic input");
        try { host.tick(frame(worldA,playerA,260));throw new AssertionError("closed host admitted call"); }
        catch (IllegalStateException expected) { }
        System.out.println("PASS detailed JNI host: world/player generations, manual input safety, failure cleanup, off-thread rejection");
    }
}
