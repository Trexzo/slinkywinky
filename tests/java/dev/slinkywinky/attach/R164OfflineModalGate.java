package dev.slinkywinky.attach;

/**
 * Test-only deterministic modal probe. NEVER install into a Minecraft game
 * classloader or ship inside slinkywinky_instrument.jar.
 */
public final class R164OfflineModalGate {
    private static volatile boolean quarantine;
    private R164OfflineModalGate(){}
    public static boolean shouldQuarantineInput(){
        return quarantine;
    }
    public static void setForFixture(boolean active) {
        quarantine=active;
    }
}
