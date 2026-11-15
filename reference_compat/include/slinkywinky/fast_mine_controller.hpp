#pragma once
// R226: first-party bounded native Fast Mine model reconstructed from the
// pinned C++ source descriptor. Block category 3, callback event0 and
// later event18, both priority0. Pure VALUE proposals, no game writes.
#include <cmath>
#include <limits>
#include <optional>

namespace slinkywinky::reference_compat {

struct FastMineEarlyInput {
    bool in_game=false;
    bool controller_present=false;
    float damage_progress=0.0f;
    int block_hit_delay=0;
    float max_break_delay=5.0f;
    bool decrease_unchanged_delay=false;
};
struct FastMineLateInput {
    bool in_game=false;
    bool controller_present=false;
    float damage_progress=0.0f;
    float speed=2.0f;
};
struct FastMineEarlyDecision {
    bool admitted=false;
    std::optional<int> proposed_block_hit_delay{};
};
struct FastMineLateDecision {
    bool admitted=false;
    std::optional<float> proposed_damage_progress{};
};

// Original source caches damage progress BEFORE the game's next mining
// progress update and caches RAW block hit delay after the first callback.
// On later event18 it only accelerates first-frame 0 -> positive progress.
class FastMineController final {
public:
    void reset() noexcept {
        last_damage_progress_=0.0f;
        last_block_hit_delay_=0;
        early_observed_=false;
    }
    FastMineEarlyDecision on_early(const FastMineEarlyInput& in) noexcept {
        FastMineEarlyDecision out;
        if(!in.in_game || !std::isfinite(in.max_break_delay) ||
           in.max_break_delay<0.0f || in.max_break_delay>100.0f) {
            reset(); // never reuse an old early-phase observation
            return out;
        }
        if(in.controller_present &&
           (!std::isfinite(in.damage_progress) ||
            in.damage_progress<0.0f || in.damage_progress>1.0f ||
            in.block_hit_delay<0 || in.block_hit_delay>10000)) {
            reset();
            return out;
        }
        out.admitted=true;
        int delay=0;
        if(in.controller_present) {
            last_damage_progress_=in.damage_progress;
            delay=in.block_hit_delay;
            if(static_cast<float>(delay)>in.max_break_delay)
                out.proposed_block_hit_delay=
                    static_cast<int>(in.max_break_delay);
            else if(delay>0 && in.decrease_unchanged_delay &&
                    delay==last_block_hit_delay_)
                out.proposed_block_hit_delay=delay-1;
        }
        last_block_hit_delay_=delay;
        early_observed_=in.controller_present;
        return out;
    }
    FastMineLateDecision on_late(const FastMineLateInput& in) noexcept {
        FastMineLateDecision out;
        if(!in.in_game || !in.controller_present || !early_observed_ ||
           !std::isfinite(in.damage_progress) ||
           !std::isfinite(in.speed) ||
           in.damage_progress<0.0f || in.damage_progress>1.0f ||
           in.speed<1.0f || in.speed>100.0f)return out;
        out.admitted=true;
        if(in.damage_progress==0.0f ||
           last_damage_progress_!=0.0f)return out;
        const double per_tick=static_cast<double>(in.damage_progress);
        const double normal=std::ceil(1.0/per_tick);
        const double wanted=std::ceil((1.0/static_cast<double>(in.speed))/per_tick);
        const double ticks_done=normal-wanted+1.0;
        const double proposed=ticks_done*per_tick;
        // Upstream's unguarded unsigned cast could overflow on bad input.
        // Refuse such states rather than exposing invalid game progress.
        if(!std::isfinite(normal) || !std::isfinite(wanted) ||
           ticks_done<0.0 ||
           ticks_done>static_cast<double>(std::numeric_limits<int>::max()) ||
           !std::isfinite(proposed) || proposed<0.0 || proposed>1.0+1e-5)
            return {};
        out.proposed_damage_progress=static_cast<float>(proposed);
        return out;
    }
    float previous_damage_progress() const noexcept {
        return last_damage_progress_;
    }
    int previous_block_hit_delay() const noexcept {
        return last_block_hit_delay_;
    }
private:
    float last_damage_progress_=0.0f;
    int last_block_hit_delay_=0;
    bool early_observed_=false;
};
} // namespace slinkywinky::reference_compat
