#include "slinkywinky/module_execution_bridge.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;

static void check(bool ok, const char* expr, int line) {
    if (!ok) throw std::runtime_error(
        "R202 line " + std::to_string(line) + ": " + expr);
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

static ExecutionFrame before(KeepSprintAttackInput input) {
    ExecutionFrame f;
    f.event = kKeepSprintAttackEvent;
    f.world_present = true;
    f.player_present = true;
    f.keep_sprint_attack = KeepSprintAttackPayload{input, {}, 0};
    return f;
}
static ExecutionFrame after(KeepSprintPostInput input) {
    ExecutionFrame f;
    f.event = kKeepSprintPostEvent;
    f.world_present = true;
    f.player_present = true;
    f.keep_sprint_post = KeepSprintPostPayload{input, {}, 0};
    return f;
}
static ExecutionFrame update(bool player_present = true) {
    ExecutionFrame f;
    f.event = kKeepSprintUpdateEvent;
    f.world_present = true;
    f.player_present = true;
    f.keep_sprint_update = KeepSprintUpdatePayload{{player_present}, {}, 0};
    return f;
}

int main() {
    KeepSprintController policy;
    KeepSprintAttackInput attack;
    attack.phase_active = true;
    attack.in_game = true;
    attack.player_present = true;
    attack.sprinting = true;
    attack.chance_succeeded = true;
    attack.motion = {0.8, 0.2, -0.4};

    CHECK(policy.on_attack(attack).armed);
    CHECK(policy.armed() && !policy.pending_reapply());
    KeepSprintPostInput post;
    post.phase_active = true;
    post.player_present = true;
    post.on_ground = true;
    post.ticks_existed = post.last_attacker_time = 30;
    post.require_on_ground = true;
    post.motion_multiplier = 0.5f;
    post.allow_reduce = true;
    post.target_valid = true;
    CHECK(!policy.on_attack_post(post).proposed_motion);
    post.target_valid = false;
    post.on_ground = false;
    CHECK(!policy.on_attack_post(post).proposed_motion);
    post.on_ground = true;
    post.last_attacker_time = 29;
    CHECK(!policy.on_attack_post(post).proposed_motion);
    post.last_attacker_time = 30;
    post.phase_active = false;
    CHECK(!policy.on_attack_post(post).proposed_motion);
    post.phase_active = true;

    const auto proposed = policy.on_attack_post(post);
    CHECK(proposed.sprint_reapply_pending && proposed.proposed_motion);
    CHECK(std::abs(proposed.proposed_motion->x - 0.4) < 1e-9);
    CHECK(std::abs(proposed.proposed_motion->y - 0.2) < 1e-9);
    CHECK(std::abs(proposed.proposed_motion->z + 0.2) < 1e-9);
    CHECK(policy.pending_reapply());

    // The candidate native source mutates saved X/Z motion in-place,
    // so a repeated qualifying post phase applies the multiplier again.
    const auto repeated = policy.on_attack_post(post);
    CHECK(repeated.proposed_motion && std::abs(repeated.proposed_motion->x - 0.2) < 1e-9);
    CHECK(std::abs(repeated.proposed_motion->y - 0.2) < 1e-9);
    CHECK(policy.on_player_update({true}).propose_set_sprinting);
    CHECK(!policy.pending_reapply());
    CHECK(!policy.on_player_update({true}).propose_set_sprinting);
    CHECK(policy.on_attack(KeepSprintAttackInput{}).armed);
    CHECK(policy.armed()); // Inactive pre-event did not reset prior state.

    attack.chance_succeeded = false;
    CHECK(!policy.on_attack(attack).armed);
    CHECK(!policy.on_attack_post(post).proposed_motion);
    attack.chance_succeeded = true;
    attack.motion.x = std::numeric_limits<double>::quiet_NaN();
    CHECK(!policy.on_attack(attack).armed);
    attack.motion.x = 0.8;
    CHECK(policy.on_attack(attack).armed);
    post.motion_multiplier = std::numeric_limits<float>::infinity();
    CHECK(!policy.on_attack_post(post).proposed_motion);
    post.motion_multiplier = -1.0f;
    CHECK(!policy.on_attack_post(post).proposed_motion);
    post.motion_multiplier = 1.0f;
    CHECK(policy.on_attack_post(post).proposed_motion);
    CHECK(!policy.on_player_update({false}).propose_set_sprinting);
    CHECK(!policy.pending_reapply());
    policy.reset();
    CHECK(!policy.armed() && !policy.pending_reapply());
    CHECK(!policy.on_attack_post(post).proposed_motion);

    ModuleExecutionBridge bridge;
    const auto no_hit = bridge.add({"No Hit Delay", 4, {{14, 0}}, true});
    const auto use_fix = bridge.add({"Item Use Fix", 4, {{14, 0}, {15, 0}}, true});
    const auto jump = bridge.add({"No Jump Delay", 1, {{17, 100}}, true});
    const auto keep = bridge.add(
        {"Keep Sprint", 1, {{14, 0}, {15, 0}, {17, 0}}, false});
    CHECK(bridge.bind_no_hit_delay(no_hit));
    CHECK(bridge.bind_item_use_fix(use_fix));
    CHECK(bridge.bind_no_jump_delay(jump));
    CHECK(bridge.bind_keep_sprint_from_catalog());
    CHECK(!bridge.bind_keep_sprint(keep));

    const auto coverage = bridge.coverage();
    CHECK(coverage.registered_modules == 4);
    CHECK(coverage.native_callback_subscriptions == 8);
    CHECK(coverage.executable_callback_bindings == 8);
    CHECK(coverage.unimplemented_callback_subscriptions == 0);

    CHECK(bridge.request(keep, true));
    CHECK(!bridge.apply(false, true));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(keep));

    auto f = before(attack);
    f.keep_sprint_attack->input.chance_succeeded = true;
    f.no_hit_delay = NoHitDelayTickPayload{};
    f.item_use_fix = ItemUseFixTickPayload{};
    CHECK(bridge.dispatch(f));
    CHECK(f.keep_sprint_attack->executions == 1 && f.keep_sprint_attack->decision.armed);
    CHECK((f.executed_modules == std::vector<std::size_t>{no_hit, use_fix, keep}));

    auto p = after(post);
    p.keep_sprint_post->input.motion_multiplier = 0.5f;
    p.item_use_fix = ItemUseFixTickPayload{};
    CHECK(bridge.dispatch(p));
    CHECK(p.keep_sprint_post->executions == 1 &&
          p.keep_sprint_post->decision.proposed_motion);
    CHECK((p.executed_modules == std::vector<std::size_t>{use_fix, keep}));

    auto u = update(true);
    u.jump_delay = JumpDelayTickPayload{};
    CHECK(bridge.dispatch(u));
    CHECK(u.keep_sprint_update->executions == 1 &&
          u.keep_sprint_update->decision.propose_set_sprinting);
    CHECK((u.executed_modules == std::vector<std::size_t>{jump, keep}));
    u = update(true);
    CHECK(bridge.dispatch(u));
    CHECK(!u.keep_sprint_update->decision.propose_set_sprinting);

    // Wrong event, fabricated concurrent typed payload or missing world
    // must be rejected before reaching any live or offline callback.
    f = before(attack);
    f.event = kKeepSprintPostEvent;
    CHECK(!bridge.dispatch(f));
    p = after(post);
    p.event = kKeepSprintAttackEvent;
    CHECK(!bridge.dispatch(p));
    u = update();
    u.event = kKeepSprintAttackEvent;
    CHECK(!bridge.dispatch(u));
    f = before(attack);
    f.no_slow = NoSlowTickPayload{};
    CHECK(!bridge.dispatch(f));
    f = before(attack);
    f.world_present = false;
    CHECK(!bridge.dispatch(f));

    bridge.reset_keep_sprint_world_state();
    p = after(post);
    CHECK(bridge.dispatch(p));
    CHECK(!p.keep_sprint_post->decision.proposed_motion);

    CHECK(bridge.request(keep, false));
    CHECK(bridge.apply(true, true));
    CHECK(!bridge.enabled(keep));
    f = before(attack);
    CHECK(bridge.dispatch(f));
    CHECK(f.keep_sprint_attack->executions == 0);
    bridge.stop();
    CHECK(!bridge.bind_keep_sprint_from_catalog());

    for (unsigned variant = 0; variant < 5; ++variant) {
        ModuleExecutionBridge invalid;
        ModuleSpec spec{"Keep Sprint", variant == 0 ? 0u : 1u,
                        {{14, 0}, {15, 0}, {17, 0}}, true};
        if (variant == 1) spec.subscriptions[0].priority = 1;
        if (variant == 2) spec.subscriptions[1].priority = 1;
        if (variant == 3) spec.subscriptions[2].priority = 1;
        if (variant == 4) spec.subscriptions.pop_back();
        const auto id = invalid.add(spec);
        CHECK(!invalid.bind_keep_sprint(id));
        CHECK(invalid.coverage().executable_callback_bindings == 0);
    }
    std::cout << "R202_KEEP_SPRINT_OFFLINE_PASS events=14,15,17"
              << " policies=attack,post,update shared_event_payloads=3"
              << " wrong_metadata_rejected=5\n";
}
