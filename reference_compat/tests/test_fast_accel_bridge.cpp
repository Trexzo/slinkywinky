#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;
static void check(bool result, const char* label, int line) {
    if (!result)
        throw std::runtime_error("R189 line " + std::to_string(line) +
                                 ": " + label);
}
#define CHECK(e) check(static_cast<bool>(e), #e, __LINE__)
static bool close(double a, double b, double eps = 0.00001) {
    return std::abs(a - b) < eps;
}
static FastAccelInput base_input() {
    FastAccelInput value;
    value.forward = 1.0f;
    value.movement_speed = 0.1f;
    value.on_ground = true;
    value.motion_x = 0.1;
    value.motion_y = 0.25;
    value.motion_z = 0.0;
    value.settings.acceleration = 0.5f;
    return value;
}
static ExecutionFrame tick(const FastAccelInput& input) {
    ExecutionFrame f;
    f.event = kFastAccelEvent;
    f.world_present = true;
    f.player_present = true;
    f.fast_accel = FastAccelTickPayload{input, {}, 0};
    return f;
}
int main() {
    FastAccelController policy;
    auto input = base_input();
    auto d = policy.evaluate(input);
    CHECK(d.proposed_motion.has_value());
    CHECK(close(d.current_horizontal_speed, 0.1));
    // Independent analytical oracle: push=0.1, sum of powers 0.546^0..19.
    const double terminal_oracle =
        0.1 * (1.0 - std::pow(0.546, 20.0)) / (1.0 - 0.546);
    CHECK(close(d.target_horizontal_speed, terminal_oracle));
    CHECK(close(d.proposed_motion->x, (0.1 + terminal_oracle) / 2.0));
    CHECK(close(d.proposed_motion->y, 0.25));
    CHECK(close(d.proposed_motion->z, 0.0));

    input.settings.acceleration = 0.0f;
    d = policy.evaluate(input);
    CHECK(d.proposed_motion && close(d.proposed_motion->x, 0.1));
    input.settings.acceleration = 1.0f;
    d = policy.evaluate(input);
    CHECK(d.proposed_motion && close(d.proposed_motion->x, terminal_oracle));
    input.settings.acceleration = 0.5f;
    input.sprint_hook = true;
    input.entity_sprinting = true;
    d = policy.evaluate(input);
    CHECK(d.proposed_motion &&
          close(d.target_horizontal_speed, terminal_oracle + 0.2));
    CHECK(close(d.proposed_motion->x,
                (0.1 + terminal_oracle + 0.2) / 2.0));
    input.entity_sprinting = false;
    d = policy.evaluate(input);
    CHECK(d.proposed_motion &&
          close(d.target_horizontal_speed, terminal_oracle));

    input = base_input();
    input.motion_x = 0.06;
    input.motion_z = 0.08;
    d = policy.evaluate(input);
    CHECK(d.proposed_motion);
    CHECK(close(d.proposed_motion->x / d.proposed_motion->z, 0.75));
    CHECK(close(d.proposed_motion->y, 0.25));
    input = base_input();
    input.forward = 1.0f;
    input.strafe = 1.0f;
    d = policy.evaluate(input);
    CHECK(d.proposed_motion &&
          close(d.target_horizontal_speed, terminal_oracle));

    input = base_input();
    input.forward = 0.0f; CHECK(!policy.evaluate(input).proposed_motion);
    input.forward = 1.0f;
    input.on_ground = false; CHECK(!policy.evaluate(input).proposed_motion);
    input.on_ground = true;
    input.motion_x = 0.0; CHECK(!policy.evaluate(input).proposed_motion);
    input.motion_x = 0.5; CHECK(!policy.evaluate(input).proposed_motion);
    input.motion_x = 0.1;
    input.settings.require_weapon = true;
    CHECK(!policy.evaluate(input).proposed_motion);
    input.accepted_weapon = true;
    CHECK(policy.evaluate(input).proposed_motion);
    input.settings.require_not_damaged = true;
    input.hurt_time = 2;
    CHECK(!policy.evaluate(input).proposed_motion);
    input.hurt_time = 0;
    CHECK(policy.evaluate(input).proposed_motion);
    input.settings.acceleration = -0.2f;
    CHECK(!policy.evaluate(input).proposed_motion);
    input.settings.acceleration = 1.2f;
    CHECK(!policy.evaluate(input).proposed_motion);
    input.settings.acceleration =
        std::numeric_limits<float>::quiet_NaN();
    CHECK(!policy.evaluate(input).proposed_motion);
    input = base_input();
    input.movement_speed = -0.1f;
    CHECK(!policy.evaluate(input).proposed_motion);
    input = base_input();
    input.motion_z = std::numeric_limits<double>::infinity();
    CHECK(!policy.evaluate(input).proposed_motion);

    ModuleExecutionBridge bridge;
    const auto fast = bridge.add({"Fast Accel", 1, {{22, 0}}, false});
    const auto observer = bridge.add({"Earlier Observer", 2, {{22, 100}}, true});
    const auto unbound = bridge.add({"Unimplemented", 1, {{22, -100}}, true});
    CHECK(fast == 0 && observer == 1 && unbound == 2);
    unsigned observed = 0;
    CHECK(bridge.bind(observer, kFastAccelEvent,
        [&](const Delivery& delivery, ExecutionFrame&) {
            CHECK(delivery.event == kFastAccelEvent &&
                  delivery.recipient == observer);
            ++observed;
        }));
    CHECK(bridge.bind_fast_accel_from_catalog());
    CHECK(!bridge.bind_fast_accel(fast));
    const auto cov = bridge.coverage();
    CHECK(cov.registered_modules == 3);
    CHECK(cov.native_callback_subscriptions == 3);
    CHECK(cov.modules_with_executable_bindings == 2);
    CHECK(cov.executable_callback_bindings == 2);
    CHECK(cov.unimplemented_callback_subscriptions == 1);
    CHECK(cov.implemented_per_event[kFastAccelEvent] == 2);

    auto f = tick(base_input());
    CHECK(bridge.dispatch(f));
    CHECK(f.fast_accel->executions == 0);
    CHECK(!f.fast_accel->decision.proposed_motion);
    CHECK((f.executed_modules == std::vector<std::size_t>{observer}));
    CHECK(observed == 1);

    CHECK(bridge.request(fast, true));
    CHECK(!bridge.apply(false, false));
    CHECK(!bridge.apply(true, false));
    CHECK(!bridge.enabled(fast));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(fast));

    f = tick(base_input());
    CHECK(bridge.dispatch(f));
    CHECK(f.fast_accel->executions == 1);
    CHECK(f.fast_accel->decision.proposed_motion);
    CHECK(close(f.fast_accel->decision.proposed_motion->x,
                (0.1 + terminal_oracle) / 2.0));
    CHECK((f.executed_modules ==
           std::vector<std::size_t>{observer, fast}));
    f = tick(base_input());
    f.world_present = false;
    CHECK(!bridge.dispatch(f));
    f.world_present = true;
    f.player_present = false;
    CHECK(!bridge.dispatch(f));
    f.player_present = true;
    f.event = 21;
    CHECK(!bridge.dispatch(f));
    f.event = kFastAccelEvent;
    f.no_slow = NoSlowTickPayload{};
    CHECK(!bridge.dispatch(f));

    CHECK(bridge.request(fast, false));
    CHECK(bridge.apply(true, true));
    f = tick(base_input());
    CHECK(bridge.dispatch(f));
    CHECK(f.fast_accel->executions == 0);
    CHECK(!f.fast_accel->decision.proposed_motion);
    CHECK((f.executed_modules == std::vector<std::size_t>{observer}));
    bridge.stop();
    CHECK(!bridge.dispatch(f));
    CHECK(!bridge.bind_fast_accel_from_catalog());

    ModuleExecutionBridge wrong_category;
    auto id = wrong_category.add({"Fast Accel", 2, {{22, 0}}, true});
    CHECK(!wrong_category.bind_fast_accel(id));
    ModuleExecutionBridge wrong_event;
    id = wrong_event.add({"Fast Accel", 1, {{21, 0}}, true});
    CHECK(!wrong_event.bind_fast_accel(id));
    ModuleExecutionBridge wrong_priority;
    id = wrong_priority.add({"Fast Accel", 1, {{22, 1}}, true});
    CHECK(!wrong_priority.bind_fast_accel(id));
    std::cout << "R189 Fast Accel PASS: 20-tick friction oracle, sprint "
                 "bonus, diagonal normalization, gating, event-22 lifecycle\n";
}
