#pragma once
// R259: independently authored Auto Weapon event25 delay and hotbar
// proposal model. Caller supplies a separately verified Weapons first-slot.
// No Minecraft inventory writes or synthetic mouse actions.
#include <cmath>
#include <cstdint>

namespace slinkywinky::reference_compat {
struct AutoWeaponInput {
    bool input_ready=false,is_using_item=false;
    bool require_mouse_pressed=false,mouse_pressed=false;
    bool hit_is_entity=false,remote_other_player=false,hostile=false;
    bool current_is_weapon=false,action_owner_granted=false;
    std::uint64_t world_token=0,player_token=0;
    int current_slot=0,first_allowed_slot=-1;
    float activation_time=0.0f;
};
struct AutoWeaponDecision {
    bool admitted=false,eligible=false,waiting=false;
    bool claim_required=false,proposed_slot_change=false;
    int proposed_slot=-1,selection_delay_ticks=0;
};
class AutoWeaponController final {
public:
    AutoWeaponDecision on_event25(const AutoWeaponInput& in) noexcept {
        AutoWeaponDecision out;
        if(!valid(in))return out;
        if(world_!=0 && (world_!=in.world_token ||
                         player_!=in.player_token)) {
            reset();
            return out; // fail closed on player/world ownership swap
        }
        if(world_==0) {world_=in.world_token;player_=in.player_token;}
        out.admitted=true;
        if(!in.input_ready || in.is_using_item ||
           (in.require_mouse_pressed && !in.mouse_pressed) ||
           !in.hit_is_entity || !in.remote_other_player ||
           !in.hostile || in.current_is_weapon ||
           in.first_allowed_slot<0 ||
           in.first_allowed_slot==in.current_slot) {
            ticks_=0;
            return out;
        }
        out.eligible=true;
        // Reconstructed Java State.waiting(): compare float threshold
        // with integer counter BEFORE incrementing, never ceil/round.
        if(in.activation_time>static_cast<float>(ticks_)) {
            ++ticks_;
            out.waiting=true;out.selection_delay_ticks=ticks_;
            return out;
        }
        out.claim_required=true;
        out.selection_delay_ticks=ticks_;
        if(in.action_owner_granted) {
            out.proposed_slot_change=true;
            out.proposed_slot=in.first_allowed_slot;
        }
        return out;
    }
    void reset() noexcept {
        ticks_=0;world_=player_=0;
    }
    int delay_ticks() const noexcept {return ticks_;}
private:
    static bool valid(const AutoWeaponInput& in) noexcept {
        return in.world_token>0 && in.player_token>0 &&
               in.current_slot>=0 && in.current_slot<9 &&
               in.first_allowed_slot>=-1 && in.first_allowed_slot<9 &&
               std::isfinite(in.activation_time) &&
               in.activation_time>=0.0f && in.activation_time<=1000.0f;
    }
    std::uint64_t world_=0,player_=0;
    int ticks_=0;
};
} // namespace slinkywinky::reference_compat
