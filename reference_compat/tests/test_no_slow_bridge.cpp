#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;
static void check(bool value, const char* reason, int line) {
    if (!value)
        throw std::runtime_error("R182 line " + std::to_string(line) + ": " + reason);
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)
static bool close_to(float a, float b) { return std::abs(a-b) < 0.00001f; }

static ExecutionFrame movement(int item_id, bool using_item = true,
                               bool item_present = true) {
    ExecutionFrame frame;
    frame.event = kNoSlowEvent;
    frame.world_present = true;
    frame.player_present = true;
    NoSlowInput input;
    input.item_id = item_id;
    input.using_item = using_item;
    input.held_item_present = item_present;
    input.strafe = 0.1f;
    input.forward = -0.2f;
    input.settings = {1.0f, 0.4f, 0.8f};
    frame.no_slow = NoSlowTickPayload{input, {}, 0};
    return frame;
}

int main() {
    NoSlowController policy;
    NoSlowInput input = movement(261).no_slow->input;
    auto result = policy.evaluate(input);
    CHECK(result.replace_input && close_to(result.strafe, 0.5f));
    CHECK(close_to(result.forward, -1.0f));
    for (int sword_id : {267,268,272,276,283}) {
        input.item_id = sword_id;
        result = policy.evaluate(input);
        CHECK(result.replace_input && close_to(result.strafe, 0.2f));
        CHECK(close_to(result.forward, -0.4f));
    }
    input.item_id = 349;
    result = policy.evaluate(input);
    CHECK(result.replace_input && close_to(result.strafe, 0.4f));
    CHECK(close_to(result.forward, -0.8f));
    input.using_item = false;
    CHECK(!policy.evaluate(input).replace_input);
    input.using_item = true;
    input.held_item_present = false;
    CHECK(!policy.evaluate(input).replace_input);
    input.held_item_present = true;
    input.settings.consumable_multiplier =
        std::numeric_limits<float>::quiet_NaN();
    CHECK(!policy.evaluate(input).replace_input);
    input.settings.consumable_multiplier = -1.0f;
    CHECK(!policy.evaluate(input).replace_input);
    input.settings.consumable_multiplier = 1.0f;
    input.forward = std::numeric_limits<float>::infinity();
    CHECK(!policy.evaluate(input).replace_input);

    ModuleExecutionBridge bridge;
    const auto higher = bridge.add({"No Slow", 1, {{21, 200}}, false});
    const auto observer = bridge.add({"Observer", 2, {{21, 100}}, true});
    const auto unbound = bridge.add({"Unbound", 4, {{21, -10}}, true});
    CHECK(higher == 0 && observer == 1 && unbound == 2);
    CHECK(bridge.bind_no_slow_from_catalog());
    CHECK(!bridge.bind_no_slow(higher));
    unsigned observer_calls = 0;
    CHECK(bridge.bind(observer, kNoSlowEvent,
                      [&](const Delivery& d, ExecutionFrame& frame) {
        CHECK(d.recipient == observer && d.event == 21 && frame.no_slow);
        ++observer_calls;
    }));
    const auto c = bridge.coverage();
    CHECK(c.native_callback_subscriptions == 3);
    CHECK(c.executable_callback_bindings == 2);
    CHECK(c.unimplemented_callback_subscriptions == 1);
    CHECK(c.implemented_per_event[kNoSlowEvent] == 2);

    auto frame = movement(261);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_slow->executions == 0);
    CHECK((frame.executed_modules == std::vector<std::size_t>{observer}));
    CHECK(observer_calls == 1);
    CHECK(bridge.request(higher, true));
    CHECK(!bridge.apply(false, false));
    CHECK(!bridge.enabled(higher));
    CHECK(bridge.apply(true, true));
    frame = movement(261);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_slow->executions == 1);
    CHECK(frame.no_slow->decision.replace_input);
    CHECK(close_to(frame.no_slow->decision.strafe, 0.5f));
    CHECK(close_to(frame.no_slow->decision.forward, -1.0f));
    CHECK((frame.executed_modules == std::vector<std::size_t>{higher, observer}));
    CHECK(observer_calls == 2);

    frame = movement(267);
    CHECK(bridge.dispatch(frame));
    CHECK(close_to(frame.no_slow->decision.strafe, 0.2f));
    frame = movement(260);
    CHECK(bridge.dispatch(frame));
    CHECK(close_to(frame.no_slow->decision.strafe, 0.4f));
    frame = movement(261, false);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_slow->executions == 1);
    CHECK(!frame.no_slow->decision.replace_input);
    frame = movement(261);
    frame.world_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.world_present = true;
    frame.player_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.player_present = true;
    frame.event = 20;
    CHECK(!bridge.dispatch(frame));
    frame.event = 21;
    frame.jump_delay = JumpDelayTickPayload{};
    CHECK(!bridge.dispatch(frame)); // mutually exclusive typed payloads
    frame.jump_delay.reset();

    CHECK(bridge.request(higher, false));
    CHECK(bridge.apply(true, true));
    frame = movement(261);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_slow->executions == 0);
    CHECK((frame.executed_modules == std::vector<std::size_t>{observer}));
    bridge.stop();
    CHECK(!bridge.dispatch(frame));
    CHECK(!bridge.bind_no_slow_from_catalog());

    ModuleExecutionBridge bad_cat;
    auto id = bad_cat.add({"No Slow", 0, {{21, 200}}, true});
    CHECK(!bad_cat.bind_no_slow(id));
    ModuleExecutionBridge bad_event;
    id = bad_event.add({"No Slow", 1, {{20, 200}}, true});
    CHECK(!bad_event.bind_no_slow(id));
    ModuleExecutionBridge bad_priority;
    id = bad_priority.add({"No Slow", 1, {{21, 199}}, true});
    CHECK(!bad_priority.bind_no_slow(id));

    std::cout << "R182 No Slow PASS: bow/sword/consumable scaling, "
                 "fail-closed inputs, event-21 priority and lifecycle\n";
}
