#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace slinkywinky::reference_compat {
enum class TimerRangeMode : unsigned{freeze_after=0,freeze_before=1};
struct TimerRangeSettings{
    TimerRangeMode mode=TimerRangeMode::freeze_after;
    float duration_ticks=5,duration_legit_ticks=4,min_dist=2;
    float cooldown_seconds=0.5f;
    bool block_while_frozen=false,require_mouse=false,require_weapon=false,require_not_using_item=false;
};
struct TimerRangeTick{
    bool in_game=false,target=false,player_on_ground=false,mouse=false,weapon=false;
    bool using_item=false,jumping=false,line_of_sight=false,entity_under_crosshair=false;
    int target_hurt_time=0;
    double target_distance_sq=0,motion_y=0;
    float forward_input=0;
};
struct TimerRangeRayStep{
    bool hitbox_intersects=false;
    double nearest_distance_sq=0;
};
struct TimerRangeProposal{
    bool write_speed=false;
    float speed=1.0f;
    bool write_elapsed_ticks=false;
    float elapsed_ticks=0;
    bool hold_packets=false;
    bool write_use_key=false,use_key=false;
    bool attack_click=false;
    bool burst_planned=false;
};
class TimerRangeController{
public:
    void configure(TimerRangeSettings s){
        if(static_cast<unsigned>(s.mode)>1||!std::isfinite(s.duration_ticks)||
           !std::isfinite(s.duration_legit_ticks)||!std::isfinite(s.min_dist)||
           !std::isfinite(s.cooldown_seconds)||s.duration_ticks<0||
           s.duration_ticks>10000||s.duration_legit_ticks<0||
           s.duration_legit_ticks>10000||s.min_dist<0||s.min_dist>128||
           s.cooldown_seconds<0||s.cooldown_seconds>10000)
            throw std::invalid_argument("bad Timer Range settings");
        settings_=s;
    }
    TimerRangeProposal reset_world()noexcept{
        TimerRangeProposal out{};
        if(timer_modified_){out.write_speed=true;out.speed=1;}
        if(use_key_held_){out.write_use_key=true;out.use_key=false;}
        timer_modified_=false;hold_packets_=use_key_held_=eligible_=false;
        burst_ticks_=0;freeze_time_left_=cooldown_left_=0;
        return out;
    }
    TimerRangeProposal disable()noexcept{return reset_world();}
    TimerRangeProposal tick(const TimerRangeTick& f){
        if(!std::isfinite(f.target_distance_sq)||!std::isfinite(f.motion_y)||
           !std::isfinite(f.forward_input))throw std::invalid_argument("bad Timer Range tick");
        TimerRangeProposal out{};
        if(burst_ticks_>0){
            --burst_ticks_;
            if(burst_ticks_==0&&settings_.mode==TimerRangeMode::freeze_after){
                start_freeze(out,f.weapon);
            }
            if(f.in_game&&f.target&&f.entity_under_crosshair)out.attack_click=true;
            return out;
        }
        eligible_=f.in_game&&f.target&&f.target_distance_sq>=3.0&&
            f.player_on_ground&&f.target_hurt_time<=1&&
            (!settings_.require_mouse||f.mouse)&&(!settings_.require_weapon||f.weapon)&&
            (!settings_.require_not_using_item||!f.using_item)&&
            f.motion_y<=0&&!f.jumping&&f.line_of_sight&&f.forward_input>0;
        return out;
    }
    TimerRangeProposal frame(double dt,float partial,const std::vector<TimerRangeRayStep>& steps,
                             bool sword_held){
        if(!std::isfinite(dt)||dt<0||dt>60||!std::isfinite(partial)||
           partial<0||partial>1)throw std::invalid_argument("bad Timer Range frame time");
        TimerRangeProposal out{};
        if(timer_modified_){
            freeze_time_left_-=dt;
            if(freeze_time_left_>0){out.write_speed=true;out.speed=0;return out;}
            timer_modified_=false;hold_packets_=false;
            out.write_speed=true;out.speed=1;
            if(use_key_held_){out.write_use_key=true;out.use_key=false;use_key_held_=false;}
            if(settings_.mode==TimerRangeMode::freeze_before){
                out.write_elapsed_ticks=true;out.elapsed_ticks=static_cast<float>(burst_ticks_);
            }
        }
        if(cooldown_left_>0){cooldown_left_-=dt;return out;}
        if(!eligible_)return out;
        const int count=static_cast<int>(settings_.mode==TimerRangeMode::freeze_after?
                                         settings_.duration_ticks:settings_.duration_legit_ticks);
        for(int i=1;i<=count && static_cast<std::size_t>(i)<=steps.size();++i){
            const auto& s=steps[static_cast<std::size_t>(i-1)];
            if(!std::isfinite(s.nearest_distance_sq)||s.nearest_distance_sq<0)
                throw std::invalid_argument("bad Timer Range ray step");
            if(!s.hitbox_intersects||s.nearest_distance_sq<=0)continue;
            if(s.nearest_distance_sq<static_cast<double>(settings_.min_dist)*settings_.min_dist)
                return out;
            if(settings_.mode==TimerRangeMode::freeze_before){
                start_freeze(out,sword_held);
            } else {
                out.write_elapsed_ticks=true;out.elapsed_ticks=static_cast<float>(i);
            }
            burst_ticks_=i;
            freeze_time_left_=static_cast<double>(static_cast<float>(i)-partial)*0.05;
            cooldown_left_=settings_.cooldown_seconds;
            out.burst_planned=true;out.hold_packets=hold_packets_;
            return out;
        }
        return out;
    }
    bool packet_hold()const noexcept{return hold_packets_;}
    bool frozen()const noexcept{return timer_modified_;}
    int burst_ticks()const noexcept{return burst_ticks_;}
    double cooldown_remaining()const noexcept{return cooldown_left_;}
private:
    void start_freeze(TimerRangeProposal& out,bool sword_held)noexcept{
        timer_modified_=true;
        out.write_speed=true;out.speed=0;
        if(settings_.mode==TimerRangeMode::freeze_before)hold_packets_=true;
        else if(settings_.block_while_frozen&&sword_held){
            use_key_held_=true;out.write_use_key=true;out.use_key=true;
        }
        out.hold_packets=hold_packets_;
    }
    TimerRangeSettings settings_{};
    bool timer_modified_=false,hold_packets_=false,use_key_held_=false,eligible_=false;
    int burst_ticks_=0;
    double freeze_time_left_=0,cooldown_left_=0;
};
} // namespace slinkywinky::reference_compat
