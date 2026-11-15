#pragma once
// R202: independently authored, typed offline Keep Sprint state machine.
// Based on source-grounded event phases in the candidate references.
// Does not interact with Minecraft, JNI, keyboard input or combat.
#include <cmath>
#include <optional>

namespace slinkywinky::reference_compat {

inline constexpr unsigned kKeepSprintAttackEvent = 14;
inline constexpr unsigned kKeepSprintPostEvent = 15;
inline constexpr unsigned kKeepSprintUpdateEvent = 17;

struct KeepSprintMotion {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct KeepSprintAttackInput {
    bool phase_active = false;
    bool in_game = false;
    bool player_present = false;
    bool sprinting = false;
    // The candidate native random_chance() call is modelled as an
    // externally provided deterministic result, not an invented RNG.
    bool chance_succeeded = false;
    KeepSprintMotion motion{};
};

struct KeepSprintAttackDecision {
    bool armed = false;
};

struct KeepSprintPostInput {
    bool phase_active = false;
    bool player_present = false;
    bool on_ground = false;
    int ticks_existed = 0;
    int last_attacker_time = 0;
    bool target_valid = false;
    bool require_on_ground = false;
    bool allow_reduce = false;
    float motion_multiplier = 1.0f;
};

struct KeepSprintPostDecision {
    std::optional<KeepSprintMotion> proposed_motion{};
    bool sprint_reapply_pending = false;
};

struct KeepSprintUpdateInput {
    bool player_present = false;
};

struct KeepSprintUpdateDecision {
    bool propose_set_sprinting = false;
};

class KeepSprintController final {
public:
    KeepSprintAttackDecision on_attack(const KeepSprintAttackInput& input) noexcept {
        // The original before-hit callback ignores inactive phases without
        // changing the previously armed state.
        if (!input.phase_active) return {armed_};
        armed_ = input.in_game && input.player_present && input.sprinting &&
                 input.chance_succeeded && finite(input.motion);
        if (armed_) saved_motion_ = input.motion;
        return {armed_};
    }

    KeepSprintPostDecision on_attack_post(const KeepSprintPostInput& input) noexcept {
        KeepSprintPostDecision decision;
        if (!armed_ || !input.phase_active || !input.player_present ||
            (input.require_on_ground && !input.on_ground) ||
            input.ticks_existed != input.last_attacker_time ||
            (input.allow_reduce && input.target_valid) ||
            !std::isfinite(input.motion_multiplier) ||
            input.motion_multiplier < 0.0f)
            return decision;
        const double mult = static_cast<double>(input.motion_multiplier);
        KeepSprintMotion candidate{saved_motion_.x * mult,
                                   saved_motion_.y,
                                   saved_motion_.z * mult};
        // Defensive divergence: avoid emitting invalid motion proposals
        // when a setting or earlier state is malformed.
        if (!finite(candidate)) return decision;
        saved_motion_ = candidate;
        pending_ = true;
        decision.proposed_motion = saved_motion_;
        decision.sprint_reapply_pending = true;
        return decision;
    }

    KeepSprintUpdateDecision on_player_update(
        const KeepSprintUpdateInput& input) noexcept {
        // Consuming the pending flag even if the player is missing matches
        // both candidate native and Java lifecycle semantics.
        const bool was_pending = pending_;
        pending_ = false;
        return {was_pending && input.player_present};
    }

    void reset() noexcept {
        armed_ = false;
        pending_ = false;
        saved_motion_ = {};
    }

    bool armed() const noexcept { return armed_; }
    bool pending_reapply() const noexcept { return pending_; }
    KeepSprintMotion saved_motion() const noexcept { return saved_motion_; }

private:
    static bool finite(const KeepSprintMotion& motion) noexcept {
        return std::isfinite(motion.x) && std::isfinite(motion.y) &&
               std::isfinite(motion.z);
    }

    KeepSprintMotion saved_motion_{};
    bool armed_ = false;
    bool pending_ = false;
};

} // namespace slinkywinky::reference_compat
