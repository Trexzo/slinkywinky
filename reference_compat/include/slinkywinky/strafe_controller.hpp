#pragma once

// R222: independently authored offline steering policy for the candidate
// Strafe module. Evaluates typed observations only: no Minecraft/JNI writes.
// The public fd9c592 reference defines two native callbacks, but no
// original commercial source or executable instructions are included.
#include <cmath>
#include <optional>

namespace slinkywinky::reference_compat {

inline constexpr unsigned kStrafeSnapshotEvent = 21;
inline constexpr unsigned kStrafeSteeringEvent = 22;

struct StrafeSettings {
    double max_hurt_time = 10.0;
    double strength_air = 1.0;
    double strength_jump = 1.0;
    double strength_ground = 1.0;
    bool require_weapon = false;
};

struct StrafeMotionInput {
    float forward = 0.0f;
    float strafe = 0.0f;
    bool jump_phase = false;
    bool on_ground = true;
    bool weapon_accepted = true;
    int hurt_time = 0;
    float yaw_degrees = 0.0f;
    double motion_x = 0.0;
    double motion_y = 0.0;
    double motion_z = 0.0;
    StrafeSettings settings{};
};

struct StrafeMotionProposal {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

struct StrafeMotionDecision {
    std::optional<StrafeMotionProposal> proposed_motion{};
};

class StrafeController final {
public:
    void observe_sprinting(bool sprinting) noexcept {
        last_sprinting_ = sprinting;
        observed_ = true;
    }

    void reset() noexcept {
        last_sprinting_ = false;
        observed_ = false;
    }

    bool has_snapshot() const noexcept { return observed_; }

    StrafeMotionDecision evaluate(const StrafeMotionInput& in) const noexcept {
        StrafeMotionDecision out;
        if (!observed_ || (in.forward == 0.0f && in.strafe == 0.0f) ||
            (in.settings.require_weapon && !in.weapon_accepted) ||
            in.hurt_time < 0 || !std::isfinite(in.settings.max_hurt_time) ||
            static_cast<double>(in.hurt_time) > in.settings.max_hurt_time)
            return out;

        if (!std::isfinite(in.forward) || !std::isfinite(in.strafe) ||
            !std::isfinite(in.yaw_degrees) ||
            !std::isfinite(in.motion_x) || !std::isfinite(in.motion_y) ||
            !std::isfinite(in.motion_z) ||
            !std::isfinite(in.settings.strength_air) ||
            !std::isfinite(in.settings.strength_jump) ||
            !std::isfinite(in.settings.strength_ground))
            return out;

        const double strength = in.on_ground ?
            (in.jump_phase ? in.settings.strength_jump :
                             in.settings.strength_ground) :
            in.settings.strength_air;
        // Reject malformed settings rather than asking a future live
        // action adapter to extrapolate or store non-finite motion.
        if (strength <= 0.0 || strength > 1.0)
            return out;

        const bool jump_boost = in.on_ground && in.jump_phase && !last_sprinting_;
        const double horizontal_speed =
            std::hypot(in.motion_x, in.motion_z) + (jump_boost ? 0.2 : 0.0);
        if (!std::isfinite(horizontal_speed) || horizontal_speed == 0.0)
            return out;

        double sideways = in.strafe > 0.0f ? 1.0 :
                          (in.strafe < 0.0f ? -1.0 : 0.0);
        double heading = static_cast<double>(in.yaw_degrees);
        if (in.forward > 0.0f) {
            sideways *= -45.0;
        } else if (in.forward < 0.0f) {
            heading += 180.0;
            sideways *= 45.0;
        } else {
            sideways *= -90.0;
        }
        const double radians = (heading + sideways) *
                               (3.14159265358979323846 / 180.0);
        const double target_x = -std::sin(radians) * horizontal_speed;
        const double target_z =  std::cos(radians) * horizontal_speed;
        const double new_x = in.motion_x + (target_x - in.motion_x) * strength;
        const double new_z = in.motion_z + (target_z - in.motion_z) * strength;
        if (!std::isfinite(new_x) || !std::isfinite(new_z))
            return out;
        out.proposed_motion = StrafeMotionProposal{new_x, in.motion_y, new_z};
        return out;
    }

private:
    bool observed_ = false;
    bool last_sprinting_ = false;
};

} // namespace slinkywinky::reference_compat
