#pragma once
// R181: independently authored Instant Stop *offline decision*.
// Uses a verified candidate movement comparison, never touches game keys,
// JNI, native hooks, player memory, or upstream proprietary code.
#include "slinkywinky/null_move_controller.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>

namespace slinkywinky::reference_compat {

enum class InstantStopPhase : unsigned char { idle, watching_forward, counter_active };
enum class InstantStopChoice : unsigned char {
    none, back_right, back_only, back_left
};

struct InstantStopInput {
    bool in_game = false;
    bool forward_pressed = false; // effective forward key state
    bool on_ground = false;
    bool sprinting = false;
    std::array<bool, kMovementKeys> physically_held{}; // F L B R
    double motion_x = 0.0;
    double motion_z = 0.0;
    float yaw_degrees = 0.0f;
    float land_movement_factor = 0.0f;
};

struct InstantStopDecision {
    InstantStopChoice choice = InstantStopChoice::none;
    InstantStopPhase phase = InstantStopPhase::idle;
    std::array<std::optional<bool>, kMovementKeys> overrides{};
    double predicted_speed_squared = 0.0;
};

class InstantStopController final {
public:
    InstantStopDecision evaluate(const InstantStopInput& in) noexcept {
        InstantStopDecision out;
        out.phase = phase_;
        out.predicted_speed_squared = squared(in.motion_x, in.motion_z);
        if (!in.in_game) {
            // Fail-closed world exit: stale simulated counter key ownership
            // must not continue. This is stricter than the candidate's
            // observable out-of-game branch; actual game key reset is NOT done.
            if (phase_ == InstantStopPhase::counter_active)
                restore(in, out);
            reset();
            out.phase = phase_;
            return out;
        }
        if (phase_ == InstantStopPhase::counter_active) {
            restore(in, out);
            phase_ = InstantStopPhase::idle;
            out.phase = phase_;
            return out;
        }
        if (phase_ == InstantStopPhase::idle) {
            if (in.forward_pressed) phase_ = InstantStopPhase::watching_forward;
            out.phase = phase_;
            return out;
        }
        // In flight, or while forward remains held, preserve the watch phase.
        if (in.forward_pressed || !in.on_ground) return out;
        // Reject NaN/Inf and negative movement factors; do not generate a
        // potentially dangerous synthetic counter-key proposal.
        if (!std::isfinite(in.motion_x) || !std::isfinite(in.motion_z) ||
            !std::isfinite(in.yaw_degrees) ||
            !std::isfinite(in.land_movement_factor) ||
            in.land_movement_factor < 0.0f) {
            phase_ = InstantStopPhase::idle;
            out.phase = phase_;
            return out;
        }
        const double speed = static_cast<double>(in.land_movement_factor) /
                             (in.sprinting ? 1.3 : 1.0);
        double best = out.predicted_speed_squared;
        InstantStopChoice chosen = InstantStopChoice::none;
        for (const auto [strafe, choice] : {
                 std::pair<float, InstantStopChoice>{-1.0f, InstantStopChoice::back_right},
                 std::pair<float, InstantStopChoice>{ 0.0f, InstantStopChoice::back_only},
                 std::pair<float, InstantStopChoice>{ 1.0f, InstantStopChoice::back_left}
             }) {
            const auto delta = predicted_push(in.yaw_degrees, -1.0f, strafe, speed);
            const double candidate = squared(in.motion_x + delta[0],
                                             in.motion_z + delta[1]);
            // Strict improvement ensures the original candidate's
            // back-right -> back-only -> back-left tie preference.
            if (candidate < best) {
                best = candidate;
                chosen = choice;
            }
        }
        out.predicted_speed_squared = best;
        out.choice = chosen;
        if (chosen != InstantStopChoice::none) {
            out.overrides[static_cast<unsigned>(MoveKey::back)] = true;
            out.overrides[static_cast<unsigned>(MoveKey::left)] =
                chosen == InstantStopChoice::back_left;
            out.overrides[static_cast<unsigned>(MoveKey::right)] =
                chosen == InstantStopChoice::back_right;
            phase_ = InstantStopPhase::counter_active;
        } else {
            phase_ = InstantStopPhase::idle;
        }
        out.phase = phase_;
        return out;
    }

    void reset() noexcept { phase_ = InstantStopPhase::idle; }
    InstantStopPhase phase() const noexcept { return phase_; }

private:
    static double squared(double x, double z) noexcept { return x*x + z*z; }

    // Derived from the original candidate's public movement math invariants;
    // numerical rounding is not claimed to be bit-for-bit original.
    static std::array<double, 2> predicted_push(float yaw, float forward,
                                                 float strafe, double speed) noexcept {
        constexpr float friction = 0.9800000190734863f;
        constexpr float one = 0.9999998211860657f;
        constexpr double pi = 3.14159265358979323846;
        forward *= friction;
        strafe *= friction;
        const float magnitude_squared = forward*forward + strafe*strafe;
        if (magnitude_squared < 0.0001f) return {0.0, 0.0};
        const double normalizer = std::max(1.0,
                                          std::sqrt(static_cast<double>(magnitude_squared)));
        const double k = speed * static_cast<double>(one) / normalizer;
        const double f = static_cast<double>(forward) * k;
        const double s = static_cast<double>(strafe) * k;
        const double radians = static_cast<double>(yaw) * pi / 180.0;
        const double sn = std::sin(radians), cs = std::cos(radians);
        return {s*cs - f*sn, f*cs + s*sn};
    }

    static void restore(const InstantStopInput& in,
                        InstantStopDecision& out) noexcept {
        for (auto key : {MoveKey::back, MoveKey::left, MoveKey::right})
            out.overrides[static_cast<unsigned>(key)] =
                in.physically_held[static_cast<unsigned>(key)];
    }
    InstantStopPhase phase_ = InstantStopPhase::idle;
};
} // namespace slinkywinky::reference_compat
