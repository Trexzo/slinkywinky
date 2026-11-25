#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace slinkywinky::reference_compat {
struct BacktrackSettings {
    float delay_ms=150, cooldown_ticks=5, max_target_hurt=10;
    float min_range=0, max_range=6;
    bool require_mouse=false, require_weapon=false;
    bool disable_on_hit=false, render_remote_pos=false;
};
struct BacktrackSample {
    bool in_game=false, mouse=false, weapon=false;
    int player_ticks=0, last_attack_tick=0;
    bool target=false, source_reach_eligible=false;
    int target_id=0, own_id=0, target_hurt=0;
    double distance_sq=0;
    double real_position_minus_shown_distance=0;
};
enum class BacktrackPacketType { other, target_status, update_health, sound_effect, entity_velocity, teleport };
struct BacktrackPacketSample {
    BacktrackPacketType kind=BacktrackPacketType::other;
    std::int64_t held_ms=0;
    int sequence=0;
    int entity_id=0; // status/velocity target: source compares exact entity IDs
    bool prior_verdict_zero=true;
};
enum class BacktrackPacketVerdict { unchanged, hold, pass_feedback };
struct BacktrackPacketProposal {
    BacktrackPacketVerdict verdict=BacktrackPacketVerdict::unchanged;
    bool sample_held_movement=false;
    bool sample_released_movement=false;
    bool stopped=false;
};
class BacktrackController {
public:
    void configure(BacktrackSettings s) {
        if(!std::isfinite(s.delay_ms)||!std::isfinite(s.cooldown_ticks)||
           !std::isfinite(s.max_target_hurt)||!std::isfinite(s.min_range)||
           !std::isfinite(s.max_range)||s.delay_ms<0||s.delay_ms>65535||
           s.cooldown_ticks<0||s.cooldown_ticks>100000||
           s.min_range<0||s.max_range<s.min_range||s.max_range>128)
            throw std::invalid_argument("bad Backtrack settings");
        settings_=s;
    }
    void reset_world() noexcept {
        active_=render_box_=false;
        cooldown_=0;
        target_id_=own_id_=0;
        last_recorded_hold_seq_=last_recorded_release_seq_=std::numeric_limits<int>::min();
    }
    void tick(const BacktrackSample& f) {
        if(!std::isfinite(f.distance_sq)||!std::isfinite(f.real_position_minus_shown_distance))
            throw std::invalid_argument("bad Backtrack frame");
        if(cooldown_>0) {
            const int old=cooldown_--;
            if(old!=1)return; // original final cooldown tick evaluates eligibility
        }
        const std::int64_t age=static_cast<std::int64_t>(f.player_ticks)-f.last_attack_tick;
        const bool eligible=f.in_game&&(!settings_.require_mouse||f.mouse)&&
            (!settings_.require_weapon||f.weapon)&&age<=20&&f.target&&
            f.target_hurt<=settings_.max_target_hurt&&f.distance_sq>=
            static_cast<double>(settings_.min_range)*settings_.min_range&&
            f.distance_sq<=static_cast<double>(settings_.max_range)*settings_.max_range;
        if(!eligible) {
            if(active_) {active_=false;cooldown_=static_cast<int>(settings_.cooldown_ticks);}
            render_box_=false;
            return;
        }
        const int old_target=target_id_;
        target_id_=f.target_id;own_id_=f.own_id;
        if(!active_) {
            if(!f.source_reach_eligible){render_box_=false;return;}
            active_=true;render_box_=settings_.render_remote_pos;
            last_recorded_hold_seq_=last_recorded_release_seq_=std::numeric_limits<int>::min();
            return;
        }
        if(f.real_position_minus_shown_distance < -0.05 || old_target!=f.target_id){
            active_=false;render_box_=false;
            cooldown_=static_cast<int>(settings_.cooldown_ticks);
        }
    }
    BacktrackPacketProposal packet(const BacktrackPacketSample& p) noexcept {
        BacktrackPacketProposal out{};
        if(!active_)return out;
        if((p.kind==BacktrackPacketType::target_status &&
            p.entity_id==target_id_) ||
           p.kind==BacktrackPacketType::update_health||
           p.kind==BacktrackPacketType::sound_effect){
            out.verdict=BacktrackPacketVerdict::pass_feedback;return out;
        }
        if(p.kind==BacktrackPacketType::teleport ||
           (settings_.disable_on_hit&&p.kind==BacktrackPacketType::entity_velocity &&
            p.entity_id==own_id_)){
            active_=false;render_box_=false;out.stopped=true;return out;
        }
        if(p.held_ms<=static_cast<std::int64_t>(settings_.delay_ms)){
            out.verdict=BacktrackPacketVerdict::hold;
            if(p.sequence>last_recorded_hold_seq_){
                last_recorded_hold_seq_=p.sequence;
                out.sample_held_movement=true;
            }
        } else if(p.prior_verdict_zero&&p.sequence>last_recorded_release_seq_){
            last_recorded_release_seq_=p.sequence;
            out.sample_released_movement=true;
        }
        return out;
    }
    bool active() const noexcept{return active_;}
    bool render_box() const noexcept{return render_box_;}
    int cooldown() const noexcept{return cooldown_;}
    int target_id() const noexcept{return target_id_;}
private:
    BacktrackSettings settings_{};
    bool active_=false,render_box_=false;
    int cooldown_=0,target_id_=0,own_id_=0;
    int last_recorded_hold_seq_=std::numeric_limits<int>::min();
    int last_recorded_release_seq_=std::numeric_limits<int>::min();
};
} // namespace slinkywinky::reference_compat
