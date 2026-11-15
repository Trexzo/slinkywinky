#pragma once
// R227: independently authored, OFFLINE Bridge Assist decisions from pinned
// OpenSlinky C++ source. Native descriptor category 3, events 5/14/17/18.
// No key-state, placement, player-position, packet or JNI writes.
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace slinkywinky::reference_compat {

struct BridgeEdgeSample {
    double next_x=0.0, next_z=0.0;
    float margin=0.08f;
    // World-provided solids at (floor(x-margin),floor(z-margin)),
    // low-x/high-z, high-x/low-z, high-x/high-z respectively.
    // A required but absent observation MUST fail closed.
    std::array<std::optional<bool>,4> solid{};
};
struct BridgeEdgeResult {bool valid=false,unsafe=false;};
inline BridgeEdgeResult evaluate_bridge_edge(const BridgeEdgeSample& s) noexcept {
    if(!std::isfinite(s.next_x)||!std::isfinite(s.next_z)||
       !std::isfinite(s.margin)||s.margin<0.0f||s.margin>0.5f||
       std::fabs(s.next_x)>1000000.0||std::fabs(s.next_z)>1000000.0)
        return {};
    const int x0=static_cast<int>(std::floor(s.next_x-s.margin));
    const int x1=static_cast<int>(std::floor(s.next_x+s.margin));
    const int z0=static_cast<int>(std::floor(s.next_z-s.margin));
    const int z1=static_cast<int>(std::floor(s.next_z+s.margin));
    if(!s.solid[0] || (z0!=z1 && !s.solid[1]) ||
       (x0!=x1 && !s.solid[2]) ||
       (x0!=x1 && z0!=z1 && !s.solid[3]))return {};
    const bool supported=*s.solid[0] ||
        (z0!=z1 && *s.solid[1]) ||
        (x0!=x1 && *s.solid[2]) ||
        (x0!=x1 && z0!=z1 && *s.solid[3]);
    return {true,!supported};
}

struct BridgeAssistTickInput {
    bool in_game=false;
    int activation_mode=1; // 0 off, 1 on, 2 keybind-toggle
    bool only_while_sneak_held=false;
    bool only_when_looking_down=false;
    bool only_while_not_rising=false;
    bool only_few_ticks_after_blocks=false;
    bool also_air_while_not_falling=false;
    bool edge_only_placing=false;
    bool randomise=false;
    bool physical_sneak_held=false;
    bool effective_sneak_pressed=false;
    bool on_ground=true;
    bool holding_block=false;
    float pitch=80.0f, move_forward=-1.0f;
    double motion_y=0.0;
    float release_delay=2.0f;
    float margin_random_reduction=0.0f; // preverified RNG result
    float release_random_delta=0.0f; // preverified RNG result
    std::optional<BridgeEdgeSample> edge{};
};
struct BridgeAssistTickDecision {
    bool admitted=false,placing=false,restrict_placing=false;
    bool owned_sneak=false;
    std::optional<bool> proposed_sneak_pressed{};
    int cooldown_left=0,held_block_ticks_left=0;
    float release_timer=0.0f;
};
struct BridgeAssistLateDecision {
    bool placing=false;
    std::optional<bool> proposed_track_owner_toggle{};
};
struct BridgeAssistDisableInput {
    bool own_module=false,enabled=false,physical_sneak_held=false;
};
struct BridgeAssistDisableDecision {
    std::optional<bool> proposed_restore_sneak_pressed{};
};
struct BridgeAssistPlaceInput {
    bool cancelled=false,holding_block=false;
    bool hit_is_block=false,on_ground=false;
    unsigned hit_face=0;
    bool back_key=false,left_key=false,right_key=false;
    float yaw=0.0f;
    double player_x=0.0,player_y=0.0,player_z=0.0;
    double hit_x=0.0,hit_y=0.0,hit_z=0.0;
    double placed_block_x=0.0,placed_block_z=0.0;
};
struct BridgeAssistPlaceDecision {
    bool admitted=false;
    bool proposed_allow=false;
    int cooldown_after=0;
};
inline bool bridge_aimed_at_edge_below(const BridgeAssistPlaceInput& v) noexcept {
    if(!v.hit_is_block||v.hit_face<2||!v.on_ground||!v.back_key||
       v.left_key||v.right_key||!std::isfinite(v.yaw) ||
       !std::isfinite(v.player_x)||!std::isfinite(v.player_y)||
       !std::isfinite(v.player_z)||!std::isfinite(v.hit_x)||
       !std::isfinite(v.hit_y)||!std::isfinite(v.hit_z)||
       !std::isfinite(v.placed_block_x)||!std::isfinite(v.placed_block_z))
       return false;
    const double yaw=std::fabs(std::fmod(static_cast<double>(v.yaw),90.0));
    if(yaw<30.0||yaw>60.0||std::fabs(v.hit_x-v.player_x)>1.0||
       std::fabs(v.hit_z-v.player_z)>1.0||
       v.hit_y-v.player_y< -1.0||v.hit_y-v.player_y>0.0)return false;
    const bool inside_x=v.placed_block_x<v.player_x &&
                        v.player_x<v.placed_block_x+1.0;
    const bool inside_z=v.placed_block_z<v.player_z &&
                        v.player_z<v.placed_block_z+1.0;
    const double edge=inside_x && inside_z ? 0.25:0.15;
    const double frac_x=std::fabs(std::fmod(v.hit_x,1.0));
    const double frac_z=std::fabs(std::fmod(v.hit_z,1.0));
    return (frac_x>=edge && frac_x<=1.0-edge) ||
           (frac_z>=edge && frac_z<=1.0-edge);
}

class BridgeAssistController final {
public:
    void reset() noexcept {
        owned_=false;placing_=false;restrict_=false;
        cooldown_=0;held_ticks_=0;release_timer_=0.0f;
        activation_mode_=1;
    }
    BridgeAssistTickDecision on_tick(const BridgeAssistTickInput& in) noexcept {
        BridgeAssistTickDecision out;
        if(in.activation_mode<0||in.activation_mode>2||
           !std::isfinite(in.pitch)||!std::isfinite(in.move_forward)||
           !std::isfinite(in.motion_y)||!std::isfinite(in.release_delay)||
           in.release_delay<0.0f||in.release_delay>200.0f||
           !std::isfinite(in.margin_random_reduction)||
           !std::isfinite(in.release_random_delta)||
           std::fabs(in.release_random_delta)>4.0f||
           in.margin_random_reduction<0.0f||
           in.margin_random_reduction>0.15f)return out;
        // A missing collision sample can never be interpreted as "air".
        // Validate any supplied snapshot before changing source state.
        if(in.in_game && !in.edge)return out;
        BridgeEdgeSample edge{};
        if(in.edge) {
            edge=*in.edge;
            if(in.randomise)
                edge.margin=std::max(0.0f,
                    edge.margin-in.margin_random_reduction);
            if(!evaluate_bridge_edge(edge).valid)return out;
        }
        if(!in.in_game) {
            out.admitted=true;
            if(owned_)out.proposed_sneak_pressed=in.physical_sneak_held;
            reset();
            return complete(out);
        }
        out.admitted=true;
        activation_mode_=in.activation_mode;
        placing_=restrict_=false;
        if(cooldown_>0)--cooldown_;
        bool permitted=(in.only_while_sneak_held==in.physical_sneak_held) &&
            (!in.only_when_looking_down||in.pitch>=65.0f) &&
            (!in.only_while_not_rising||in.move_forward<=0.0f);
        if(permitted) {
            placing_=(in.activation_mode!=0);
            if(in.only_few_ticks_after_blocks) {
                if(in.holding_block)held_ticks_=2;
                else if(held_ticks_<=0)permitted=false;
                else --held_ticks_;
            }
        }
        if(!permitted) {
            if(owned_)out.proposed_sneak_pressed=in.physical_sneak_held;
            owned_=false;
            return complete(out);
        }
        // The pinned callback takes ownership even if the next position
        // is safe and no actual sneak change is proposed.
        owned_=true;
        enum Action {Keep,CheckEdge,CountDown};
        Action action=Keep;
        if(in.effective_sneak_pressed) {
            if(in.only_while_sneak_held) {
                action=(in.move_forward>=0.0f||cooldown_<=0)?
                    Keep:CheckEdge;
            } else if(in.on_ground)action=CheckEdge;
            else if(in.also_air_while_not_falling && in.motion_y>=0.0)
                action=CheckEdge;
            else action=CountDown;
        } else {
            if(!in.on_ground &&
               (!in.also_air_while_not_falling||in.motion_y<0.0))
                return complete(out);
            if(!evaluate_bridge_edge(edge).unsafe)return complete(out);
            out.proposed_sneak_pressed=true;
            action=Keep;
        }
        if(action==CheckEdge) {
            if(evaluate_bridge_edge(edge).unsafe)action=Keep;
        }
        if(action!=Keep) {
            release_timer_-=1.0f;
            if(!(release_timer_>0.0f)) {
                out.proposed_sneak_pressed=false;
                release_timer_+=in.release_delay +
                    (in.randomise?in.release_random_delta:0.0f);
                return complete(out);
            }
        }
        restrict_=in.edge_only_placing;
        return complete(out);
    }

    BridgeAssistLateDecision on_late() const noexcept {
        BridgeAssistLateDecision out;
        out.placing=placing_;
        if(placing_)out.proposed_track_owner_toggle=activation_mode_==2;
        return out;
    }
    BridgeAssistDisableDecision on_disable(
        const BridgeAssistDisableInput& input) noexcept {
        BridgeAssistDisableDecision out;
        if(input.own_module && !input.enabled && owned_)
            out.proposed_restore_sneak_pressed=input.physical_sneak_held;
        if(input.own_module && !input.enabled)reset();
        return out;
    }
    BridgeAssistPlaceDecision on_place(
        const BridgeAssistPlaceInput& input) noexcept {
        BridgeAssistPlaceDecision out;
        out.cooldown_after=cooldown_;
        if(input.cancelled||cooldown_==20)return out;
        out.admitted=true;
        if(restrict_ && bridge_aimed_at_edge_below(input)) {
            out.proposed_allow=true;
            return out;
        }
        if(input.holding_block)cooldown_=20;
        out.cooldown_after=cooldown_;
        return out;
    }
    int cooldown() const noexcept {return cooldown_;}
    bool owns_sneak() const noexcept {return owned_;}
    bool placing() const noexcept {return placing_;}
private:
    BridgeAssistTickDecision complete(BridgeAssistTickDecision v) noexcept {
        v.placing=placing_;
        v.restrict_placing=restrict_;
        v.owned_sneak=owned_;
        v.cooldown_left=cooldown_;
        v.held_block_ticks_left=held_ticks_;
        v.release_timer=release_timer_;
        return v;
    }
    bool owned_=false,placing_=false,restrict_=false;
    int cooldown_=0,held_ticks_=0;
    float release_timer_=0.0f;
    int activation_mode_=1;
};
} // namespace slinkywinky::reference_compat
