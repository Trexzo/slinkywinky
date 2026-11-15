#pragma once
// R229: first-party source-derived Clutch click cadence, rotation ownership
// and late-tick snap-back. Candidate WORLD SELECTION and RNG are explicit,
// prevalidated caller observations, not reconstructed original JNI world hooks.
// Returns value-only proposals. No input, rotation, packet or gameplay writes.
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <array>
#include <cstdint>
#include "slinkywinky/source_walk_bank.hpp"

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
    // R233: opt-in source seed/clock, includes two timed walk_update_timed
    // calls per yaw and pitch on an admitted actual aim phase.
    bool use_source_walk=false;
    double clock_seconds=0.0;
    float source_randomisation=1.0f;
    bool straight_down_aim=false;
    // R234: optional ONE source candidate sampling step, explicitly
    // requested after the caller has observed a candidate. Full
    // world enumeration can invoke more samples and remains external.
    bool source_sample_candidate=false;
    bool source_candidate_top=false;
};
struct ClutchLateDecision {
    bool admitted=false,planned=false,rotating=false;
    bool proposed_disable_module=false,proposed_track_owner=false;
    bool proposed_restore_hotbar=false;
    std::optional<float> proposed_yaw_delta{},proposed_pitch_delta{};
    std::optional<float> source_yaw_scale{},source_pitch_scale{};
    std::optional<float> source_yaw_jitter{},source_pitch_jitter{};
    std::optional<float> source_straight_down_yaw{};
    std::optional<float> source_candidate_inset{};
    std::optional<float> source_candidate_shift_x{};
    std::optional<float> source_candidate_shift_z{};
    int snapback_delay_remaining=0,snapback_ticks_remaining=0,
        jump_snapback_ticks_remaining=0,ticks_since_hurt=0;
};
struct ClutchRotationDecision {
    bool override_yaw=false,override_pitch=false;
};

class ClutchController final {
public:
    bool configure_source_walk_seed(
        const std::array<std::uint32_t,4>& seed) noexcept {
        return source_walks_.configure(seed);
    }
    const SourceWalkBank<8>& source_walks() const noexcept {
        return source_walks_;
    }
    SourceWalkRng source_rng_snapshot() const noexcept {
        return source_walks_.source_rng_snapshot();
    }
    bool adopt_source_rng(const SourceWalkRng& rng) noexcept {
        return source_walks_.adopt_source_rng(rng);
    }
    struct CandidateSamples {
        float inset=0.0f,shift_x=0.0f,shift_z=0.0f;
    };
    // Original side-face planner consumes walks #1,2,3; top-face
    // consumes walks #1,4,5. Caller remains responsible for world
    // candidate ordering and target raycast.
    std::optional<CandidateSamples> source_candidate_samples(
        bool top,double now,float randomisation) noexcept {
        if(!source_walks_.configured() ||
           !std::isfinite(randomisation) ||
           randomisation<0.0f||randomisation>1.0f)
            return std::nullopt;
        const auto previous=source_walks_;
        const auto inset=source_walks_.sample_clock(
            0,now,1.0,0.0f,0.05f);
        const auto x=source_walks_.sample_clock(
            top?3:1,now,top?1.0:0.5,
            top?-0.1f:-0.01f,top?0.1f:0.01f);
        const auto z=source_walks_.sample_clock(
            top?4:2,now,top?1.0:0.5,
            top?-0.1f:-0.01f,top?0.1f:0.01f);
        if(!inset||!x||!z) {
            source_walks_=previous;
            return std::nullopt;
        }
        return CandidateSamples{0.1f+(*inset)*randomisation,
            (*x)*randomisation,(*z)*randomisation};
    }
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
           in.capped_pitch_scale<0.0f||in.capped_pitch_scale>1.0f ||
           (in.use_source_walk &&
             (!source_walks_.configured() ||
              !std::isfinite(in.clock_seconds) ||
              in.clock_seconds<0.0||in.clock_seconds>1e12||
              !std::isfinite(in.source_randomisation)||
              in.source_randomisation<0.0f||
              in.source_randomisation>1.0f)))
            return out;
        const ClutchController previous=*this;
        auto reject=[&]() noexcept {
            *this=previous;
            return ClutchLateDecision{};
        };
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
                if(in.use_source_walk && in.source_sample_candidate) {
                    const auto sample=source_candidate_samples(
                        in.source_candidate_top,in.clock_seconds,
                        in.source_randomisation);
                    if(!sample)return reject();
                    out.source_candidate_inset=sample->inset;
                    out.source_candidate_shift_x=sample->shift_x;
                    out.source_candidate_shift_z=sample->shift_z;
                }
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
            float yaw_diff=in.aim_yaw_diff;
            float yaw_scale=in.capped_yaw_scale;
            float pitch_scale=in.capped_pitch_scale;
            float yaw_jitter=in.aim_jitter_yaw;
            float pitch_jitter=in.aim_jitter_pitch;
            if(in.use_source_walk) {
                if(in.straight_down_aim) {
                    const auto noise=source_walks_.sample_clock(
                        5,in.clock_seconds,0.5,-20.0f,20.0f);
                    if(!noise)return reject();
                    yaw_diff=(*noise)*in.source_randomisation;
                    out.source_straight_down_yaw=yaw_diff;
                }
                const float lo=1.0f-0.25f*in.source_randomisation;
                const auto ys=source_walks_.sample_timed(
                    6,1.0,10.0,lo,1.0f);
                const auto ps=source_walks_.sample_timed(
                    7,1.0,10.0,lo,1.0f);
                const auto yj=source_walks_.sample_timed(
                    6,1.0,10.0,-0.05f,0.05f);
                const auto pj=source_walks_.sample_timed(
                    7,1.0,10.0,-0.05f,0.05f);
                if(!ys||!ps||!yj||!pj)return reject();
                yaw_scale=*ys;pitch_scale=*ps;
                yaw_jitter=*yj;pitch_jitter=*pj;
                out.source_yaw_scale=yaw_scale;
                out.source_pitch_scale=pitch_scale;
                out.source_yaw_jitter=yaw_jitter;
                out.source_pitch_jitter=pitch_jitter;
            }
            const float capped_yaw=std::clamp(
                yaw_diff,-in.aim_speed*yaw_scale,
                 in.aim_speed*yaw_scale);
            const float capped_pitch=std::clamp(
                in.aim_pitch_diff,-in.aim_speed*pitch_scale,
                 in.aim_speed*pitch_scale);
            const float fractional_yaw=
                (in.aim_multiplier+yaw_jitter)*yaw_diff;
            const float fractional_pitch=
                (in.aim_multiplier+pitch_jitter)*in.aim_pitch_diff;
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
    SourceWalkBank<8> source_walks_{};
};
} // namespace slinkywinky::reference_compat
