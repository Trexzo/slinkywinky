#include "slinkywinky/module_execution_bridge.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;
static void require(bool check, const char* reason, int line) {
    if (!check) throw std::runtime_error(
        "R183 line " + std::to_string(line) + ": " + reason);
}
#define CHECK(expr) require(static_cast<bool>(expr), #expr, __LINE__)

static ExecutionFrame attack(unsigned mode = 0, bool target_present = false,
                             int target_type = 0, bool using_item = false) {
    ExecutionFrame frame;
    frame.event = kNoHitDelayEvent;
    frame.world_present = true;
    frame.player_present = true;
    NoHitDelayInput input;
    input.attack_click = true;
    input.jvm_ready = true;
    input.left_click_counter = 10;
    input.mode = mode;
    input.target_present = target_present;
    input.target_type_ordinal = target_type;
    input.player_using_item = using_item;
    frame.no_hit_delay = NoHitDelayTickPayload{input, {}, 0};
    return frame;
}

int main() {
    NoHitDelayController decision;
    auto input = attack().no_hit_delay->input;
    auto result = decision.evaluate(input);
    CHECK(result.clear_click_counter && !result.swing_arm);
    input.attack_click = false;
    CHECK(!decision.evaluate(input).clear_click_counter);
    CHECK(!decision.evaluate(input).swing_arm);
    input.attack_click = true;
    input.jvm_ready = false;
    CHECK(!decision.evaluate(input).clear_click_counter);
    input.jvm_ready = true;
    input.left_click_counter = 0;
    CHECK(!decision.evaluate(input).clear_click_counter);
    input.left_click_counter = -1;
    CHECK(!decision.evaluate(input).clear_click_counter);
    input.left_click_counter = 10;
    input.mode = 1;
    input.target_present = false;
    result = decision.evaluate(input);
    CHECK(!result.clear_click_counter && result.swing_arm);
    input.player_using_item = true;
    result = decision.evaluate(input);
    CHECK(!result.clear_click_counter && !result.swing_arm);
    input.player_using_item = false;
    input.target_present = true;
    input.target_type_ordinal = 0;
    result = decision.evaluate(input);
    CHECK(!result.clear_click_counter && result.swing_arm);
    input.target_type_ordinal = 1;
    result = decision.evaluate(input);
    CHECK(result.clear_click_counter && !result.swing_arm);
    input.mode = 99; // original branch only distinguishes mode == 1
    input.target_present = false;
    result = decision.evaluate(input);
    CHECK(result.clear_click_counter && !result.swing_arm);

    // Event 14 callback priority and module gating. Hit Select may also
    // register event 14 but is deliberately left unimplemented.
    ModuleExecutionBridge bridge;
    const auto no_hit = bridge.add({"No Hit Delay", 4, {{14, 0}}, false});
    const auto observer = bridge.add({"Higher Observer", 2, {{14, 100}}, true});
    const auto unimplemented = bridge.add({"Hit Select", 0, {{14, -100}}, true});
    CHECK(no_hit == 0 && observer == 1 && unimplemented == 2);
    unsigned observer_calls = 0;
    CHECK(bridge.bind(observer, kNoHitDelayEvent, [&](const Delivery& d, ExecutionFrame&) {
        CHECK(d.recipient == observer && d.event == 14);
        ++observer_calls;
    }));
    CHECK(bridge.bind_no_hit_delay_from_catalog());
    CHECK(!bridge.bind_no_hit_delay(no_hit));
    const auto coverage = bridge.coverage();
    CHECK(coverage.registered_modules == 3);
    CHECK(coverage.native_callback_subscriptions == 3);
    CHECK(coverage.executable_callback_bindings == 2);
    CHECK(coverage.unimplemented_callback_subscriptions == 1);
    CHECK(coverage.implemented_per_event[14] == 2);

    auto frame = attack();
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_hit_delay->executions == 0);
    CHECK(!frame.no_hit_delay->decision.clear_click_counter);
    CHECK((frame.executed_modules == std::vector<std::size_t>{observer}));
    CHECK(observer_calls == 1);
    CHECK(bridge.request(no_hit, true));
    CHECK(!bridge.apply(false, false));
    CHECK(!bridge.apply(true, false));
    CHECK(!bridge.enabled(no_hit));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(no_hit));

    frame = attack();
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_hit_delay->executions == 1);
    CHECK(frame.no_hit_delay->decision.clear_click_counter);
    CHECK(!frame.no_hit_delay->decision.swing_arm);
    CHECK((frame.executed_modules == std::vector<std::size_t>{observer, no_hit}));

    frame = attack(1, false);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_hit_delay->executions == 1);
    CHECK(frame.no_hit_delay->decision.swing_arm);
    CHECK(!frame.no_hit_delay->decision.clear_click_counter);
    frame = attack(1, false, 0, true);
    CHECK(bridge.dispatch(frame));
    CHECK(!frame.no_hit_delay->decision.swing_arm);
    CHECK(!frame.no_hit_delay->decision.clear_click_counter);
    frame = attack(1, true, 1);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_hit_delay->decision.clear_click_counter);

    frame = attack();
    frame.world_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.world_present = true;
    frame.player_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.player_present = true;
    frame.event = 13;
    CHECK(!bridge.dispatch(frame));
    frame.event = 14;
    frame.no_slow = NoSlowTickPayload{};
    CHECK(!bridge.dispatch(frame)); // contradictory event envelopes
    frame.no_slow.reset();

    CHECK(bridge.request(no_hit, false));
    CHECK(bridge.apply(true, true));
    frame = attack();
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_hit_delay->executions == 0);
    CHECK((frame.executed_modules == std::vector<std::size_t>{observer}));
    bridge.stop();
    CHECK(!bridge.dispatch(frame));
    CHECK(!bridge.request(no_hit, true));
    CHECK(!bridge.bind_no_hit_delay_from_catalog());

    ModuleExecutionBridge bad_category;
    auto id = bad_category.add({"No Hit Delay", 0, {{14, 0}}, true});
    CHECK(!bad_category.bind_no_hit_delay(id));
    ModuleExecutionBridge bad_event;
    id = bad_event.add({"No Hit Delay", 4, {{13, 0}}, true});
    CHECK(!bad_event.bind_no_hit_delay(id));
    ModuleExecutionBridge bad_priority;
    id = bad_priority.add({"No Hit Delay", 4, {{14, 1}}, true});
    CHECK(!bad_priority.bind_no_hit_delay(id));

    std::cout << "R183 No Hit Delay PASS: both modes, misses, item-use swing "
                 "gate, click counter, event-14 priority and shutdown\n";
}
