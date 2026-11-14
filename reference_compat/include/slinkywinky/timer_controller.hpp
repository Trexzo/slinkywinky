#pragma once
// R201: first-party, offline Timer state proposals reconstructed from the
// pinned public event/setting contract. No JNI, game-time writes or hooks.
#include <cmath>
#include <optional>

namespace slinkywinky::reference_compat {

inline constexpr unsigned kTimerFrameEvent = 1;
inline constexpr unsigned kTimerToggleEvent = 5;

struct TimerSettings {
    float speed = 1.0f;
    bool require_mouse = false;
    bool require_falling = false;
    bool require_damage = false;
};

struct TimerFrameInput {
    bool in_game = false;
    bool player_present = false;
    bool mouse_pressed = false;
    bool on_ground = true;
    double vertical_motion = 0.0;
    int hurt_time = 0;
    TimerSettings settings{};
};

struct TimerFrameDecision {
    float proposed_speed = 1.0f;
    bool conditions_met = false;
};

struct TimerToggleInput {
    bool targets_timer = false;
    bool switched_on = false;
};

struct TimerToggleDecision {
    // Only a matching disable proposes restoring vanilla 1.0x game speed.
    // A non-Timer toggle and an enable produce no timer write.
    std::optional<float> proposed_speed{};
};

class TimerController final {
public:
    TimerFrameDecision on_frame(const TimerFrameInput& input) const noexcept {
        TimerFrameDecision result;
        // Defensive divergence: upstream assumes a valid game/JVM context.
        // Never propose a non-default timer for a missing world/player.
        if (!input.in_game || !input.player_present ||
            !std::isfinite(input.settings.speed) ||
            input.settings.speed <= 0.0f)
            return result;
        if (input.settings.require_mouse && !input.mouse_pressed)
            return result;
        if (input.settings.require_falling &&
            (input.on_ground || !std::isfinite(input.vertical_motion) ||
             input.vertical_motion >= 0.0))
            return result;
        if (input.settings.require_damage && input.hurt_time <= 0)
            return result;
        result.conditions_met = true;
        result.proposed_speed = input.settings.speed;
        return result;
    }

    TimerToggleDecision on_toggle(const TimerToggleInput& input) const noexcept {
        if (input.targets_timer && !input.switched_on)
            return {1.0f};
        return {};
    }
};

} // namespace slinkywinky::reference_compat
