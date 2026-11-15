#pragma once
// R189: independently authored pure Fast Accel decision. No original source
// copied, no game memory writes, JNI calls, keyboard or network interactions.
#include <cmath>
#include <limits>
#include <optional>

namespace slinkywinky::reference_compat {
inline constexpr unsigned kFastAccelEvent = 22;

struct FastAccelSettings {
    float acceleration = 0.5f;  // interpolation between current and terminal
    bool require_weapon = false;
    bool require_not_damaged = false;
};
struct FastAccelInput {
    float strafe = 0.0f;
    float forward = 0.0f;
    float movement_speed = 0.0f;
    bool sprint_hook = false;
    bool entity_sprinting = false;
    bool on_ground = false;
    bool accepted_weapon = false;
    int hurt_time = 0;
    double motion_x = 0.0;
    double motion_y = 0.0;
    double motion_z = 0.0;
    FastAccelSettings settings{};
};
struct FastAccelMotion { double x = 0.0, y = 0.0, z = 0.0; };
struct FastAccelDecision {
    std::optional<FastAccelMotion> proposed_motion{};
    double target_horizontal_speed = 0.0;
    double current_horizontal_speed = 0.0;
};

class FastAccelController final {
public:
    FastAccelDecision evaluate(const FastAccelInput& in) const noexcept {
        FastAccelDecision out;
        if ((in.forward == 0.0f && in.strafe == 0.0f) ||
            (in.settings.require_weapon && !in.accepted_weapon) ||
            (in.settings.require_not_damaged && in.hurt_time > 0) ||
            !in.on_ground) return out;

        // Fail closed for malformed numeric state. The original callback
        // does not necessarily implement the same guards.
        if (!std::isfinite(in.forward) || !std::isfinite(in.strafe) ||
            !std::isfinite(in.movement_speed) || in.movement_speed < 0.0f ||
            !std::isfinite(in.motion_x) || !std::isfinite(in.motion_y) ||
            !std::isfinite(in.motion_z) ||
            !std::isfinite(in.settings.acceleration) ||
            in.settings.acceleration < 0.0f ||
            in.settings.acceleration > 1.0f) return out;

        const double current = std::hypot(in.motion_x, in.motion_z);
        if (!std::isfinite(current) || current == 0.0) return out;
        out.current_horizontal_speed = current;

        // Use float intermediate input operations to mirror the typed
        // movement-hook contract, then accumulate 20 ticks in double.
        const float length = std::hypot(in.strafe, in.forward);
        if (!std::isfinite(length)) return out;
        const float scale = in.movement_speed /
                            (length > 1.0f ? length : 1.0f);
        const float push_strafe = in.strafe * scale;
        const float push_forward = in.forward * scale;
        const float push_float = std::hypot(push_strafe, push_forward);
        if (!std::isfinite(push_float)) return out;
        const double push = static_cast<double>(push_float);

        constexpr double kGroundFriction = 0.546;
        double terminal = push;
        for (int tick = 0; tick < 19; ++tick)
            terminal = terminal * kGroundFriction + push;
        if (in.sprint_hook && in.entity_sprinting)
            terminal += 0.2;
        if (!std::isfinite(terminal)) return out;
        out.target_horizontal_speed = terminal;
        // Already above target: do NOT slow the player.
        if (current > terminal) return out;

        const double alpha = static_cast<double>(in.settings.acceleration);
        const double next = current + (terminal - current) * alpha;
        const double result_x = in.motion_x / current * next;
        const double result_z = in.motion_z / current * next;
        if (!std::isfinite(result_x) || !std::isfinite(result_z))
            return out;
        out.proposed_motion = FastAccelMotion{result_x, in.motion_y, result_z};
        return out;
    }
};
} // namespace slinkywinky::reference_compat
