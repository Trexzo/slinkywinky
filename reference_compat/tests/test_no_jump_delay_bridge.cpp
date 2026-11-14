#include "slinkywinky/module_execution_bridge.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;

static void verify(bool value, const char* description, int line) {
    if (!value) throw std::runtime_error(
        "R179 line " + std::to_string(line) + ": " + description);
}
#define CHECK(x) verify(static_cast<bool>(x), #x, __LINE__)

static ExecutionFrame player_tick(bool in_game, int previous_jump_ticks = 10) {
    ExecutionFrame frame;
    frame.event = kPlayerUpdateEvent;
    frame.world_present = true;
    frame.player_present = true;
    JumpDelayTickPayload payload;
    payload.input = {in_game, previous_jump_ticks};
    frame.jump_delay = payload;
    return frame;
}

int main() {
    NoJumpDelayController policy;
    const auto before_game = policy.evaluate({false, 10});
    CHECK(!before_game.override_jump_ticks);
    const auto in_game = policy.evaluate({true, 10});
    CHECK(in_game.override_jump_ticks && in_game.proposed_jump_ticks == 0);
    const auto negative_ticks = policy.evaluate({true, -7});
    CHECK(negative_ticks.override_jump_ticks && negative_ticks.proposed_jump_ticks == 0);

    ModuleExecutionBridge bridge;
    const auto observer = bridge.add({"Synthetic Observer", 2, {{17, 200}}, true});
    const auto jump = bridge.add({"No Jump Delay", 1, {{17, 100}}, false});
    const auto reach = bridge.add({"Reach", 0, {{8, 0}}, false});
    const auto unbound = bridge.add({"Other", 4, {{17, -100}}, true});
    CHECK(observer == 0 && jump == 1 && reach == 2 && unbound == 3);
    unsigned observer_calls = 0;
    CHECK(bridge.bind(observer, 17, [&](const Delivery& d, ExecutionFrame& frame) {
        CHECK(d.event == 17 && d.recipient == observer);
        CHECK(frame.event == 17);
        ++observer_calls;
    }));
    CHECK(bridge.bind_no_jump_delay_from_catalog());
    CHECK(!bridge.bind_no_jump_delay(jump));
    CHECK(bridge.bind_reach_from_catalog());
    const auto coverage = bridge.coverage();
    CHECK(coverage.registered_modules == 4);
    CHECK(coverage.native_callback_subscriptions == 4);
    CHECK(coverage.executable_callback_bindings == 3);
    CHECK(coverage.unimplemented_callback_subscriptions == 1);
    CHECK(coverage.implemented_per_event[17] == 2);
    CHECK(coverage.implemented_per_event[8] == 1);

    auto frame = player_tick(true);
    frame.world_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.world_present = true;
    frame.player_present = false;
    CHECK(!bridge.dispatch(frame));
    CHECK(observer_calls == 0);
    CHECK(bridge.request(jump, true));
    CHECK(!bridge.apply(false, false));
    CHECK(!bridge.enabled(jump));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(jump));

    frame = player_tick(true, 10);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.jump_delay->executions == 1);
    CHECK(frame.jump_delay->proposed_jump_ticks.has_value());
    CHECK(*frame.jump_delay->proposed_jump_ticks == 0);
    CHECK((frame.executed_modules == std::vector<std::size_t>{observer, jump}));
    CHECK(observer_calls == 1);

    frame = player_tick(false);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.jump_delay->executions == 1);
    CHECK(!frame.jump_delay->proposed_jump_ticks);
    CHECK((frame.executed_modules == std::vector<std::size_t>{observer, jump}));

    frame = player_tick(true, -7);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.jump_delay->proposed_jump_ticks == 0);
    CHECK(frame.jump_delay->executions == 1);

    frame = player_tick(true);
    frame.event = 8;
    CHECK(!bridge.dispatch(frame)); // mismatched typed event
    frame.event = 27;
    CHECK(!bridge.dispatch(frame));
    frame.event = 17;
    frame.reach = ReachQueryPayload{};
    CHECK(!bridge.dispatch(frame)); // two conflicting typed payloads

    // Reach remains independent and uses event 8, not the movement tick.
    ExecutionFrame reach_frame;
    reach_frame.event = 8;
    reach_frame.world_present = true;
    reach_frame.player_present = true;
    ReachQueryPayload reach_payload;
    reach_payload.input.sprinting = true;
    reach_payload.settings.distance = 3.1f;
    reach_payload.settings.activation_ticks = 3.0f;
    reach_frame.reach = reach_payload;
    CHECK(bridge.request(reach, true));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.dispatch(reach_frame));
    CHECK(reach_frame.reach->executions == 1);
    CHECK(reach_frame.reach->proposed_distance == 3.1f);
    CHECK((reach_frame.executed_modules == std::vector<std::size_t>{reach}));

    // Disabling the movement module leaves other player-tick bindings alive.
    CHECK(bridge.request(jump, false));
    CHECK(bridge.apply(true, true));
    CHECK(!bridge.enabled(jump));
    frame = player_tick(true);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.jump_delay->executions == 0);
    CHECK(!frame.jump_delay->proposed_jump_ticks);
    CHECK((frame.executed_modules == std::vector<std::size_t>{observer}));

    bridge.stop();
    frame = player_tick(true);
    CHECK(!bridge.dispatch(frame));
    CHECK(!bridge.request(jump, true));
    CHECK(!bridge.bind_no_jump_delay_from_catalog());

    ModuleExecutionBridge wrong;
    const auto wrong_category = wrong.add({"No Jump Delay", 0, {{17, 100}}, true});
    const auto wrong_slot = wrong.add({"Wrong Slot", 1, {{16, 100}}, true});
    CHECK(!wrong.bind_no_jump_delay(wrong_category));
    CHECK(!wrong.bind_no_jump_delay(wrong_slot));
    CHECK(!wrong.bind_no_jump_delay_from_catalog());

    std::cout << "R179 No Jump Delay PASS: native candidate movement "
                 "event=17 priority=100, pure zero-cooldown decision, "
                 "typed frame admission, priority and shutdown\n";
}
