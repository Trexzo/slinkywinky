#include "slinkywinky/module_execution_bridge.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;

static void require(bool ok, const char* what, int line) {
    if (!ok)
        throw std::runtime_error("R203 line " + std::to_string(line) + ": " + what);
}
#define CHECK(x) require(static_cast<bool>(x), #x, __LINE__)

static ExecutionFrame update_frame(const SprintUpdateInput& input) {
    ExecutionFrame frame;
    frame.event = kSprintPlayerUpdateEvent;
    frame.world_present = frame.player_present = true;
    frame.sprint_update = SprintUpdatePayload{input, {}, 0};
    return frame;
}
static ExecutionFrame move_frame(const SprintMovementInput& input) {
    ExecutionFrame frame;
    frame.event = kSprintMovementInputEvent;
    frame.world_present = frame.player_present = true;
    frame.sprint_movement = SprintMovementPayload{input, {}, 0};
    return frame;
}

int main() {
    SprintController policy;
    SprintUpdateInput update;
    auto decision = policy.on_player_update(update);
    CHECK(decision.proposed_key_pressed);
    CHECK(!decision.proposed_player_sprinting);
    CHECK(decision.tracked_forced_state && policy.tracked_forced_state());

    update.gui_open = true;
    decision = policy.on_player_update(update);
    CHECK(!decision.proposed_key_pressed);
    CHECK(decision.proposed_player_sprinting == false);
    CHECK(decision.tracked_forced_state);  // early native return preserves flag
    update.settings.while_in_gui = true;
    update.player_sprinting = true;
    decision = policy.on_player_update(update);
    CHECK(decision.proposed_key_pressed);
    CHECK(decision.proposed_player_sprinting == true);
    update.player_present = false;
    decision = policy.on_player_update(update);
    CHECK(decision.proposed_key_pressed && !decision.proposed_player_sprinting);
    update.gui_open = true;
    update.settings.while_in_gui = false;
    decision = policy.on_player_update(update);
    CHECK(!decision.proposed_key_pressed);
    CHECK(!decision.proposed_player_sprinting);
    CHECK(!policy.tracked_forced_state());

    // The "never forced" menu case must not issue an unowned release.
    SprintController pristine;
    update.player_present = true;
    decision = pristine.on_player_update(update);
    CHECK(!decision.proposed_key_pressed);
    CHECK(!decision.proposed_player_sprinting);
    CHECK(!pristine.tracked_forced_state());
    policy.reset();
    CHECK(!policy.tracked_forced_state());

    SprintMovementInput move;
    move.player_present = true;
    move.speed = 0.1f;
    CHECK(!policy.on_movement_input(move).proposed_speed);
    move.forward = 1.0f;
    CHECK(!policy.on_movement_input(move).proposed_speed); // vanilla forward
    move.using_item = true;
    CHECK(!policy.on_movement_input(move).proposed_speed);
    move.settings.while_using_item = true;
    auto faster = policy.on_movement_input(move);
    CHECK(faster.proposed_speed.has_value() &&
          std::abs(*faster.proposed_speed - 0.13f) < 0.000001f);
    CHECK(faster.propose_set_sprinting);

    move.using_item = false;
    move.forward = -1.0f;
    CHECK(!policy.on_movement_input(move).proposed_speed);
    move.settings.while_backwards = true;
    CHECK(policy.on_movement_input(move).proposed_speed.has_value());
    move.forward = 0.0f;
    move.strafe = 1.0f;
    CHECK(!policy.on_movement_input(move).proposed_speed);
    move.settings.while_sideways = true;
    CHECK(policy.on_movement_input(move).proposed_speed.has_value());
    move.using_item = true;
    CHECK(policy.on_movement_input(move).proposed_speed.has_value());
    move.settings.while_using_item = false;
    CHECK(!policy.on_movement_input(move).proposed_speed);
    move.settings.while_using_item = true;
    move.speed = std::numeric_limits<float>::infinity();
    CHECK(!policy.on_movement_input(move).proposed_speed);
    move.speed = std::numeric_limits<float>::max();
    CHECK(!policy.on_movement_input(move).proposed_speed);
    move.speed = 0.1f;
    move.forward = std::numeric_limits<float>::quiet_NaN();
    CHECK(!policy.on_movement_input(move).proposed_speed);
    move.forward = 0.0f;
    move.player_present = false;
    CHECK(!policy.on_movement_input(move).proposed_speed);

    ModuleExecutionBridge bridge;
    const auto jump = bridge.add({"No Jump Delay", 1, {{17, 100}}, true});
    const auto keep = bridge.add(
        {"Keep Sprint", 1, {{14, 0}, {15, 0}, {17, 0}}, true});
    const auto no_slow = bridge.add({"No Slow", 1, {{21, 200}}, true});
    const auto sprint = bridge.add({"Sprint", 1, {{17, 100}, {21, 100}}, false});
    CHECK(bridge.bind_no_jump_delay(jump));
    CHECK(bridge.bind_keep_sprint(keep));
    CHECK(bridge.bind_no_slow(no_slow));
    CHECK(bridge.bind_sprint_from_catalog());
    CHECK(!bridge.bind_sprint(sprint));
    const auto coverage = bridge.coverage();
    CHECK(coverage.registered_modules == 4);
    CHECK(coverage.native_callback_subscriptions == 7);
    CHECK(coverage.executable_callback_bindings == 7);
    CHECK(coverage.modules_with_executable_bindings == 4);
    CHECK(coverage.unimplemented_callback_subscriptions == 0);

    update = SprintUpdateInput{};
    update.gui_open = false;
    update.player_present = true;
    auto tick = update_frame(update);
    CHECK(bridge.dispatch(tick) && tick.sprint_update &&
          tick.sprint_update->executions == 0);
    CHECK(bridge.request(sprint, true));
    CHECK(!bridge.apply(true, false));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(sprint));

    // Event17: all three original reference policies can be present.
    tick = update_frame(update);
    tick.jump_delay = JumpDelayTickPayload{};
    tick.keep_sprint_update = KeepSprintUpdatePayload{{true}, {}, 0};
    CHECK(bridge.dispatch(tick));
    CHECK(tick.sprint_update->executions == 1);
    CHECK(tick.sprint_update->decision.proposed_key_pressed);
    CHECK(tick.jump_delay->executions == 1);
    CHECK(tick.keep_sprint_update->executions == 1);
    CHECK(tick.executed_modules.size() == 3);

    tick = update_frame(update);
    tick.keep_sprint_update = KeepSprintUpdatePayload{{false}, {}, 0};
    CHECK(!bridge.dispatch(tick)); // conflicting player presence
    tick = update_frame(update);
    tick.event = kSprintMovementInputEvent;
    CHECK(!bridge.dispatch(tick)); // wrong typed event

    SprintMovementInput shared;
    shared.forward = 0.0f;
    shared.strafe = 1.0f;
    shared.speed = 0.1f;
    shared.using_item = true;
    shared.settings.while_using_item = true;
    shared.settings.while_sideways = true;
    auto movement = move_frame(shared);
    movement.no_slow = NoSlowTickPayload{};
    movement.no_slow->input.forward = shared.forward;
    movement.no_slow->input.strafe = shared.strafe;
    movement.no_slow->input.using_item = shared.using_item;
    movement.no_slow->input.held_item_present = true;
    CHECK(bridge.dispatch(movement));
    CHECK(movement.sprint_movement->executions == 1);
    CHECK(movement.sprint_movement->decision.propose_set_sprinting);
    CHECK(movement.sprint_movement->decision.proposed_speed.has_value());
    CHECK(movement.no_slow->executions == 1);
    CHECK(movement.executed_modules.size() == 2);

    movement = move_frame(shared);
    movement.no_slow = NoSlowTickPayload{};
    movement.no_slow->input.using_item = false;
    CHECK(!bridge.dispatch(movement)); // same event but inconsistent input
    movement = move_frame(shared);
    movement.keep_sprint_update = KeepSprintUpdatePayload{};
    CHECK(!bridge.dispatch(movement)); // unrelated event17 payload
    movement = move_frame(shared);
    movement.world_present = false;
    CHECK(!bridge.dispatch(movement));
    movement = move_frame(shared);
    movement.event = 0;
    CHECK(!bridge.dispatch(movement));

    bridge.reset_sprint_world_state();
    tick = update_frame(SprintUpdateInput{true, true, false, {}});
    CHECK(bridge.dispatch(tick));
    CHECK(!tick.sprint_update->decision.proposed_key_pressed);
    CHECK(!tick.sprint_update->decision.proposed_player_sprinting);
    CHECK(!tick.sprint_update->decision.tracked_forced_state);
    CHECK(bridge.request(sprint, false));
    CHECK(bridge.apply(true, true));
    CHECK(!bridge.enabled(sprint));
    tick = update_frame(update);
    CHECK(bridge.dispatch(tick));
    CHECK(tick.sprint_update->executions == 0);
    bridge.stop();
    CHECK(!bridge.bind_sprint_from_catalog());

    for (unsigned mutation = 0; mutation < 5; ++mutation) {
        ModuleExecutionBridge invalid;
        ModuleSpec spec{"Sprint", mutation == 0 ? 2u : 1u,
                        {{17, 100}, {21, 100}}, true};
        if (mutation == 1) spec.subscriptions[0].priority = 0;
        if (mutation == 2) spec.subscriptions[1].priority = 200;
        if (mutation == 3) spec.subscriptions.pop_back();
        if (mutation == 4) spec.name = "Sprint Variant";
        const auto id = invalid.add(spec);
        CHECK(!invalid.bind_sprint(id));
        CHECK(invalid.coverage().executable_callback_bindings == 0);
    }

    std::cout << "R203_SPRINT_OFFLINE_PASS slots=17,21 priorities=100,100"
              << " shared_event17=3 shared_event21=2 negatives=5\n";
}
