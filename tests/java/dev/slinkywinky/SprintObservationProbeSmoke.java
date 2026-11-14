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
        // R210: GUI-facing request -> JNI -> native owner state ->
        // world-admitted callback -> module-state snapshot. This is the
        // integration chain; no simulated GUI demo flag is involved.
        try (final SprintObservationProbe control = new SprintObservationProbe()) {
            SprintObservationProbe.ControlState initial = control.state();
            check(initial.requestedEnabled && !initial.activeInWorld &&
                  initial.worldGeneration == 0 && !initial.settingsOverride,
                  "initial module state contract wrong");

            control.setEnabled(false);
            SprintObservationProbe.ControlState disabled = control.state();
            check(!disabled.requestedEnabled && !disabled.activeInWorld,
                  "disabling before a world was not retained");
            SprintObservationProbe.Decision noOp =
                send(control, 4000, 30, 3, false, true);
            check(noOp.admitted && noOp.worldReady &&
                  !noOp.proposedKeyPressed && !noOp.hasSprinting &&
                  !noOp.hasSpeed, "disabled Sprint produced a gameplay proposal");
            check(!control.state().activeInWorld && control.state().worldGeneration == 30,
                  "disabled world registered an active module");

            control.setEnabled(true);
            check(control.state().activeInWorld,
                  "module was not enabled in already-admitted world");
            control.setSettings(false, false, false, false);
            SprintObservationProbe.Decision noBackward =
                send(control, 4001, 30, 3, false, true);
            check(noBackward.admitted && noBackward.proposedKeyPressed &&
                  !noBackward.hasSpeed && control.state().settingsOverride,
                  "explicit settings did not suppress backward speed");
            control.setSettings(false, false, true, false);
            SprintObservationProbe.Decision yesBackward =
                send(control, 4002, 30, 3, false, true);
            check(yesBackward.admitted && yesBackward.hasSpeed &&
                  Math.abs(yesBackward.proposedSpeed - 0.13f) < 0.00001f,
                  "explicit backwards setting did not reach native policy");

            final boolean[] foreignRejected = {false};
            Thread other = new Thread(() -> {
                try { control.setEnabled(false); }
                catch (IllegalStateException expected) { foreignRejected[0] = true; }
            });
            other.start();
            other.join();
            check(foreignRejected[0] && control.state().requestedEnabled,
                  "cross-thread control changed module state");

            control.setEnabled(false);
            noOp = send(control, 4003, 30, 3, false, true);
            check(noOp.admitted && !noOp.hasSpeed && !noOp.proposedKeyPressed,
                  "module continued dispatching after disable");
            SprintObservationProbe.Decision absent =
                send(control, 4004, 31, 0, false, false);
            check(absent.admitted && !control.state().activeInWorld &&
                  !control.state().requestedEnabled,
                  "disabled choice was lost during disconnect");
            control.setEnabled(true);
            check(control.state().requestedEnabled && !control.state().activeInWorld,
                  "no-world enable incorrectly activated an event subscription");
            SprintObservationProbe.Decision returned =
                send(control, 4005, 32, 3, false, true);
            check(returned.admitted && returned.sessionTransition &&
                  returned.hasSpeed && control.state().activeInWorld &&
                  control.state().worldGeneration == 32,
                  "world re-entry failed to preserve selection and settings");
        }
        System.out.println("R210_JAVA8_MODULE_CONTROL_INTEGRATION_PASS " +
            "toggle=1 settings=1 state=1 world_transition=1 wrong_thread=1 " +
            "gameplay_writes=0");

        // R211: real Java8 -> JNI -> same native owner bridge with three
        // independent toggles and typed event17/21 data-only proposals.
        try (SprintObservationProbe session = new SprintObservationProbe()) {
            SprintObservationProbe.ControlState[] initial = session.moduleStates();
            check(initial.length == 3 &&
                  SprintObservationProbe.NO_JUMP_DELAY.equals(initial[0].moduleName) &&
                  SprintObservationProbe.NO_SLOW.equals(initial[1].moduleName) &&
                  SprintObservationProbe.MODULE_NAME.equals(initial[2].moduleName),
                  "canonical module-state snapshot order");
            check(!initial[0].requestedEnabled && !initial[1].requestedEnabled &&
                  initial[2].requestedEnabled && !initial[2].activeInWorld,
                  "initial native selection state incorrect");
            session.setModuleEnabled(SprintObservationProbe.NO_JUMP_DELAY, true);
            session.setModuleEnabled(SprintObservationProbe.NO_SLOW, true);
            session.setNoSlowSettings(0.8f, 0.4f, 0.6f);
            final long[] clock = {6000, 40};
            final int[] inWorldFlags = {3, 0, 1, 1, 0, 0, 1, 1};
            final int[] item = {1, 10, 1, 1, 261};
            final float[] movement = {-0.2f, 0.1f, 0.1f};
            final float[] passive = {1.0f, 1.0f, 1.0f};
            SprintObservationProbe.ModuleDecision active =
                session.observeModules(clock, inWorldFlags, new double[6],
                                       new float[2], movement, item, passive);
            check(active.sprint.admitted && active.sprint.worldReady &&
                  active.hasJumpTicks && active.proposedJumpTicks == 0 &&
                  active.noSlowEvaluated && active.replaceInput &&
                  Math.abs(active.proposedStrafe - 0.4f) < 0.00001f &&
                  Math.abs(active.proposedForward + 0.8f) < 0.00001f,
                  "shared event17/event21 module decisions not routed");
            check(session.moduleState(SprintObservationProbe.NO_JUMP_DELAY).activeInWorld &&
                  session.moduleState(SprintObservationProbe.NO_SLOW).activeInWorld &&
                  session.moduleState("No Slow").settingsOverride,
                  "native module activation/settings acknowledgement missing");

            session.setModuleEnabled(SprintObservationProbe.NO_SLOW, false);
            clock[0] = 6001;
            active = session.observeModules(clock, inWorldFlags, new double[6],
                                            new float[2], movement, item, passive);
            check(active.hasJumpTicks && !active.noSlowEvaluated &&
                  !session.moduleState("No Slow").activeInWorld,
                  "No Slow disable did not preserve independent Jump state");
            session.setModuleEnabled(SprintObservationProbe.MODULE_NAME, false);
            clock[0] = 6002;
            active = session.observeModules(clock, inWorldFlags, new double[6],
                                            new float[2], movement, item, passive);
            check(active.hasJumpTicks && !active.sprint.proposedKeyPressed,
                  "Jump callback was lost or disabled Sprint still proposed");

            session.setModuleEnabled(SprintObservationProbe.NO_SLOW, true);
            session.setNoSlowSettings(0.5f, 0.4f, 0.6f);
            clock[0] = 6003;
            active = session.observeModules(clock, inWorldFlags, new double[6],
                                            new float[2], movement, item, passive);
            check(active.noSlowEvaluated && active.replaceInput &&
                  Math.abs(active.proposedStrafe - 0.25f) < 0.00001f &&
                  Math.abs(active.proposedForward + 0.5f) < 0.00001f,
                  "live No Slow settings update not reflected in native decisions");

            boolean invalidContext = false;
            try {
                session.observeModules(new long[]{6004,40}, inWorldFlags,
                    new double[6], new float[2], movement,
                    new int[]{1,10,0,1,261}, passive);
            } catch (IllegalArgumentException expected) {
                invalidContext = true;
            }
            check(invalidContext, "contradictory item-use flags admitted");
            boolean invalidName = false;
            try { session.setModuleEnabled("Fake Module", true); }
            catch (IllegalArgumentException expected) { invalidName = true; }
            check(invalidName, "invalid module name admitted");
            boolean invalidMultiplier = false;
            try { session.setNoSlowSettings(Float.NaN, 1.0f, 1.0f); }
            catch (IllegalArgumentException expected) { invalidMultiplier = true; }
            check(invalidMultiplier, "nonfinite No Slow multiplier admitted");
            final boolean[] foreignDenied = {false};
            Thread foreign = new Thread(() -> {
                try { session.setModuleEnabled("No Slow", false); }
                catch (IllegalStateException expected) { foreignDenied[0] = true; }
            });
            foreign.start();
            foreign.join();
            check(foreignDenied[0] && session.moduleState("No Slow").activeInWorld,
                  "foreign thread mutated module state");

            active = session.observeModules(
                new long[]{6004,41}, new int[8],
                new double[6], new float[2], new float[3],
                new int[]{0,0,0,0,-1}, passive);
            check(active.sprint.admitted && !active.sprint.worldReady &&
                  !session.moduleState("No Slow").activeInWorld &&
                  session.moduleState("No Slow").requestedEnabled,
                  "world exit lost requested selection or retained active handler");
            active = session.observeModules(
                new long[]{6005,42}, inWorldFlags,
                new double[6], new float[2], movement, item, passive);
            check(active.sprint.admitted && active.sprint.sessionTransition &&
                  active.hasJumpTicks && active.noSlowEvaluated &&
                  session.moduleState("No Slow").activeInWorld &&
                  !session.moduleState("Sprint").activeInWorld,
                  "world reentry did not restore exact three-module selection");
        }
        System.out.println("R211_JAVA8_THREE_MODULE_CONTROL_PASS " +
            "toggles=3 native_callbacks=4 typed_events=17,21 " +
            "settings=1 world_reentry=1 wrong_thread=1 actions=0");

        System.out.println("R204_JAVA8_NATIVE_SPRINT_OBSERVATION_ROUNDTRIP_PASS " +
            "session=1 clock=1 wrong_thread=1 zero_game_actions=1");
    }
}
