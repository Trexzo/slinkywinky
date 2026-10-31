package dev.slinkywinky;

/** Simulate 125 render frames/s through the real Java 8 JNI boundary. */
public final class RenderStress {
    private static void check(boolean ok, String message) {
        if (!ok) throw new AssertionError(message);
    }
    private static final class Sink implements GameHost.Actions {
        long frames, attacks, rotations, releases;
        @Override public void apply(NativeEngine.Decision d) {
            ++frames;
            if (d.attackPulse) ++attacks;
            if (d.hasRotation) ++rotations;
            check(d.leftEdge >= 0 && d.leftEdge <= 2, "invalid left edge");
            check(d.rightEdge >= 0 && d.rightEdge <= 2, "invalid right edge");
            check(Float.isFinite(d.yaw) && Float.isFinite(d.pitch), "invalid rotation");
        }
        @Override public void releaseSynthetic() {++releases;}
    }
    public static void main(String[] args) {
        final Sink sink=new Sink();
        final Object[] worlds=new Object[5];
        for (int i=0;i<worlds.length;i++) worlds[i]=new Object();
        final Object player=new Object();
        final GameHost host=new GameHost(new NativeEngine(new int[]{1,2,3,4}),sink);
        final GameHost.Frame f=new GameHost.Frame();
        f.settings=HostSettings.safeDefaults().withToggle(HostSettings.CLICKER,true)
            .withToggle(HostSettings.AIM,true).withToggle(HostSettings.REQUIRE_MOUSE,false)
            .toNativeArray();
        f.playerToken=player;
        f.weapon=true;f.targetPresent=true;f.targetAlive=true;
        f.targetVisible=true;f.targetId=42;f.targetX=2;
        for (int i=0;i<25000;i++) {
            f.monotonicMs=1000L+i*8L;
            f.worldToken=worlds[(i/5000)%5];
            f.guiOpen=(i%1100<7);
            f.manualLeft=(i%210<100);
            f.hurtTime=i%10;
            host.tick(f);
        }
        check(host.generation()==5,"expected five world generations");
        check(sink.frames==25000,"lost Java/native decisions");
        check(sink.rotations>10000,"lost aim decisions");
        check(sink.releases>=5,"missing world/session releases");
        host.close();
        check(sink.releases>=6,"missing final release");
        System.out.println("PASS Java 8 JNI: 25,000 render-frequency decisions, 5 worlds, no stale actions");
    }
}
