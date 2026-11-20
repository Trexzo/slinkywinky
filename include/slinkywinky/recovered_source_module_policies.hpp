#pragma once
// R254: independently written, source-grounded, value-only native policies
// for OpenSlinky fd9c592 No Hit Delay, Safe Walk and Fast Place.
// These never mutate Minecraft/JVM memory, synthesize input or send packets.
#include "slinkywinky/module_arraylist.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>

namespace slinkywinky {

struct RecoveredNoHitInput {
    bool attack_click=false;
    bool jvm_ready=false;
    int blocked_ticks=0;
    unsigned mode=0;
    bool target_present=false;
    int target_kind=0;
    bool using_item=false;
};
struct RecoveredNoHitDecision {
    bool reset_counter=false;
    bool propose_swing=false;
};
inline RecoveredNoHitDecision decide_no_hit_delay(
        const RecoveredNoHitInput& input) noexcept {
    if(!input.attack_click || !input.jvm_ready || input.blocked_ticks<=0)
        return {};
    if(input.mode==1 && (!input.target_present || input.target_kind==0))
        return {false,!input.using_item};
    return {true,false};
}

struct RecoveredSafeWalkSettings {
    bool look_down=false;
    bool forbid_forward=false;
    bool require_blocks=false;
    float horizontal_multiplier=1.0f;
};
struct RecoveredSafeWalkInput {
    bool in_game=false;
    float pitch=0.0f,forward=0.0f;
    bool holding_blocks=false,on_ground=false;
    double motion_x=0.0,motion_y=0.0,motion_z=0.0;
    RecoveredSafeWalkSettings settings{};
};
struct RecoveredSafeWalkMotion {
    double x=0.0,y=0.0,z=0.0;
};
struct RecoveredSafeWalkDecision {
    std::optional<bool> edge_guard{};
    std::optional<RecoveredSafeWalkMotion> motion{};
};
inline RecoveredSafeWalkDecision decide_safe_walk(
        const RecoveredSafeWalkInput& input) noexcept {
    RecoveredSafeWalkDecision out;
    if(!input.in_game || !std::isfinite(input.pitch) ||
       !std::isfinite(input.forward) ||
       !std::isfinite(input.settings.horizontal_multiplier) ||
       input.settings.horizontal_multiplier<0.0f)
        return out;
    const auto& cfg=input.settings;
    if((cfg.look_down && input.pitch<65.0f) ||
       (cfg.forbid_forward && input.forward>0.0f) ||
       (cfg.require_blocks && !input.holding_blocks)){
        out.edge_guard=false;
        return out;
    }
    out.edge_guard=true;
    if(!input.on_ground || cfg.horizontal_multiplier==1.0f ||
       !std::isfinite(input.motion_x) || !std::isfinite(input.motion_y) ||
       !std::isfinite(input.motion_z))
        return out;
    const double multiplier=static_cast<double>(cfg.horizontal_multiplier);
    const double x=input.motion_x*multiplier;
    const double z=input.motion_z*multiplier;
    if(std::isfinite(x) && std::isfinite(z))
        out.motion=RecoveredSafeWalkMotion{x,input.motion_y,z};
    return out;
}

struct RecoveredFastPlaceInput {
    bool in_game=false;
    bool use_held=false;
    bool holds_block=false;
    bool press_pending=false;
    bool target_block=false;
    bool placement_intersects_player=false;
    bool held_ladder=false;
    bool random_bypass=false;
    float cps=10.0f;
    float activation_ticks=0.0f;
};
struct RecoveredFastPlaceDecision {
    unsigned proposed_presses=0;
    bool valid=false;
    float timer=0.0f;
    int delay_left=0;
};
class RecoveredFastPlaceModel final {
public:
    RecoveredFastPlaceDecision tick(
            const RecoveredFastPlaceInput& in) noexcept {
        RecoveredFastPlaceDecision out;
        if(!std::isfinite(in.cps) || in.cps<1.0f || in.cps>100.0f ||
           !std::isfinite(in.activation_ticks) ||
           in.activation_ticks<0.0f || in.activation_ticks>200.0f)
            return out;
        out.valid=true;
        auto finish=[&]() noexcept {
            out.timer=timer_;
            out.delay_left=delay_;
            return out;
        };
        if(timer_>0.0f){
            timer_-=1.0f;
            if(timer_>0.0f)return finish();
        }
        if(!in.in_game)return finish();
        const bool was_held=held_last_tick_;
        held_last_tick_=in.use_held;
        if(!in.use_held){
            delay_=static_cast<int>(in.activation_ticks);
            return finish();
        }
        if(delay_>0){
            --delay_;
            return finish();
        }
        if(in.press_pending || timer_>0.0f)return finish();
        bool vanilla_first=!was_held;
        const float interval=20.0f/in.cps;
        // Six is the upper bound for 100 CPS (20 ticks/sec).
        for(unsigned slot=0;slot<6u;++slot){
            timer_+=interval;
            if(!vanilla_first && in.holds_block &&
               (in.held_ladder ||
                (in.target_block && !in.placement_intersects_player) ||
                in.random_bypass))
                ++out.proposed_presses;
            vanilla_first=false;
            if(timer_>0.0f)return finish();
        }
        reset();
        return {};
    }
    void reset() noexcept {
        timer_=0.0f;
        delay_=0;
        held_last_tick_=false;
    }
    float timer() const noexcept {return timer_;}
    int activation_left() const noexcept {return delay_;}
    bool was_held() const noexcept {return held_last_tick_;}
private:
    float timer_=0.0f;
    int delay_=0;
    bool held_last_tick_=false;
};

// Source registration metadata pinned to OpenSlinky fd9c592:
// utility No Hit Delay event 14 / priority 0;
// movement Safe Walk event 0 / priority 0;
// block Fast Place event 0 / priority -1000.
// These are PROPOSAL callbacks, not registered Minecraft game hooks.
enum class RecoveredSourceModule : std::uint8_t {
    NoHitDelay=0,SafeWalk=1,FastPlace=2
};
struct RecoveredSourceRegistration {
    const char* name;
    unsigned category;    // 0 combat, 1 move, 2 visual, 3 block, 4 utility
    unsigned event;
    int priority;
};
inline constexpr std::array<RecoveredSourceRegistration,3>
kRecoveredSourceRegistrations{{
    {"No Hit Delay",4u,14u,0},
    {"Safe Walk",1u,0u,0},
    {"Fast Place",3u,0u,-1000}
}};

// Isolated first-party module-owner replay with a real registration identity,
// explicit owner lifecycle, authoritative selection and event admission.
// The existing PR1 R245 GUI control transport is still only three R211
// modules: these R254 models are NOT connected to it or to a live engine.
class RecoveredSourcePolicyOwner final {
public:
    RecoveredSourcePolicyOwner() : owner_(std::this_thread::get_id()) {}
    bool transition(std::uint64_t generation,bool world_ready) noexcept {
        if(!on_owner() || retired_ || generation==0 ||
           generation<generation_ ||
           (generation==generation_ && !world_ready_ && world_ready))
            return false;
        if(generation!=generation_ || (world_ready_ && !world_ready))
            fast_place_.reset();
        generation_=generation;
        world_ready_=world_ready;
        return true;
    }
    bool select(RecoveredSourceModule module,bool enabled) noexcept {
        const unsigned slot=static_cast<unsigned>(module);
        if(!on_owner() || retired_ || slot>=selected_.size())
            return false;
        if(module==RecoveredSourceModule::FastPlace &&
           selected_[slot]!=enabled)fast_place_.reset();
        selected_[slot]=enabled;
        return true;
    }
    bool selected(RecoveredSourceModule module) const noexcept {
        const auto slot=static_cast<unsigned>(module);
        return slot<selected_.size() && selected_[slot];
    }
    std::optional<RecoveredNoHitDecision> attack(
            unsigned event,const RecoveredNoHitInput& in) const noexcept {
        if(!can_dispatch(RecoveredSourceModule::NoHitDelay,event))
            return std::nullopt;
        return decide_no_hit_delay(in);
    }
    std::optional<RecoveredSafeWalkDecision> safe_walk(
            unsigned event,const RecoveredSafeWalkInput& in) const noexcept {
        if(!can_dispatch(RecoveredSourceModule::SafeWalk,event))
            return std::nullopt;
        return decide_safe_walk(in);
    }
    std::optional<RecoveredFastPlaceDecision> fast_place(
            unsigned event,const RecoveredFastPlaceInput& in) noexcept {
        if(!can_dispatch(RecoveredSourceModule::FastPlace,event))
            return std::nullopt;
        return fast_place_.tick(in);
    }
    std::optional<ModuleArrayListSnapshot> snapshot(
            std::uint64_t source_sequence) const {
        if(!on_owner() || retired_ || !generation_ || !source_sequence)
            return std::nullopt;
        ModuleArrayListSnapshot value;
        value.source_sequence=source_sequence;
        value.world_generation=generation_;
        value.world_ready=world_ready_;
        for(std::size_t i=0;i<selected_.size();++i){
            if(!candidate_native_manager_registered(
                  kRecoveredSourceRegistrations[i].name))
                return std::nullopt;
            value.modules.push_back({
                kRecoveredSourceRegistrations[i].name,selected_[i],
                selected_[i]&&world_ready_,generation_});
        }
        if(!make_module_arraylist_display(value))
            return std::nullopt;
        return value;
    }
    bool retire() noexcept {
        if(!on_owner() || retired_)return false;
        retired_=true;
        selected_={};
        fast_place_.reset();
        world_ready_=false;
        return true;
    }
    bool retired() const noexcept {return retired_;}
    std::uint64_t generation() const noexcept {return generation_;}
private:
    bool on_owner() const noexcept {
        return std::this_thread::get_id()==owner_;
    }
    bool can_dispatch(RecoveredSourceModule module,
                      unsigned event) const noexcept {
        const auto index=static_cast<unsigned>(module);
        return on_owner() && !retired_ && world_ready_ &&
               index<selected_.size() && selected_[index] &&
               event==kRecoveredSourceRegistrations[index].event;
    }
    const std::thread::id owner_;
    std::array<bool,3> selected_{{false,false,false}};
    std::uint64_t generation_=0;
    bool world_ready_=false;
    bool retired_=false;
    RecoveredFastPlaceModel fast_place_{};
};

} // namespace slinkywinky
