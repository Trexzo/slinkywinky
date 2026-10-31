#include "slinkywinky/combat.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace slinkywinky {
namespace {
constexpr double pi = 3.14159265358979323846;
double wrap_degrees(double d) { return std::remainder(d, 360.0); }
bool valid_target(const TickSnapshot& s, double range) {
    if (!s.target || !s.target->alive || !s.target->visible) return false;
    const auto dx = s.target->position.x - s.player_eye.x;
    const auto dy = s.target->position.y - s.player_eye.y;
    const auto dz = s.target->position.z - s.player_eye.z;
    return dx*dx+dy*dy+dz*dz <= range*range;
}
float constrain(float value, float lower, float upper, float fallback) {
    return std::isfinite(value) ? std::max(lower, std::min(upper,value)) : fallback;
}
}
CombatCoordinator::CombatCoordinator(std::array<std::uint32_t, 4> seed) : random_(seed) {}
void CombatCoordinator::reset_world() noexcept {
    reach_.reset();
    rotations_.start_frame();
    next_press_ms_ = release_ms_ = last_ms_ = 0;
    last_target_id_ = -1;
    block_cooldown_ = 0;
    click_down_ = block_down_ = false;
    have_snapshot_ = false;
}
CombatResult CombatCoordinator::disable(bool physical_left, bool physical_right) {
    reset_world();
    CombatResult result;
    result.left_edge = left_.disable_synthetic(physical_left);
    result.right_edge = right_.disable_synthetic(physical_right);
    result.left_held = left_.effective_down();
    result.right_held = right_.effective_down();
    return result;
}
CombatResult CombatCoordinator::tick(const TickSnapshot& s, const CombatSettings& settings) {
    if (!s.world_valid || (have_snapshot_ && (s.generation != generation_ || s.elapsed_ms < last_ms_))) {
        auto result = disable(s.mouse_left, s.mouse_right);
        generation_ = s.generation;
        last_ms_ = s.elapsed_ms;
        have_snapshot_ = s.world_valid;
        return result;
    }
    generation_ = s.generation;
    last_ms_ = s.elapsed_ms;
    have_snapshot_ = true;
    CombatResult out;
    rotations_.start_frame();
    if (s.gui_open) {
        // Never synthesize input while a GUI is open.
        click_down_ = block_down_ = false;
    } else {
        const float cps = constrain(settings.clicks_per_second, 1.0f, 30.0f, 12.0f);
        const bool eligible_click = settings.auto_clicker_enabled && s.holding_weapon &&
            (!settings.require_mouse_down || s.mouse_left);
        if (!eligible_click) {
            click_down_ = false;
            next_press_ms_ = 0;
        } else if (click_down_ && s.elapsed_ms >= release_ms_) {
            click_down_ = false;
        } else if (!click_down_ && (next_press_ms_ == 0 || s.elapsed_ms >= next_press_ms_)) {
            click_down_ = true;
            const auto interval = static_cast<std::uint64_t>(std::round(1000.0f / cps));
            // Unbiased bounded jitter avoids zero-length press/release edges.
            const auto jitter = static_cast<int>(random_.next() % 9) - 4;
            const auto safe_interval = static_cast<std::uint64_t>(std::max(24, static_cast<int>(interval) + jitter));
            release_ms_ = s.elapsed_ms + std::max<std::uint64_t>(8, safe_interval / 3);
            next_press_ms_ = s.elapsed_ms + safe_interval;
            out.attempt_attack = true;
        }
        if (settings.auto_block_enabled && s.target && s.target->alive &&
            valid_target(s, std::max(0.0f, settings.block_range)) && s.holding_weapon) {
            if (block_cooldown_ > 0) --block_cooldown_;
            const float max_hurt = constrain(settings.block_max_hurt_time, 0, 10, 5);
            const auto floor_hurt = static_cast<int>(std::floor(max_hurt));
            const auto fractional = max_hurt - static_cast<float>(floor_hurt);
            const auto allowed_hurt = floor_hurt + (random_.unit_float() < fractional ? 1 : 0);
            const bool requested = !settings.block_requires_damage ||
                (s.hurt_time > 0 && s.hurt_time <= allowed_hurt);
            if (!requested && block_down_) block_cooldown_ = 10;
            if (!requested) block_down_ = false;
            else if (block_cooldown_ == 0) block_down_ = true;
        } else {
            block_down_ = false;
            block_cooldown_ = 0;
        }
        if (settings.aim_enabled && valid_target(s, std::max(0.f, settings.aim_range)) &&
            (!settings.aim_requires_mouse || s.mouse_left)) {
            const auto& p = s.target->position;
            const double dx = p.x - s.player_eye.x;
            const double dy = p.y - s.player_eye.y;
            const double dz = p.z - s.player_eye.z;
            const double horizontal = std::hypot(dx, dz);
            if (horizontal > 1e-8) {
                const double desired_yaw = std::atan2(dz, dx)*180.0/pi-90;
                const double desired_pitch = -std::atan2(dy,horizontal)*180.0/pi;
                const double diff_yaw = wrap_degrees(desired_yaw-s.yaw);
                const double diff_pitch = desired_pitch-s.pitch;
                const double angle = std::hypot(diff_yaw, diff_pitch);
                if (angle <= constrain(settings.aim_fov, 0,180,90)) {
                    const double hspeed = constrain(settings.aim_horizontal_speed, 0,100,15)/100.0;
                    const double vspeed = constrain(settings.aim_vertical_speed, 0,100,15)/100.0;
                    rotations_.submit({"aim", 100,static_cast<float>(s.yaw+diff_yaw*hspeed),
                         static_cast<float>(std::max(-90.0, std::min(90.0, s.pitch+diff_pitch*vspeed)))});
                }
            }
        }
    }
    // Hit Select gates the synthetic input edge as well as the attack pulse.
    // Never interfere with the player's physical mouse state.
    if (settings.hit_select_enabled && s.hurt_time < settings.hit_select_min_hurt) {
        out.attempt_attack = false;
        click_down_ = false;
    }
    out.left_edge = left_.update(s.mouse_left, click_down_);
    out.right_edge = right_.update(s.mouse_right, block_down_);
    out.left_held = left_.effective_down();
    out.right_held = right_.effective_down();
    if (settings.reach_enabled && !s.gui_open)
        out.requested_reach = reach_.evaluate({s.sprinting, s.last_attacker_time}, settings.reach);
    out.rotation = rotations_.winner();
    return out;
}
std::optional<Knockback> decide_velocity(Knockback in, const VelocitySettings& s,
                                          float random) {
    if (!s.enabled || !std::isfinite(random) || random < 0 || random >= 1 ||
        !std::isfinite(in.x) || !std::isfinite(in.y) || !std::isfinite(in.z))
        return std::nullopt;
    if (!std::isfinite(s.horizontal) || !std::isfinite(s.vertical) ||
        !std::isfinite(s.chance_percent) || s.horizontal < 0 || s.horizontal > 1 ||
        s.vertical < 0 || s.vertical > 1 || s.chance_percent < 0 || s.chance_percent > 100) return std::nullopt;
    if (random*100.0f >= s.chance_percent) return std::nullopt;
    return Knockback{in.x*s.horizontal, in.y*s.vertical, in.z*s.horizontal};
}
} // namespace slinkywinky
