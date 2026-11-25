#pragma once
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace slinkywinky::reference_compat {
struct KnockbackDelaySettings {
    unsigned max_delay_ms = 0;
    bool require_air = false;
    float range = 6;
    bool require_weapon = false;
    bool require_mouse = false;
    bool require_target = false;
};
struct KnockbackDelayFrame {
    bool player = false;
    int own_entity_id = 0;
    bool on_ground = true;
    bool has_combat_target = false;
    double combat_distance_sq = 0;
    bool weapon = false, mouse = false, player_in_front = false;
    bool random_chance_passed = false;
};
struct KnockbackDelayPacket {
    bool previous_verdict_zero = true;
    bool is_velocity = false;
    int velocity_entity_id = 0;
    std::int64_t held_ms = 0;
    int transaction_id = 0;
    bool hurt_feedback = false;
};
enum class KnockbackDelayVerdict { unchanged, hold, hold_feedback };

class KnockbackDelayController {
public:
    void configure(KnockbackDelaySettings s) {
        if (!std::isfinite(s.range) || s.range < 0 || s.range > 128 ||
            s.max_delay_ms > 65535)
            throw std::invalid_argument("invalid Knockback Delay settings");
        settings_ = s;
    }
    void reset_world() noexcept {
        allowed_ = false;
        in_air_ = false;
        holding_ = false;
        held_id_ = 0;
        own_entity_id_ = 0;
    }
    void late_tick(const KnockbackDelayFrame& f) noexcept {
        if (!f.player) return;
        own_entity_id_ = f.own_entity_id;
        in_air_ = settings_.require_air && !f.on_ground;
        allowed_ = false;
        if (settings_.require_air && !in_air_) holding_ = false;
        else if (f.has_combat_target &&
                 std::isfinite(f.combat_distance_sq) &&
                 f.combat_distance_sq <= static_cast<double>(settings_.range) * settings_.range &&
                 (!settings_.require_weapon || f.weapon) &&
                 (!settings_.require_mouse || f.mouse) &&
                 (!settings_.require_target || f.player_in_front))
            allowed_ = f.random_chance_passed;
    }
    KnockbackDelayVerdict packet(const KnockbackDelayPacket& p) noexcept {
        if (!p.previous_verdict_zero) return KnockbackDelayVerdict::unchanged;
        const bool own = p.is_velocity && p.velocity_entity_id == own_entity_id_;
        bool hold = false;
        if (in_air_) {
            hold = (p.held_ms == 0 && allowed_);
            holding_ = hold;
        } else if (allowed_ && own) {
            holding_ = true;
            hold = true;
        } else hold = holding_;
        if (own && hold) {
            if (held_id_ == 0) held_id_ = p.transaction_id;
            else if (held_id_ != p.transaction_id) {
                stop_holding();
                return KnockbackDelayVerdict::unchanged;
            }
        } else if (!hold) return KnockbackDelayVerdict::unchanged;
        if (p.held_ms > static_cast<std::int64_t>(settings_.max_delay_ms)) {
            stop_holding();
            return KnockbackDelayVerdict::unchanged;
        }
        return p.hurt_feedback ? KnockbackDelayVerdict::hold_feedback
                               : KnockbackDelayVerdict::hold;
    }
    bool allowed() const noexcept { return allowed_; }
    bool holding() const noexcept { return holding_; }
    int held_transaction_id() const noexcept { return held_id_; }
private:
    void stop_holding() noexcept {
        holding_ = false;
        allowed_ = false;
        held_id_ = 0;
    }
    KnockbackDelaySettings settings_{};
    bool in_air_ = false, allowed_ = false, holding_ = false;
    int own_entity_id_ = 0, held_id_ = 0;
};
} // namespace slinkywinky::reference_compat
