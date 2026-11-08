#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;

static void check(bool condition, const char* expression, int line) {
    if (!condition) throw std::runtime_error(
        "R177 line " + std::to_string(line) + ": " + expression);
}
#define CHECK(x) check(static_cast<bool>(x), #x, __LINE__)

static ModuleSpec module(std::string name, unsigned category,
                         int event_priority, bool initially_enabled = false) {
    return {std::move(name), category, {{kReachQueryEvent, event_priority}},
            initially_enabled};
}

static ExecutionFrame query(int last_attacker_time = 0, float distance = 3.2f,
                            float activation = 3.0f) {
    ExecutionFrame frame;
    frame.event = kReachQueryEvent;
    frame.world_present = true;
    frame.player_present = true;
    ReachQueryPayload reach;
    reach.input.sprinting = true;
    reach.input.last_attacker_time = last_attacker_time;
    reach.settings.distance = distance;
    reach.settings.activation_ticks = activation;
    frame.reach = reach;
    return frame;
}

int main() {
    ModuleExecutionBridge bridge;
    const auto reach = bridge.add(module("Reach", 0, 7));
    const auto probe = bridge.add(module("Probe", 2, 12, true));
    const auto fallback = bridge.add(module("Fallback", 0, -2, true));
    CHECK(reach == 0 && probe == 1 && fallback == 2);

    CHECK(!bridge.bind_reach(probe));                 // name/category ownership
    CHECK(!bridge.bind(reach, 27, [](const Delivery&, ExecutionFrame&) {}));
    CHECK(!bridge.bind(reach, 2, [](const Delivery&, ExecutionFrame&) {}));
    CHECK(!bridge.bind(999, kReachQueryEvent,
                       [](const Delivery&, ExecutionFrame&) {}));
    CHECK(bridge.bind_reach(reach));                  // REAL src/core.cpp code
    CHECK(!bridge.bind_reach(reach));                 // duplicate binding rejected

    unsigned probe_calls = 0;
    bool reentry_denied = false;
    CHECK(bridge.bind(probe, kReachQueryEvent, [&](const Delivery& delivered,
                                                  ExecutionFrame&) {
        CHECK(delivered.recipient == probe);
        ++probe_calls;
        auto attempted_nested = query();
        reentry_denied = !bridge.dispatch(attempted_nested);
    }));
    CHECK(bridge.implemented_event_bindings() == 2);

    auto frame = query();
    frame.world_present = false;
    CHECK(!bridge.dispatch(frame));
    CHECK(frame.executed_modules.empty());
    frame.world_present = true;
    frame.player_present = false;
    CHECK(!bridge.dispatch(frame));

    CHECK(bridge.request(reach, true));
    CHECK(!bridge.apply(false, false));
    CHECK(!bridge.apply(true, false));
    CHECK(!bridge.enabled(reach));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(reach));

    frame = query();
    CHECK(bridge.dispatch(frame));
    CHECK(reentry_denied);
    CHECK(frame.reach && frame.reach->executions == 1);
    CHECK(frame.reach->proposed_distance.has_value());
    CHECK(*frame.reach->proposed_distance == 3.2f);
    CHECK((frame.executed_modules == std::vector<std::size_t>{probe, reach}));
    CHECK(probe_calls == 1 && bridge.reach_cooldown_ticks() == 0);

    // The underlying independent ReachController uses a 9-tick cooldown.
    // The new attack (attacker-time change) must suppress Reach first.
    frame = query(123);
    CHECK(bridge.dispatch(frame));
    CHECK(!frame.reach->proposed_distance);
    CHECK(bridge.reach_cooldown_ticks() == 9);
    for (int tick = 1; tick <= 7; ++tick) {
        frame = query(123);
        CHECK(bridge.dispatch(frame));
        CHECK(frame.reach->executions == 1);
        CHECK(frame.reach->proposed_distance.has_value() == (tick == 7));
        CHECK(bridge.reach_cooldown_ticks() == 9 - tick);
    }

    // Disabling Reach leaves other module callbacks and their order intact.
    CHECK(bridge.request(reach, false));
    CHECK(bridge.apply(true, true));
    CHECK(!bridge.enabled(reach));
    frame = query(123);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.reach->executions == 0);
    CHECK(!frame.reach->proposed_distance);
    CHECK((frame.executed_modules == std::vector<std::size_t>{probe}));

    // Reenable does not imply original state-reset semantics; explicit
    // world-state reset is a separate, reviewable adapter operation.
    CHECK(bridge.request(reach, true));
    CHECK(bridge.apply(true, true));
    bridge.reset_reach_world_state();
    frame = query(123);
    CHECK(bridge.dispatch(frame));
    CHECK(!frame.reach->proposed_distance);
    CHECK(bridge.reach_cooldown_ticks() == 9);

    // A malformed typed event must fail before invoking any callback.
    frame = query();
    frame.event = 26;
    CHECK(!bridge.dispatch(frame));
    frame.event = 27;
    CHECK(!bridge.dispatch(frame));

    // NaN output cannot escape from existing ReachController.
    bridge.reset_reach_world_state();
    frame = query(0, std::numeric_limits<float>::quiet_NaN());
    CHECK(bridge.dispatch(frame));
    CHECK(frame.reach->executions == 1);
    CHECK(!frame.reach->proposed_distance);

    // Observer exceptions must clear the active-frame pointer.
    bool throw_once = true;
    CHECK(bridge.bind(fallback, kReachQueryEvent,
                      [&](const Delivery&, ExecutionFrame&) {
                          if (throw_once) {
                              throw_once = false;
                              throw std::runtime_error("synthetic callback failure");
                          }
                      }));
    bool threw = false;
    try {
        frame = query();
        bridge.dispatch(frame);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    CHECK(threw);
    frame = query();
    CHECK(bridge.dispatch(frame));
    CHECK((frame.executed_modules == std::vector<std::size_t>{probe, reach, fallback}));

    bridge.stop();
    CHECK(bridge.implemented_event_bindings() == 0);
    frame = query();
    CHECK(!bridge.dispatch(frame));
    CHECK(!bridge.request(reach, true));
    CHECK(!bridge.apply(true, true));
    CHECK(!bridge.bind_reach(reach));

    ModuleExecutionBridge wrong_slot;
    const auto missing = wrong_slot.add(ModuleSpec{"Reach", 0, {{2, 100}}, true});
    CHECK(!wrong_slot.bind_reach(missing));
    const auto wrong_category = wrong_slot.add(ModuleSpec{"Other", 2, {{8, 100}}, true});
    CHECK(!wrong_slot.bind_reach(wrong_category));

    std::cout << "R177 module execution bridge PASS: metadata-registered "
                 "Reach callback executed via original R-series core, "
                 "9-tick cooldown, input guards, priorities and shutdown\n";
    return 0;
}
