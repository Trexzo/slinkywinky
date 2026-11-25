#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace slinkywinky::reference_compat {
struct LagRangePoint {double x=0,y=0,z=0;};
inline double lag_range_distance_sq(LagRangePoint a,LagRangePoint b)noexcept{
    const double x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return x*x+y*y+z*z;
}
struct LagRangeSettings {
    float delay_ms=150;
    bool require_weapon=false,flush_splash_potion=false,flush_sprint_reset=false;
    bool render_remote_pos=false;
};
struct LagRangeTickSample {
    bool in_game=false,weapon=false,splash_potion=false,sprinting=false;
    bool target=false,hit_this_tick=false,third_person=false;
    int target_hurt=0;
    double target_distance_sq=0;
    float sampled_max_range=10;
    LagRangePoint target_server_position{};
};
enum class LagRangePacketType {other, movement, attack, stop_sprinting};
struct LagRangePacketSample {
    LagRangePacketType kind=LagRangePacketType::other;
    int sequence=0;
    std::int64_t held_ms=0;
    bool movement_has_position=false,prior_verdict_zero=true;
    LagRangePoint movement_position{};
};
struct LagRangePacketProposal{
    bool hold=false;
    bool released_on_attack=false;
    bool released_on_sprint=false;
    bool position_recorded=false;
};
class LagRangeController {
public:
    void configure(LagRangeSettings s){
        if(!std::isfinite(s.delay_ms)||s.delay_ms<0||s.delay_ms>65535)
            throw std::invalid_argument("bad Lag Range delay");
        settings_=s;
    }
    void reset_world()noexcept{
        active_=closer_=hold_active_=track_sprint_=just_released_=false;
        show_box_=last_sprint_=hit_this_tick_=false;
        pause_ticks_=0;
        last_sequence_=std::numeric_limits<int>::min();
        latest_sent_position_={}; target_position_={};
    }
    void attack_phase(bool phase_one,bool hit_at_current_tick)noexcept{
        if(phase_one)hit_this_tick_=hit_at_current_tick;
    }
    void tick(const LagRangeTickSample& f){
        if(!std::isfinite(f.target_distance_sq) || !std::isfinite(f.sampled_max_range) ||
           f.sampled_max_range<10 || f.sampled_max_range>15 ||
           !finite(f.target_server_position))
            throw std::invalid_argument("bad Lag Range frame");
        const bool hit=f.hit_this_tick||hit_this_tick_;
        hit_this_tick_=false;
        bool eligible=f.in_game&&(!settings_.require_weapon||f.weapon);
        if(eligible&&settings_.flush_splash_potion){
            if(f.splash_potion){pause_ticks_=15;eligible=false;}
            else if(pause_ticks_>0){--pause_ticks_;eligible=false;}
        }
        if(eligible){
            const bool stopped_sprint=last_sprint_&&settings_.flush_sprint_reset&&!f.sprinting;
            last_sprint_=f.sprinting;
            if(f.target&&f.target_distance_sq<=static_cast<double>(f.sampled_max_range)*f.sampled_max_range){
                active_=true;budget_ms_=static_cast<std::int64_t>(settings_.delay_ms);
                hold_active_=hit&&f.target_hurt<5;
                track_sprint_=stopped_sprint;
                target_position_=f.target_server_position;
                show_box_=settings_.render_remote_pos&&f.third_person&&closer_;
                return;
            }
        }
        active_=hold_active_=track_sprint_=just_released_=show_box_=false;
    }
    LagRangePacketProposal packet(const LagRangePacketSample& p){
        if(!finite(p.movement_position))throw std::invalid_argument("bad movement position");
        LagRangePacketProposal out{};
        if(p.sequence>last_sequence_){
            if(active_&&p.kind==LagRangePacketType::movement&&p.movement_has_position){
                closer_=lag_range_distance_sq(target_position_,latest_sent_position_)>
                        lag_range_distance_sq(target_position_,p.movement_position);
            }
            last_sequence_=p.sequence;
        }
        if(active_&&closer_&&!hold_active_&&!just_released_&&!track_sprint_&&
           p.held_ms<budget_ms_){
            out.hold=true;
        }else if(p.prior_verdict_zero){
            if(p.kind==LagRangePacketType::movement){
                just_released_=false;
                if(p.movement_has_position){latest_sent_position_=p.movement_position;out.position_recorded=true;}
            }else if(p.kind==LagRangePacketType::attack&&hold_active_){
                hold_active_=false;just_released_=true;out.released_on_attack=true;
            }else if(p.kind==LagRangePacketType::stop_sprinting&&track_sprint_){
                track_sprint_=false;just_released_=true;out.released_on_sprint=true;
            }
        }
        return out;
    }
    bool active()const noexcept{return active_;}
    bool moving_closer()const noexcept{return closer_;}
    bool render_box()const noexcept{return show_box_;}
    int pause_ticks()const noexcept{return pause_ticks_;}
    bool awaiting_attack_release()const noexcept{return hold_active_;}
private:
    static bool finite(LagRangePoint p)noexcept{
        return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
    }
    LagRangeSettings settings_{};
    bool active_=false,closer_=false,hold_active_=false,track_sprint_=false;
    bool just_released_=false,show_box_=false,last_sprint_=false,hit_this_tick_=false;
    int pause_ticks_=0,last_sequence_=std::numeric_limits<int>::min();
    std::int64_t budget_ms_=0;
    LagRangePoint latest_sent_position_{},target_position_{};
};
} // namespace slinkywinky::reference_compat
