package dev.slinkywinky;

/** R204 read-only Java 8 JNI roundtrip: no Minecraft classes required. */
public final class SprintObservationProbeSmoke {
    private static void check(boolean ok, String why) {
        if (!ok) throw new AssertionError("R204: " + why);
    }
    private static long[] clocks(long time, long gen) {
        return new long[]{time, gen};
    }
    private static int[] flags(int f, boolean gui, boolean sprinting) {
        // allow backwards and sideways; do not enable item-use override.
        return new int[]{f, gui ? 1 : 0, sprinting ? 1 : 0, 0,
                         0, 0, 1, 1};
    }
    private static SprintObservationProbe.Decision send(
            SprintObservationProbe probe, long time, long gen,
            int observedFlags, boolean gui, boolean sprinting) {
        return probe.observe(clocks(time, gen), flags(observedFlags, gui, sprinting),
                             new double[6], new float[2],
                             new float[]{-1.0f, 0.0f, 0.1f});
    }
    public static void main(String[] ignored) throws Exception {
        try (final SprintObservationProbe probe = new SprintObservationProbe()) {
            SprintObservationProbe.Decision first =
                send(probe, 1000, 1, 3, false, true);
            check(first.admitted && first.worldReady && first.sessionTransition,
                  "new world was not admitted");
            check(first.proposedKeyPressed && first.hasSprinting &&
                  first.proposedSprinting, "sprint-key proposal missing");
            check(first.hasSpeed && first.proposeSetSprinting &&
                  Math.abs(first.proposedSpeed - 0.13f) < 0.00001f,
                  "native directional multiplier wrong");
            SprintObservationProbe.Decision gui =
                send(probe, 1001, 1, 3, true, true);
            check(gui.admitted && !gui.proposedKeyPressed &&
                  gui.hasSprinting && !gui.proposedSprinting,
                  "GUI sprint release proposal wrong");

            check(!send(probe, 1000, 1, 3, false, true).admitted,
                  "rollback clock was accepted");
            SprintObservationProbe.Decision absent =
                send(probe, 1002, 2, 0, false, false);
            check(absent.admitted && !absent.worldReady &&
                  !absent.hasSpeed && absent.sessionTransition,
                  "world teardown failed");
            check(!send(probe, 1003, 2, 3, false, true).admitted,
                  "world returned without generation increment");
            SprintObservationProbe.Decision restored =
                send(probe, 1004, 3, 3, true, true);
            check(restored.admitted && !restored.hasSprinting &&
                  !restored.proposedKeyPressed,
                  "new generation retained old key hold");

            boolean shapeRejected = false;
            try {
                probe.observe(clocks(1005, 3), new int[4],
                              new double[6], new float[2], new float[3]);
            } catch (IllegalArgumentException expected) {
                shapeRejected = true;
            }
            check(shapeRejected, "invalid Java arrays not rejected");

            boolean nativeFlagRejected = false;
            try {
                probe.observe(clocks(1005, 3),
                              new int[]{3, 2, 1, 0, 0, 0, 1, 1},
                              new double[6], new float[2], new float[3]);
            } catch (IllegalArgumentException expected) {
                nativeFlagRejected = true;
            }
            check(nativeFlagRejected, "invalid JNI flag not rejected");

            final boolean[] crossThreadDenied = {false};
            Thread other = new Thread(() -> {
                try {
                    send(probe, 1005, 3, 3, false, true);
                } catch (IllegalStateException expected) {
                    crossThreadDenied[0] = true;
                }
            });
            other.start();
            other.join();
            check(crossThreadDenied[0], "cross-thread Java observation admitted");
            check(send(probe, 1005, 3, 3, false, true).admitted,
                  "owner thread did not recover");
        }
        System.out.println("R204_JAVA8_NATIVE_SPRINT_OBSERVATION_ROUNDTRIP_PASS " +
            "session=1 clock=1 wrong_thread=1 zero_game_actions=1");
    }
}
