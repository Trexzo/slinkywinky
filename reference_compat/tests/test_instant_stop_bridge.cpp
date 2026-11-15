#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;

static void check(bool value, const char* expression, int line) {
    if (!value) throw std::runtime_error(
        "R181 line " + std::to_string(line) + ": " + expression);
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

static InstantStopInput instant(bool forward, bool ground, double x, double z,
                                 bool in_game = true) {
    InstantStopInput in;
    in.in_game = in_game;
    in.forward_pressed = forward;
    in.on_ground = ground;
    in.motion_x = x;
    in.motion_z = z;
    in.yaw_degrees = 0.0f;
    in.land_movement_factor = 0.1f;
    in.physically_held = {forward, false, false, false};
    return in;
}

static ExecutionFrame movement(int tick, bool forward, bool ground,
                                double x, double z) {
    ExecutionFrame f;
    f.event = kMovementTickEvent;
    f.world_present = true;
    f.player_present = true;
    auto value = instant(forward, ground, x, z);
    f.instant_stop = InstantStopTickPayload{value, {}, 0};
    NullMoveInput null_input;
    null_input.in_game = true;
    null_input.ticks_existed = tick;
    null_input.game_pressed = {forward, false, false, false};
    null_input.physically_held = value.physically_held;
    f.null_move = NullMoveTickPayload{null_input, {}, 0};
    return f;
}

int main() {
    InstantStopController pure;
    auto step = pure.evaluate(instant(true, true, 0.0, 0.10));
    CHECK(step.phase == InstantStopPhase::watching_forward);
    CHECK(step.choice == InstantStopChoice::none);
    step = pure.evaluate(instant(true, true, 0.0, 0.10));
    CHECK(step.phase == InstantStopPhase::watching_forward);
    step = pure.evaluate(instant(false, false, 0.0, 0.10));
    CHECK(step.phase == InstantStopPhase::watching_forward);
    step = pure.evaluate(instant(false, true, 0.0, 0.10));
    CHECK(step.phase == InstantStopPhase::counter_active);
    CHECK(step.choice == InstantStopChoice::back_only);
    CHECK(step.overrides[static_cast<unsigned>(MoveKey::back)] == true);
    CHECK(step.overrides[static_cast<unsigned>(MoveKey::left)] == false);
    CHECK(step.overrides[static_cast<unsigned>(MoveKey::right)] == false);
    CHECK(step.predicted_speed_squared < 0.001);

    auto restore = instant(false, true, 0.0, 0.0);
    restore.physically_held = {false, true, true, false};
    step = pure.evaluate(restore);
    CHECK(step.phase == InstantStopPhase::idle);
    CHECK(step.overrides[static_cast<unsigned>(MoveKey::back)] == true);
    CHECK(step.overrides[static_cast<unsigned>(MoveKey::left)] == true);
    CHECK(step.overrides[static_cast<unsigned>(MoveKey::right)] == false);

    // Three physical counter-input choices are explicitly distinguished.
    pure.evaluate(instant(true, true, 0.07, 0.07));
    step = pure.evaluate(instant(false, true, 0.07, 0.07));
    CHECK(step.choice == InstantStopChoice::back_right);
    CHECK(step.overrides[static_cast<unsigned>(MoveKey::right)] == true);
    CHECK(step.overrides[static_cast<unsigned>(MoveKey::left)] == false);
    pure.reset();
    pure.evaluate(instant(true, true, -0.07, 0.07));
    step = pure.evaluate(instant(false, true, -0.07, 0.07));
    CHECK(step.choice == InstantStopChoice::back_left);
    CHECK(step.overrides[static_cast<unsigned>(MoveKey::left)] == true);
    CHECK(step.overrides[static_cast<unsigned>(MoveKey::right)] == false);

    pure.reset();
    pure.evaluate(instant(true, true, 0.0, 0.0));
    step = pure.evaluate(instant(false, true, 0.0, 0.0));
    CHECK(step.choice == InstantStopChoice::none);
    CHECK(step.phase == InstantStopPhase::idle);
    pure.evaluate(instant(true, true, 0.0, 0.1));
    auto invalid = instant(false, true, std::numeric_limits<double>::quiet_NaN(), 0.1);
    step = pure.evaluate(invalid);
    CHECK(step.choice == InstantStopChoice::none);
    CHECK(step.phase == InstantStopPhase::idle);
    pure.evaluate(instant(true, true, 0.0, 0.1));
    auto gone = instant(false, true, 0.0, 0.0, false);
    step = pure.evaluate(gone);
    CHECK(step.phase == InstantStopPhase::idle);

    // Event 16 includes two policies. Priority 200 must be invoked before
    // priority 100, with its output visible to downstream Null Move.
    ModuleExecutionBridge bridge;
    const auto lower = bridge.add({"Null Move", 1, {{16, 100}}, false});
    const auto higher = bridge.add({"Instant Stop", 1, {{16, 200}}, false});
    const auto unused = bridge.add({"Unimplemented", 1, {{16, -20}}, true});
    CHECK(lower == 0 && higher == 1 && unused == 2);
    CHECK(bridge.bind_null_move_from_catalog());
    CHECK(bridge.bind_instant_stop_from_catalog());
    CHECK(!bridge.bind_instant_stop(higher)); // no double bind
    CHECK(bridge.coverage().native_callback_subscriptions == 3);
    CHECK(bridge.coverage().executable_callback_bindings == 2);
    CHECK(bridge.coverage().unimplemented_callback_subscriptions == 1);
    CHECK(bridge.request(higher, true));
    CHECK(bridge.request(lower, true));
    CHECK(!bridge.apply(false, false));
    CHECK(bridge.apply(true, true));

    auto frame = movement(100, true, true, 0.0, 0.1);
    CHECK(bridge.dispatch(frame));
    CHECK((frame.executed_modules == std::vector<std::size_t>{higher, lower}));
    CHECK(frame.instant_stop->executions == 1);
    CHECK(frame.instant_stop->decision.phase == InstantStopPhase::watching_forward);
    CHECK(frame.null_move->executions == 1);
    CHECK(frame.null_move->input.game_pressed[0]);

    frame = movement(101, false, true, 0.0, 0.10);
    CHECK(bridge.dispatch(frame));
    CHECK((frame.executed_modules == std::vector<std::size_t>{higher, lower}));
    CHECK(frame.instant_stop->decision.choice == InstantStopChoice::back_only);
    CHECK(frame.null_move->input.game_pressed[static_cast<unsigned>(MoveKey::back)]);
    CHECK(frame.null_move->decision.resulting_game_pressed[static_cast<unsigned>(MoveKey::back)]);
    CHECK(frame.null_move->executions == 1);

    frame = movement(102, false, true, 0.0, 0.0);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.instant_stop->decision.phase == InstantStopPhase::idle);
    CHECK(frame.instant_stop->decision.overrides[static_cast<unsigned>(MoveKey::back)] == false);
    CHECK(!frame.null_move->input.game_pressed[static_cast<unsigned>(MoveKey::back)]);

    // Both typed movement inputs must agree about the same game frame.
    frame = movement(103, true, true, 0.0, 0.1);
    frame.null_move->input.physically_held[2] = true;
    CHECK(!bridge.dispatch(frame));
    frame.null_move->input.physically_held[2] = false;
    frame.null_move->input.game_pressed[0] = false;
    CHECK(!bridge.dispatch(frame));
    frame.null_move->input.game_pressed[0] = true;
    frame.reach = ReachQueryPayload{};
    CHECK(!bridge.dispatch(frame));
    frame.reach.reset();
    frame.event = 17;
    CHECK(!bridge.dispatch(frame));
    frame.event = 16;
    frame.world_present = false;
    CHECK(!bridge.dispatch(frame));

    // Disabling Instant Stop preserves Null Move but prevents counter writes.
    CHECK(bridge.request(higher, false));
    CHECK(bridge.apply(true, true));
    frame = movement(110, false, true, 0.0, 0.1);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.instant_stop->executions == 0);
    CHECK(frame.null_move->executions == 1);
    CHECK((frame.executed_modules == std::vector<std::size_t>{lower}));
    CHECK(!frame.null_move->input.game_pressed[static_cast<unsigned>(MoveKey::back)]);

    CHECK(bridge.request(higher, true));
    CHECK(bridge.apply(true, true));
    bridge.reset_instant_stop_world_state();
    bridge.reset_null_move_world_state();
    frame = movement(120, true, true, 0.0, 0.1);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.instant_stop->decision.phase == InstantStopPhase::watching_forward);
    bridge.stop();
    frame = movement(121, false, true, 0.0, 0.1);
    CHECK(!bridge.dispatch(frame));
    CHECK(!bridge.bind_instant_stop_from_catalog());

    ModuleExecutionBridge bad;
    const auto wrong_category = bad.add({"Instant Stop", 0, {{16, 200}}, true});
    CHECK(!bad.bind_instant_stop(wrong_category));
    ModuleExecutionBridge bad_event;
    const auto wrong_slot = bad_event.add({"Instant Stop", 1, {{17, 200}}, true});
    CHECK(!bad_event.bind_instant_stop(wrong_slot));

    std::cout << "R181 Instant Stop PASS: 3 counter-input choices, single-tick "
                 "restoration, priority 200 before Null Move 100, typed event "
                 "agreement, fail-closed lifecycle and shutdown\n";
}
