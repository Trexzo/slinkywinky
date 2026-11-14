#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;

static void check(bool passed, const char* expression, int line) {
    if (!passed) throw std::runtime_error(
        "R201 line " + std::to_string(line) + ": " + expression);
}
#define CHECK(x) check(static_cast<bool>(x), #x, __LINE__)

static ExecutionFrame frame(TimerFrameInput input) {
    ExecutionFrame f;
    f.event = kTimerFrameEvent;
    f.world_present = true;
    f.player_present = true;
    f.timer_frame = TimerFramePayload{input, {}, 0};
    return f;
}
static ExecutionFrame toggle(bool matches, bool enabled) {
    ExecutionFrame f;
    f.event = kTimerToggleEvent;
    f.world_present = true;
    f.player_present = true;
    f.timer_toggle = TimerTogglePayload{{matches, enabled}, {}, 0};
    return f;
}
int main() {
    TimerController policy;
    TimerFrameInput in;
    in.in_game = true;
    in.player_present = true;
    in.settings.speed = 1.75f;
    CHECK(policy.on_frame(in).conditions_met);
    CHECK(policy.on_frame(in).proposed_speed == 1.75f);
    in.settings.require_mouse = true;
    CHECK(!policy.on_frame(in).conditions_met);
    CHECK(policy.on_frame(in).proposed_speed == 1.0f);
    in.mouse_pressed = true;
    CHECK(policy.on_frame(in).proposed_speed == 1.75f);
    in.settings.require_falling = true;
    CHECK(policy.on_frame(in).proposed_speed == 1.0f);
    in.on_ground = false;
    in.vertical_motion = 0.0;
    CHECK(!policy.on_frame(in).conditions_met);
    in.vertical_motion = -0.12;
    CHECK(policy.on_frame(in).proposed_speed == 1.75f);
    in.settings.require_damage = true;
    CHECK(policy.on_frame(in).proposed_speed == 1.0f);
    in.hurt_time = 6;
    CHECK(policy.on_frame(in).proposed_speed == 1.75f);
    in.in_game = false;
    CHECK(policy.on_frame(in).proposed_speed == 1.0f);
    in.in_game = true;
    in.player_present = false;
    CHECK(!policy.on_frame(in).conditions_met);
    in.player_present = true;
    in.settings.speed = std::numeric_limits<float>::quiet_NaN();
    CHECK(policy.on_frame(in).proposed_speed == 1.0f);
    in.settings.speed = std::numeric_limits<float>::infinity();
    CHECK(policy.on_frame(in).proposed_speed == 1.0f);
    in.settings.speed = 0.0f;
    CHECK(policy.on_frame(in).proposed_speed == 1.0f);
    in.settings.speed = -3.0f;
    CHECK(policy.on_frame(in).proposed_speed == 1.0f);
    CHECK(!policy.on_toggle({false, false}).proposed_speed);
    CHECK(!policy.on_toggle({true, true}).proposed_speed);
    CHECK(policy.on_toggle({true, false}).proposed_speed == 1.0f);

    ModuleExecutionBridge bridge;
    const std::size_t id = bridge.add(
        {"Timer", 4, {{kTimerFrameEvent, 200}, {kTimerToggleEvent, 0}}, false});
    const std::size_t observer = bridge.add(
        {"Frame Observer", 2, {{kTimerFrameEvent, 250}}, true});
    unsigned observed = 0;
    CHECK(bridge.bind(observer, kTimerFrameEvent,
        [&](const Delivery& d, ExecutionFrame&) {
            CHECK(d.recipient == observer && d.event == kTimerFrameEvent);
            ++observed;
        }));
    CHECK(bridge.bind_timer_from_catalog());
    CHECK(!bridge.bind_timer(id));
    const auto coverage = bridge.coverage();
    CHECK(coverage.registered_modules == 2);
    CHECK(coverage.native_callback_subscriptions == 3);
    CHECK(coverage.executable_callback_bindings == 3);
    CHECK(coverage.modules_with_executable_bindings == 2);
    CHECK(coverage.unimplemented_callback_subscriptions == 0);

    TimerFrameInput input;
    input.in_game = input.player_present = true;
    input.settings.speed = 1.6f;
    auto f = frame(input);
    CHECK(bridge.dispatch(f));
    CHECK(f.timer_frame->executions == 0);
    CHECK((f.executed_modules == std::vector<std::size_t>{observer}));
    CHECK(observed == 1);
    CHECK(bridge.request(id, true));
    CHECK(!bridge.apply(false, true));
    CHECK(!bridge.apply(true, false));
    CHECK(!bridge.enabled(id));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(id));
    f = frame(input);
    CHECK(bridge.dispatch(f));
    CHECK(f.timer_frame->executions == 1);
    CHECK(f.timer_frame->decision.proposed_speed == 1.6f);
    CHECK(f.timer_frame->decision.conditions_met);
    CHECK((f.executed_modules == std::vector<std::size_t>{observer, id}));
    CHECK(observed == 2);

    auto t = toggle(true, true);
    CHECK(bridge.dispatch(t));
    CHECK(t.timer_toggle->executions == 1);
    CHECK(!t.timer_toggle->decision.proposed_speed);
    t = toggle(false, false);
    CHECK(bridge.dispatch(t));
    CHECK(t.timer_toggle->executions == 1);
    CHECK(!t.timer_toggle->decision.proposed_speed);
    t = toggle(true, false);
    CHECK(bridge.dispatch(t));
    CHECK(t.timer_toggle->executions == 1);
    CHECK(t.timer_toggle->decision.proposed_speed == 1.0f);
    CHECK((t.executed_modules == std::vector<std::size_t>{id}));

    // A typed payload must never execute on a different native event.
    f = frame(input);
    f.event = kTimerToggleEvent;
    CHECK(!bridge.dispatch(f));
    t = toggle(true, false);
    t.event = kTimerFrameEvent;
    CHECK(!bridge.dispatch(t));
    f = frame(input);
    f.timer_toggle = TimerTogglePayload{};
    CHECK(!bridge.dispatch(f));
    f = frame(input);
    f.world_present = false;
    CHECK(!bridge.dispatch(f));

    CHECK(bridge.request(id, false));
    CHECK(bridge.apply(true, true));
    CHECK(!bridge.enabled(id));
    t = toggle(true, false);
    CHECK(bridge.dispatch(t));
    CHECK(t.timer_toggle->executions == 0);
    CHECK(!t.timer_toggle->decision.proposed_speed);
    bridge.stop();
    CHECK(!bridge.bind_timer_from_catalog());

    ModuleExecutionBridge invalid_category;
    auto wrong = invalid_category.add(
        {"Timer", 1, {{1, 200}, {5, 0}}, true});
    CHECK(!invalid_category.bind_timer(wrong));
    ModuleExecutionBridge wrong_frame_priority;
    wrong = wrong_frame_priority.add(
        {"Timer", 4, {{1, 100}, {5, 0}}, true});
    CHECK(!wrong_frame_priority.bind_timer(wrong));
    ModuleExecutionBridge wrong_toggle_priority;
    wrong = wrong_toggle_priority.add(
        {"Timer", 4, {{1, 200}, {5, 100}}, true});
    CHECK(!wrong_toggle_priority.bind_timer(wrong));
    ModuleExecutionBridge missing_toggle;
    wrong = missing_toggle.add({"Timer", 4, {{1, 200}}, true});
    CHECK(!missing_toggle.bind_timer(wrong));
    CHECK(missing_toggle.coverage().executable_callback_bindings == 0);

    std::cout << "R201_TIMER_OFFLINE_PASS registered_events=1,5"
                 " priorities=200,0 frame_conditions=3"
                 " disable_reset=1 invalid_metadata_rejected=1\n";
    return 0;
}
