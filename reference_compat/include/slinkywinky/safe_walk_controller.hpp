#pragma once
// R190: independently authored pure Safe Walk policy.
// The actual edge-collision hook belongs to the separate game engine.
// No JNI, Minecraft field writes or movement input injection.
#include <cmath>
#include <optional>

namespace slinkywinky::reference_compat {
inline constexpr unsigned kSafeWalkEvent = 0;

struct SafeWalkSettings {
    bool require_looking_down = false;
    bool require_not_forward = false;
    bool require_holding_blocks = false;
    float motion_multiplier = 1.0f;
};

struct SafeWalkInput {
    bool in_game = false;
    float pitch_degrees = 0.0f;
    float move_forward = 0.0f;
    bool holding_blocks = false;
    bool on_ground = false;
    double motion_x = 0.0;
    double motion_y = 0.0;
    double motion_z = 0.0;
    SafeWalkSettings settings{};
};

struct SafeWalkMotion {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct SafeWalkDecision {
    std::optional<bool> proposed_edge_guard_active{};
    std::optional<SafeWalkMotion> proposed_motion{};
};

class SafeWalkController final {
public:
    SafeWalkDecision evaluate(const SafeWalkInput& in) const noexcept {
        SafeWalkDecision decision;
        if (!in.in_game) return decision;
        // Non-finite settings or comparisons are deliberately rejected,
        // unlike the candidate's unguarded arithmetic.
        if (!std::isfinite(in.pitch_degrees) ||
            !std::isfinite(in.move_forward) ||
            !std::isfinite(in.settings.motion_multiplier) ||
            in.settings.motion_multiplier < 0.0f) return decision;

        if ((in.settings.require_looking_down && in.pitch_degrees < 65.0f) ||
            (in.settings.require_not_forward && in.move_forward > 0.0f) ||
            (in.settings.require_holding_blocks && !in.holding_blocks)) {
            decision.proposed_edge_guard_active = false;
            return decision;
        }
        decision.proposed_edge_guard_active = true;
        // The edge guard remains active airborne or with normal speed.
        if (!in.on_ground || in.settings.motion_multiplier == 1.0f)
            return decision;
        if (!std::isfinite(in.motion_x) || !std::isfinite(in.motion_y) ||
            !std::isfinite(in.motion_z)) return decision;
        const double scalar = static_cast<double>(in.settings.motion_multiplier);
        const double nx = in.motion_x * scalar;
        const double nz = in.motion_z * scalar;
        if (!std::isfinite(nx) || !std::isfinite(nz)) return decision;
        decision.proposed_motion = SafeWalkMotion{nx, in.motion_y, nz};
        return decision;
    }
};
} // namespace slinkywinky::reference_compat
