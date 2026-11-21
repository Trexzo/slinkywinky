#pragma once
// R257: reconstructed No Hurt Cam maxHurtTime sentinel policy.
// This controller proposes values but NEVER changes an entity or renderer.
#include <cstdint>
#include <limits>

namespace slinkywinky::reference_compat {
struct NoHurtCamObservation {
    bool game_ready=false;
    std::uint64_t world_token=0,player_token=0;
    int observed_max_hurt_time=0;
};
struct NoHurtCamProposal {
    bool admitted=false,apply_sentinel=false,restore_original=false;
    int proposed_value=0;
};
class NoHurtCamController final {
public:
    static constexpr int kSentinel=std::numeric_limits<int>::max();
    NoHurtCamProposal on_pre_render(const NoHurtCamObservation& in) noexcept {
        NoHurtCamProposal out;
        if(!valid(in))return out;
        out.admitted=true;
        if(!in.game_ready)return out;
        if(held_ && !same(in)) {
            // An old entity is no longer owned. Never write its cached
            // value to a new entity; caller must supply the old entity
            // and use on_disable before changing players.
            reset();
            return {};
        }
        if(!held_) {
            world_=in.world_token;player_=in.player_token;
        }
        // Source: !held || current!=MAX overwrites saved. On a repeated
        // rendered sentinel, keep the earlier original field value.
        if(!held_ || in.observed_max_hurt_time!=kSentinel)
            saved_=in.observed_max_hurt_time;
        held_=true;
        out.apply_sentinel=true;out.proposed_value=kSentinel;
        return out;
    }
    NoHurtCamProposal on_disable(const NoHurtCamObservation& in) noexcept {
        NoHurtCamProposal out{};
        if(!held_) {
            out.admitted=true;
            return out;
        }
        if(!valid(in) || !same(in)) {
            reset();
            return out;
        }
        out.admitted=true;
        // Source State.restore only uses saved if current is sentinel.
        // Otherwise external non-sentinel writes must not be undone.
        if(in.observed_max_hurt_time==kSentinel) {
            out.restore_original=true;out.proposed_value=saved_;
        }
        reset();
        return out;
    }
    NoHurtCamProposal on_world_unload(
        const NoHurtCamObservation& in) noexcept {
        return on_disable(in);
    }
    bool held() const noexcept {return held_;}
    void reset() noexcept {
        held_=false;saved_=0;world_=player_=0;
    }
private:
    static bool valid(const NoHurtCamObservation& in) noexcept {
        return in.observed_max_hurt_time>=0 &&
               (!in.game_ready ||
                (in.world_token>0 && in.player_token>0));
    }
    bool same(const NoHurtCamObservation& in) const noexcept {
        return in.world_token==world_ && in.player_token==player_;
    }
    bool held_=false;
    int saved_=0;
    std::uint64_t world_=0,player_=0;
};
} // namespace slinkywinky::reference_compat
