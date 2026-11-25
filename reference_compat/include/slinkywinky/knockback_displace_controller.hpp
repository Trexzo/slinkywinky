#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace slinkywinky::reference_compat {
// Read-only attack/rotation/key proposals from the candidate native logic.
struct KnockbackDisplaceSettings {
    float cooldown_ticks=0;
    float angle_degrees=90;
    unsigned direction=0; // left/right/random/strafe/against-strafe
    bool use_last_strafe=false;
    bool hide_rotation=false;
    bool require_no_kb=false, require_weapon=false;
    bool require_kb_enchant=false, require_facing_away=false;
};
struct KnockbackDisplaceAttack {
    bool phase_pre_attack=false, selected_enemy_player=false;
    bool timer_range_pending=false, combat_target_valid=false;
    bool accepted_weapon=false, kb_enchant=false, sprinting=false;
    bool facing_away=false, rotation_priority_available=false;
    float yaw=0, pitch=0;
    bool sampled_random_direction_positive=false;
    float sampled_angle_reduction=0; // caller's original 0..10 degree sample
    float sampled_pitch_magnitude=10; // caller's original 10..20 degree sample
    bool sampled_pitch_down=false;
};
struct KnockbackDisplaceProposal {
    bool cancel_attack=false, apply_rotation=false, hide_packets=false;
    float original_yaw=0, original_pitch=0, yaw_offset=0, pitch_offset=0;
};
struct KnockbackDisplaceLateProposal {
    bool restore_rotation=false, replay_one_click=false, packet_hold=false;
    float target_yaw=0, target_pitch=0;
};
class KnockbackDisplaceController {
public:
    void configure(KnockbackDisplaceSettings s) {
        if(s.direction>4 || !std::isfinite(s.cooldown_ticks) ||
           !std::isfinite(s.angle_degrees) || s.cooldown_ticks<0 ||
           s.cooldown_ticks>10000 || s.angle_degrees<0 ||
           s.angle_degrees>360)
            throw std::invalid_argument("invalid Knockback Displace settings");
        settings_=s;
    }
    void reset_world() noexcept {
        hidden_=displacing_=displaced_last_=hit_seen_=false;
        ticks_since_reset_=ticks_since_displacement_=0;
        last_strafe_sign_=0;
        previous_yaw_=previous_pitch_=0;
    }
    KnockbackDisplaceProposal attack(const KnockbackDisplaceAttack& f) {
        if(!std::isfinite(f.yaw) || !std::isfinite(f.pitch) ||
           !std::isfinite(f.sampled_angle_reduction) ||
           !std::isfinite(f.sampled_pitch_magnitude) ||
           f.sampled_angle_reduction<0 || f.sampled_angle_reduction>10 ||
           f.sampled_pitch_magnitude<10 || f.sampled_pitch_magnitude>20)
            throw std::invalid_argument("invalid Knockback Displace attack sample");
        KnockbackDisplaceProposal out{};
        if(displacing_){out.cancel_attack=true;return out;}
        if(!f.phase_pre_attack || !f.selected_enemy_player)return out;
        hit_seen_=true;
        if(displaced_last_ || ticks_since_reset_<9 ||
           ticks_since_displacement_<settings_.cooldown_ticks ||
           f.timer_range_pending || (settings_.require_no_kb && f.combat_target_valid) ||
           (settings_.require_weapon && !f.accepted_weapon) ||
           (!f.kb_enchant && (settings_.require_kb_enchant || !f.sprinting)) ||
           (settings_.require_facing_away && !f.facing_away) ||
           !f.rotation_priority_available) return out;
        int dir=0;
        switch(settings_.direction){
            case 0:dir=-1;break;
            case 1:dir=1;break;
            case 2:dir=f.sampled_random_direction_positive?1:-1;break;
            case 3:dir=last_strafe_sign_;break;
            case 4:dir=-last_strafe_sign_;break;
        }
        if(!dir)return out;
        previous_yaw_=f.yaw;previous_pitch_=f.pitch;
        displacing_=true;
        hidden_=settings_.hide_rotation;
        ticks_since_displacement_=0;
        out.cancel_attack=true;out.apply_rotation=true;
        out.hide_packets=hidden_;out.original_yaw=f.yaw;out.original_pitch=f.pitch;
        out.yaw_offset=(settings_.angle_degrees-f.sampled_angle_reduction)*dir;
        out.pitch_offset=(f.sampled_pitch_down?1.0f:-1.0f)*f.sampled_pitch_magnitude;
        return out;
    }
    void post_attack(bool pre_attack_phase) noexcept {
        if(pre_attack_phase && ticks_since_reset_>=10 && hit_seen_)
            ticks_since_reset_=0;
    }
    KnockbackDisplaceLateProposal late_tick(bool in_game,float strafe,
                                           bool rotation_priority_available) {
        if(!std::isfinite(strafe))
            throw std::invalid_argument("invalid Knockback Displace strafe");
        if(ticks_since_reset_<std::numeric_limits<unsigned>::max())++ticks_since_reset_;
        if(ticks_since_displacement_<std::numeric_limits<unsigned>::max())
            ++ticks_since_displacement_;
        displaced_last_=displacing_;
        hit_seen_=false;
        KnockbackDisplaceLateProposal out{};
        if(in_game) {
            const int sign=(strafe>0)-(strafe<0);
            if(!settings_.use_last_strafe || sign!=0)last_strafe_sign_=sign;
            if(displacing_ && rotation_priority_available){
                out.restore_rotation=true;out.replay_one_click=true;
                out.target_yaw=previous_yaw_;out.target_pitch=previous_pitch_;
            }
        }
        displacing_=false;
        hidden_=displaced_last_ && settings_.hide_rotation;
        out.packet_hold=hidden_;
        return out;
    }
    bool packet_hold() const noexcept {return hidden_;}
    bool displacing() const noexcept {return displacing_;}
    unsigned reset_ticks() const noexcept {return ticks_since_reset_;}
    unsigned displacement_ticks() const noexcept {return ticks_since_displacement_;}
    int saved_strafe_sign() const noexcept {return last_strafe_sign_;}
private:
    KnockbackDisplaceSettings settings_{};
    bool hidden_=false,displacing_=false,displaced_last_=false,hit_seen_=false;
    unsigned ticks_since_reset_=0,ticks_since_displacement_=0;
    int last_strafe_sign_=0;
    float previous_yaw_=0,previous_pitch_=0;
};
} // namespace slinkywinky::reference_compat
