#pragma once
// R229: first-party source-derived Clutch click cadence, rotation ownership
// and late-tick snap-back. Candidate WORLD SELECTION and RNG are explicit,
// prevalidated caller observations, not reconstructed original JNI world hooks.
// Returns value-only proposals. No input, rotation, packet or gameplay writes.
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace slinkywinky::reference_compat {
struct ClutchBlock {
    int x=0,y=0,z=0;
};
inline bool operator==(const ClutchBlock& a,const ClutchBlock& b) noexcept {
    return a.x==b.x && a.y==b.y && a.z==b.z;
}

struct ClutchClickInput {
    bool in_game=false,hit_is_block=false,hit_top=false;
    ClutchBlock hit{},adjacent{};
    float clicks_per_second=10.0f;
};
struct ClutchClickDecision {
    bool admitted=false,proposed_use_click=false;
    std::optional<ClutchBlock> new_planned_block{};
    float cooldown_after=0.0f;
};

struct ClutchLateInput {
    bool in_game=false,on_ground=false,riding=false;
    int hurt_time=0;
    bool require_air=false,require_damage=false,require_backwards=false;
    bool moving_backwards=false; // source-checked facing-motion observation
    bool has_planned_candidate=false,holding_blocks=false,face_up=false;
    ClutchBlock candidate_block{}; // preverified original world/visibility candidate
    bool rotation_priority_granted=false,silent_rotation_active=false;
    bool jump_held=false,instant_jump_snapback=false;
    bool disable_after=false;
    int select_blocks_mode=0;
    float snapback_duration=5.0f,snapback_delay=0.0f;
    float snapback_delay_scale=1.0f; // preverified original random draw
    float aim_speed=15.0f,aim_multiplier=0.5f,
          aim_multiplier_strength=0.5f;
    float aim_yaw_diff=0.0f,aim_pitch_diff=0.0f; // supplied look-at result
    float capped_yaw_scale=1.0f,capped_pitch_scale=1.0f;
    float aim_jitter_yaw=0.0f,aim_jitter_pitch=0.0f;
    float current_yaw=0.0f,current_pitch=0.0f,
          render_yaw=0.0f,render_pitch=0.0f;
    float jump_wiggle_yaw=0.0f,jump_wiggle_pitch=0.0f;
    bool already_aimed_at_top=false;
};
struct ClutchLateDecision {
    bool admitted=false,planned=false,rotating=false;
    bool proposed_disable_module=false,proposed_track_owner=false;
    bool proposed_restore_hotbar=false;
    std::optional<float> proposed_yaw_delta{},proposed_pitch_delta{};
    int snapback_delay_remaining=0,snapback_ticks_remaining=0,
        jump_snapback_ticks_remaining=0,ticks_since_hurt=0;
};
struct ClutchRotationDecision {
    bool override_yaw=false,override_pitch=false;
};

class ClutchController final {
public:
    void reset() noexcept {
        cooldown_=0.0f;planned_=rotating_=false;
        block_={};face_up_=false;
        ticks_since_hurt_=0;jump_window_=0;
        snapback_delay_=0;snapback_ticks_=0;
        disable_after_snapback_=false;
    }

    ClutchClickDecision on_tick(const ClutchClickInput& in) noexcept {
        ClutchClickDecision out;
        if(!std::isfinite(in.clicks_per_second) ||
           in.clicks_per_second<1.0f || in.clicks_per_second>100.0f)
            return out;
        out.admitted=true;
        float cooldown=cooldown_;
        if(cooldown>0.0f) {
            cooldown-=1.0f;
            cooldown_=cooldown;
            if(cooldown>0.0f) {
                out.cooldown_after=cooldown_;
                return out;
            }
        }
        cooldown_=20.0f/in.clicks_per_second+cooldown;
        out.cooldown_after=cooldown_;
        // Source advances its click timer even if no planned block exists.
        if(!in.in_game||!planned_||!in.hit_is_block||
           !(in.hit==block_)||in.hit_top!=face_up_)
            return out;
        out.proposed_use_click=true;
        block_=in.adjacent;  // next click aims at adjacent placed block
        out.new_planned_block=block_;
        return out;
    }

    ClutchRotationDecision on_rotation_flag() const noexcept {
        return {rotating_,rotating_};
    }

    ClutchLateDecision on_late(const ClutchLateInput& in) noexcept {
        ClutchLateDecision out;
        const float finite_values[]{
            in.snapback_duration,in.snapback_delay,in.snapback_delay_scale,
            in.aim_speed,in.aim_multiplier,in.aim_multiplier_strength,
            in.aim_yaw_diff,in.aim_pitch_diff,in.capped_yaw_scale,
            in.capped_pitch_scale,in.aim_jitter_yaw,in.aim_jitter_pitch,
            in.current_yaw,in.current_pitch,in.render_yaw,in.render_pitch,
            in.jump_wiggle_yaw,in.jump_wiggle_pitch};
        for(float value:finite_values)if(!std::isfinite(value))return out;
        if(in.hurt_time<0||in.hurt_time>200||
           in.select_blocks_mode<0||in.select_blocks_mode>2||
           in.snapback_duration<0.0f||in.snapback_duration>200.0f||
           in.snapback_delay<0.0f||in.snapback_delay>200.0f||
           in.snapback_delay_scale<1.0f||in.snapback_delay_scale>2.0f||
           in.aim_speed<0.0f||in.aim_speed>180.0f||
           in.aim_multiplier<0.0f||in.aim_multiplier>2.0f||
           in.aim_multiplier_strength<0.0f||
           in.aim_multiplier_strength>1.0f||
           in.capped_yaw_scale<0.0f||in.capped_yaw_scale>1.0f||
           in.capped_pitch_scale<0.0f||in.capped_pitch_scale>1.0f)
            return out;
        out.admitted=true;
        planned_=false;rotating_=false;
        if(!in.in_game)return snapshot(out);
        ticks_since_hurt_ = in.hurt_time==0 ?
            std::min(ticks_since_hurt_+1,1000000):0;
        if(jump_window_>0) {
            --jump_window_;
            if(in.on_ground && in.silent_rotation_active &&
               in.instant_jump_snapback && in.jump_held) {
                out.proposed_yaw_delta=wrap_yaw(in.render_yaw-
                    in.current_yaw+in.jump_wiggle_yaw);
                out.proposed_pitch_delta=in.render_pitch-
                    in.current_pitch+in.jump_wiggle_pitch;
                rotating_=true;
                snapback_delay_=static_cast<int>(in.snapback_duration);
                return snapshot(out);
            }
        }
        const bool can_clutch=(!in.require_air||!in.on_ground) &&
            (!in.require_damage||ticks_since_hurt_<11) &&
            !in.riding && (!in.require_backwards||in.moving_backwards);
        bool aiming=false;
        if(can_clutch && in.has_planned_candidate) {
            if(in.select_blocks_mode==1 || in.select_blocks_mode==2) {
                out.proposed_track_owner=true;
                out.proposed_restore_hotbar=in.select_blocks_mode==2;
            }
            planned_=in.holding_blocks;
            if(planned_) {
                block_=in.candidate_block;face_up_=in.face_up;
                snapback_delay_=static_cast<int>(
                    in.snapback_delay_scale*in.snapback_delay);
                jump_window_=10;
                disable_after_snapback_=in.disable_after;
                aiming=true;
            }
        }
        if(!aiming) {
            if(snapback_delay_>0) {
                --snapback_delay_;
                snapback_ticks_=static_cast<int>(in.snapback_duration);
            } else if(snapback_ticks_>0) {
                --snapback_ticks_;
            } else {
                if(disable_after_snapback_) {
                    out.proposed_disable_module=true;
                    disable_after_snapback_=false;
                }
                return snapshot(out);
            }
        }
        if(!in.rotation_priority_granted)return snapshot(out);
        rotating_=true;
        if(planned_) {
            if(in.already_aimed_at_top)return snapshot(out);
            // Source caps the fractional aim and cap-speed components
            // separately, then lerps by aim_multiplier_strength. Source RNG
            // draws enter only through explicit typed jitter factors.
            const float capped_yaw=std::clamp(
                in.aim_yaw_diff,-in.aim_speed*in.capped_yaw_scale,
                 in.aim_speed*in.capped_yaw_scale);
            const float capped_pitch=std::clamp(
                in.aim_pitch_diff,-in.aim_speed*in.capped_pitch_scale,
                 in.aim_speed*in.capped_pitch_scale);
            const float fractional_yaw=
                (in.aim_multiplier+in.aim_jitter_yaw)*in.aim_yaw_diff;
            const float fractional_pitch=
                (in.aim_multiplier+in.aim_jitter_pitch)*in.aim_pitch_diff;
            out.proposed_yaw_delta=capped_yaw+(fractional_yaw-capped_yaw)*
                                          in.aim_multiplier_strength;
            out.proposed_pitch_delta=capped_pitch+(fractional_pitch-capped_pitch)*
                                            in.aim_multiplier_strength;
        } else if(snapback_delay_<=0 && snapback_ticks_>0 &&
                  in.silent_rotation_active) {
            out.proposed_yaw_delta=wrap_yaw(
                in.render_yaw-in.current_yaw)/
                static_cast<float>(snapback_ticks_);
            out.proposed_pitch_delta=(
                in.render_pitch-in.current_pitch)/
                static_cast<float>(snapback_ticks_);
        }
        return snapshot(out);
    }

    float cooldown() const noexcept{return cooldown_;}
    bool planned() const noexcept{return planned_;}
    ClutchBlock planned_block() const noexcept{return block_;}
private:
    static float wrap_yaw(float delta) noexcept {
        const float turn=std::fmod(delta,360.0f);
        return turn>180.0f?turn-360.0f:
               turn< -180.0f?turn+360.0f:turn;
    }
    ClutchLateDecision snapshot(ClutchLateDecision out) const noexcept {
        out.planned=planned_;out.rotating=rotating_;
        out.snapback_delay_remaining=snapback_delay_;
        out.snapback_ticks_remaining=snapback_ticks_;
        out.jump_snapback_ticks_remaining=jump_window_;
        out.ticks_since_hurt=ticks_since_hurt_;
        return out;
    }
    float cooldown_=0.0f;
    bool planned_=false,rotating_=false,face_up_=false;
    ClutchBlock block_{};
    int ticks_since_hurt_=0,jump_window_=0;
    int snapback_delay_=0,snapback_ticks_=0;
    bool disable_after_snapback_=false;
};
} // namespace slinkywinky::reference_compat
