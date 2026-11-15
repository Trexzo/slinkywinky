#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;
static void verify(bool ok, const char* what, int line) {
    if (!ok) throw std::runtime_error(
        "R184 line " + std::to_string(line) + ": " + what);
}
#define CHECK(expr) verify(static_cast<bool>(expr), #expr, __LINE__)

static NoUseDelayInput using_item(int ticks, int slot = 0,
                                 bool allow_desync = false) {
    NoUseDelayInput input;
    input.in_game = true;
    input.use_key_down = true;
    input.physical_use_held = true;
    input.selected_slot = slot;
    input.use_ticks_left = ticks;
    input.allow_desync = allow_desync;
    return input;
}
static ExecutionFrame tick(int ticks, int slot = 0, bool allow_desync = false) {
    ExecutionFrame frame;
    frame.event = kNoUseDelayEvent;
    frame.world_present = true;
    frame.player_present = true;
    frame.no_use_delay = NoUseDelayTickPayload{using_item(ticks, slot, allow_desync), {}, 0};
    return frame;
}

int main() {
    NoUseDelayController pure;
    auto result = pure.evaluate(using_item(3));
    CHECK(result.tracked_remaining_ticks == 3);
    CHECK(!result.proposed_use_key_pressed.has_value());
    result = pure.evaluate(using_item(2));
    CHECK(result.tracked_remaining_ticks == 2);
    CHECK(!result.proposed_use_key_pressed);
    result = pure.evaluate(using_item(1));
    CHECK(result.tracked_remaining_ticks == 1);
    CHECK(!result.proposed_use_key_pressed);
    result = pure.evaluate(using_item(0));
    CHECK(result.tracked_remaining_ticks == 0);
    CHECK(result.proposed_use_key_pressed == false);

    // Slot change proposes physically held key state, resets countdown
    // and begins tracking a new use, exactly in the original order.
    result = pure.evaluate(using_item(6));
    CHECK(result.tracked_remaining_ticks == 6);
    result = pure.evaluate(using_item(4, 1));
    CHECK(result.tracked_slot == 1 && result.tracked_remaining_ticks == 4);
    CHECK(result.proposed_use_key_pressed == true);

    // Swapping the slot while the key is no longer effectively down
    // still restores its physical state, but does not start a new use.
    auto no_use = using_item(3, 2);
    no_use.use_key_down = false;
    no_use.physical_use_held = false;
    result = pure.evaluate(no_use);
    CHECK(result.tracked_slot == 2 && result.tracked_remaining_ticks == 0);
    CHECK(result.proposed_use_key_pressed == false);

    // A paused effective key preserves a countdown when slot is unchanged.
    pure.reset();
    pure.evaluate(using_item(5));
    no_use = using_item(5);
    no_use.use_key_down = false;
    result = pure.evaluate(no_use);
    CHECK(result.tracked_remaining_ticks == 5);
    CHECK(!result.proposed_use_key_pressed);

    // Desync OFF: client-side zero alone must not release early.
    pure.reset();
    pure.evaluate(using_item(5));
    result = pure.evaluate(using_item(0, 0, false));
    CHECK(result.tracked_remaining_ticks == 4);
    CHECK(!result.proposed_use_key_pressed);

    // Desync ON: client-side zero releases at countdown 4.
    pure.reset();
    pure.evaluate(using_item(5));
    result = pure.evaluate(using_item(0, 0, true));
    CHECK(result.tracked_remaining_ticks == 4);
    CHECK(result.proposed_use_key_pressed == false);

    // Going out of game clears remaining but preserves slot tracking.
    pure.reset();
    pure.evaluate(using_item(3, 4));
    auto not_in_game = using_item(0, 7);
    not_in_game.in_game = false;
    result = pure.evaluate(not_in_game);
    CHECK(result.tracked_remaining_ticks == 0 && result.tracked_slot == 4);
    CHECK(!result.proposed_use_key_pressed);
    pure.reset();
    CHECK(pure.last_slot() == 0 && pure.remaining_ticks() == 0);

    ModuleExecutionBridge bridge;
    const auto module = bridge.add({"No Use Delay", 4, {{18, 0}}, false});
    const auto prior = bridge.add({"Prior Observer", 4, {{18, 100}}, true});
    const auto other = bridge.add({"Other Unimplemented", 0, {{18, -10}}, true});
    CHECK(module == 0 && prior == 1 && other == 2);
    unsigned prior_count = 0;
    CHECK(bridge.bind(prior, kNoUseDelayEvent,
                      [&](const Delivery& d, ExecutionFrame&) {
        CHECK(d.event == 18 && d.recipient == prior);
        ++prior_count;
    }));
    CHECK(bridge.bind_no_use_delay_from_catalog());
    CHECK(!bridge.bind_no_use_delay(module));
    CHECK(bridge.coverage().native_callback_subscriptions == 3);
    CHECK(bridge.coverage().executable_callback_bindings == 2);
    CHECK(bridge.coverage().unimplemented_callback_subscriptions == 1);

    auto frame = tick(3);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_use_delay->executions == 0);
    CHECK((frame.executed_modules == std::vector<std::size_t>{prior}));
    CHECK(prior_count == 1);
    CHECK(bridge.request(module, true));
    CHECK(!bridge.apply(false, true));
    CHECK(!bridge.enabled(module));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(module));
    frame = tick(3);
    CHECK(bridge.dispatch(frame));
    CHECK((frame.executed_modules == std::vector<std::size_t>{prior, module}));
    CHECK(frame.no_use_delay->executions == 1);
    CHECK(frame.no_use_delay->decision.tracked_remaining_ticks == 3);
    frame = tick(2);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_use_delay->decision.tracked_remaining_ticks == 2);
    frame = tick(1);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_use_delay->decision.tracked_remaining_ticks == 1);
    frame = tick(0);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_use_delay->decision.proposed_use_key_pressed == false);

    frame = tick(4, 1);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_use_delay->decision.tracked_slot == 1);
    CHECK(frame.no_use_delay->decision.proposed_use_key_pressed == true);
    frame = tick(0, 1, true);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_use_delay->decision.proposed_use_key_pressed == false);

    frame = tick(4, 1);
    frame.world_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.world_present = true;
    frame.player_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.player_present = true;
    frame.event = 17;
    CHECK(!bridge.dispatch(frame));
    frame.event = 18;
    frame.no_hit_delay = NoHitDelayTickPayload{};
    CHECK(!bridge.dispatch(frame));
    frame.no_hit_delay.reset();

    CHECK(bridge.request(module, false));
    CHECK(bridge.apply(true, true));
    frame = tick(3);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_use_delay->executions == 0);
    CHECK((frame.executed_modules == std::vector<std::size_t>{prior}));
    bridge.reset_no_use_delay_world_state();
    CHECK(bridge.request(module, true));
    CHECK(bridge.apply(true, true));
    frame = tick(2);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.no_use_delay->decision.tracked_remaining_ticks == 2);
    bridge.stop();
    CHECK(!bridge.dispatch(frame));
    CHECK(!bridge.bind_no_use_delay_from_catalog());

    ModuleExecutionBridge wrong_category;
    auto id = wrong_category.add({"No Use Delay", 1, {{18, 0}}, true});
    CHECK(!wrong_category.bind_no_use_delay(id));
    ModuleExecutionBridge wrong_event;
    id = wrong_event.add({"No Use Delay", 4, {{17, 0}}, true});
    CHECK(!wrong_event.bind_no_use_delay(id));
    ModuleExecutionBridge wrong_priority;
    id = wrong_priority.add({"No Use Delay", 4, {{18, 1}}, true});
    CHECK(!wrong_priority.bind_no_use_delay(id));

    std::cout << "R184 No Use Delay PASS: countdown, slot restoration, "
                 "desync option, event 18 priority, lifecycle and teardown\n";
}
