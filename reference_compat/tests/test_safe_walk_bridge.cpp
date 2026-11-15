#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;
static void require(bool ok, const char* message, int line) {
    if (!ok)
        throw std::runtime_error("R190 line " + std::to_string(line) +
                                 ": " + message);
}
#define CHECK(x) require(static_cast<bool>(x), #x, __LINE__)
static bool near(double a, double b) { return std::abs(a-b) < 1e-9; }

static SafeWalkInput sample() {
    SafeWalkInput in;
    in.in_game = true;
    in.pitch_degrees = 65.0f;
    in.move_forward = 0.0f;
    in.holding_blocks = true;
    in.on_ground = true;
    in.motion_x = 0.8;
    in.motion_y = 0.25;
    in.motion_z = -0.4;
    in.settings = {true, true, true, 0.5f};
    return in;
}
static ExecutionFrame tick(const SafeWalkInput& input) {
    ExecutionFrame frame;
    frame.event = kSafeWalkEvent;
    frame.world_present = true;
    frame.player_present = true;
    frame.safe_walk = SafeWalkTickPayload{input, {}, 0};
    return frame;
}
int main() {
    SafeWalkController controller;
    auto in = sample();
    auto d = controller.evaluate(in);
    CHECK(d.proposed_edge_guard_active == true);
    CHECK(d.proposed_motion.has_value());
    CHECK(near(d.proposed_motion->x, 0.4));
    CHECK(near(d.proposed_motion->y, 0.25));
    CHECK(near(d.proposed_motion->z, -0.2));

    in.pitch_degrees = 64.999f;
    d = controller.evaluate(in);
    CHECK(d.proposed_edge_guard_active == false && !d.proposed_motion);
    in.pitch_degrees = 65.0f;
    in.move_forward = 0.1f;
    d = controller.evaluate(in);
    CHECK(d.proposed_edge_guard_active == false && !d.proposed_motion);
    in.move_forward = -0.8f;  // backward is permitted
    d = controller.evaluate(in);
    CHECK(d.proposed_edge_guard_active == true && d.proposed_motion);
    in.move_forward = 0.0f;
    in.holding_blocks = false;
    d = controller.evaluate(in);
    CHECK(d.proposed_edge_guard_active == false && !d.proposed_motion);

    in = sample();
    in.on_ground = false;
    d = controller.evaluate(in);
    CHECK(d.proposed_edge_guard_active == true && !d.proposed_motion);
    in.on_ground = true;
    in.settings.motion_multiplier = 1.0f;
    d = controller.evaluate(in);
    CHECK(d.proposed_edge_guard_active == true && !d.proposed_motion);
    in.settings.motion_multiplier = 0.0f;
    d = controller.evaluate(in);
    CHECK(d.proposed_edge_guard_active == true && d.proposed_motion);
    CHECK(near(d.proposed_motion->x, 0.0));
    CHECK(near(d.proposed_motion->y, 0.25));
    in.settings.motion_multiplier = 0.5f;
    in.in_game = false;
    d = controller.evaluate(in);
    CHECK(!d.proposed_edge_guard_active && !d.proposed_motion);
    in.in_game = true;
    in.settings.motion_multiplier = -0.5f;
    CHECK(!controller.evaluate(in).proposed_edge_guard_active);
    in = sample();
    in.pitch_degrees = std::numeric_limits<float>::quiet_NaN();
    CHECK(!controller.evaluate(in).proposed_edge_guard_active);
    in = sample();
    in.motion_x = std::numeric_limits<double>::infinity();
    d = controller.evaluate(in);
    CHECK(d.proposed_edge_guard_active == true && !d.proposed_motion);

    ModuleExecutionBridge bridge;
    const auto safe = bridge.add({"Safe Walk", 1, {{0, 0}}, false});
    const auto earlier = bridge.add({"Priority Observer", 2, {{0, 100}}, true});
    const auto unbound = bridge.add({"Fast Place", 3, {{0, -1000}}, true});
    CHECK(safe == 0 && earlier == 1 && unbound == 2);
    unsigned observed = 0;
    CHECK(bridge.bind(earlier, kSafeWalkEvent,
        [&](const Delivery& delivery, ExecutionFrame&) {
            CHECK(delivery.event == 0 && delivery.recipient == earlier);
            ++observed;
        }));
    CHECK(bridge.bind_safe_walk_from_catalog());
    CHECK(!bridge.bind_safe_walk(safe));
    const auto coverage = bridge.coverage();
    CHECK(coverage.registered_modules == 3);
    CHECK(coverage.native_callback_subscriptions == 3);
    CHECK(coverage.executable_callback_bindings == 2);
    CHECK(coverage.modules_with_executable_bindings == 2);
    CHECK(coverage.unimplemented_callback_subscriptions == 1);
    CHECK(coverage.implemented_per_event[0] == 2);

    auto frame = tick(sample());
    CHECK(bridge.dispatch(frame));
    CHECK(frame.safe_walk->executions == 0);
    CHECK(!frame.safe_walk->decision.proposed_edge_guard_active);
    CHECK((frame.executed_modules == std::vector<std::size_t>{earlier}));
    CHECK(observed == 1);
    CHECK(bridge.request(safe, true));
    CHECK(!bridge.apply(false, false));
    CHECK(!bridge.apply(true, false));
    CHECK(!bridge.enabled(safe));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(safe));
    frame = tick(sample());
    CHECK(bridge.dispatch(frame));
    CHECK(frame.safe_walk->executions == 1);
    CHECK(frame.safe_walk->decision.proposed_edge_guard_active == true);
    CHECK(frame.safe_walk->decision.proposed_motion);
    CHECK(near(frame.safe_walk->decision.proposed_motion->x, 0.4));
    CHECK((frame.executed_modules == std::vector<std::size_t>{earlier, safe}));

    frame = tick(sample());
    frame.world_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.world_present = true;
    frame.player_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.player_present = true;
    frame.event = 18;
    CHECK(!bridge.dispatch(frame));
    frame.event = kSafeWalkEvent;
    frame.anti_debuff = AntiDebuffTickPayload{};
    CHECK(!bridge.dispatch(frame));

    CHECK(bridge.request(safe, false));
    CHECK(bridge.apply(true, true));
    frame = tick(sample());
    CHECK(bridge.dispatch(frame));
    CHECK(frame.safe_walk->executions == 0);
    CHECK(!frame.safe_walk->decision.proposed_edge_guard_active);
    CHECK((frame.executed_modules == std::vector<std::size_t>{earlier}));
    bridge.stop();
    CHECK(!bridge.dispatch(frame));
    CHECK(!bridge.bind_safe_walk_from_catalog());

    ModuleExecutionBridge bad_category;
    auto id = bad_category.add({"Safe Walk", 2, {{0, 0}}, true});
    CHECK(!bad_category.bind_safe_walk(id));
    ModuleExecutionBridge bad_event;
    id = bad_event.add({"Safe Walk", 1, {{1, 0}}, true});
    CHECK(!bad_event.bind_safe_walk(id));
    ModuleExecutionBridge bad_priority;
    id = bad_priority.add({"Safe Walk", 1, {{0, 1}}, true});
    CHECK(!bad_priority.bind_safe_walk(id));

    std::cout << "R190 Safe Walk PASS: pitch/forward/blocks gates, horizontal "
                 "multiplier, no-game and airborne guard, event 0/lifecycle\n";
}
