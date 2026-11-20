#pragma once
// R256: independently written source-anchored *value-only* OpenSlinky
// fd9c592 event-8 Hitboxes + Ghost Hand policies. No Minecraft/JNI
// access, no hitbox mutation and no player-target interception.
#include "slinkywinky/module_arraylist.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <string_view>
#include <thread>

namespace slinkywinky {
inline constexpr unsigned kRecoveredEntityReachQueryEvent=8u;
enum class RecoveredEntityQueryModule : unsigned char {
    Hitboxes=0,GhostHand=1
};
struct RecoveredEntityQueryRegistration {
    const char* name;
    unsigned category;
    unsigned event;
};
inline constexpr std::array<RecoveredEntityQueryRegistration,2>
kRecoveredEntityQueryRegistrations{{
    {"Hitboxes",0u,kRecoveredEntityReachQueryEvent},
    {"Ghost Hand",3u,kRecoveredEntityReachQueryEvent}
}};

// Original Hitboxes callback sets active=true, expansion=setting,
// even when expansion is zero. Offline model rejects malformed/unbounded
// values; the 16-block cap is a deliberate protection, not original parity.
struct RecoveredHitboxesDecision {
    bool proposed_active=false;
    double expansion_per_side=0.0;
};
inline std::optional<RecoveredHitboxesDecision>
recovered_hitboxes_query(float expansion) noexcept {
    if(!std::isfinite(expansion) || expansion<0.0f ||
       expansion>16.0f)return std::nullopt;
    return RecoveredHitboxesDecision{true,static_cast<double>(expansion)};
}

struct RecoveredGhostHandSettings {
    bool require_mouse=false;
    bool allow_fists=false;
    bool allow_swords=false;
    bool allow_tools=false;
    bool allow_buckets=false;
    bool allow_flint_steel=false;
    bool allow_cobweb=false;
    bool allow_other=false;
    bool ignore_non_players=false;
    bool ignore_bots=false;
    bool ignore_friendlies=false;
    bool ignore_enemies=false;
};
struct RecoveredGhostHandQuery {
    bool mouse_button_held=false;
    bool held_item_present=false;
    int held_item_id=0;
    bool entity_is_other_player=false;
    unsigned player_flags=0;
};

inline bool recovered_ghost_item_allowed(
        const RecoveredGhostHandSettings& cfg,
        const RecoveredGhostHandQuery& in) noexcept {
    if(!in.held_item_present)return cfg.allow_fists;
    switch(in.held_item_id){
        // 1.8 IDs: five-material shovels, pickaxes, axes, and shears.
        case 256:case 257:case 258:
        case 269:case 270:case 271:
        case 273:case 274:case 275:
        case 277:case 278:case 279:
        case 284:case 285:case 286:
        case 359:
            return cfg.allow_tools;
        case 267:case 268:case 272:case 276:case 283:
            return cfg.allow_swords;
        case 325:case 326:case 327:
            return cfg.allow_buckets;
        case 259:return cfg.allow_flint_steel;
        case 30:return cfg.allow_cobweb;
        default:return cfg.allow_other;
    }
}
inline bool recovered_ghost_should_ignore(
        const RecoveredGhostHandSettings& cfg,
        const RecoveredGhostHandQuery& in) noexcept {
    if((cfg.require_mouse && !in.mouse_button_held) ||
       !recovered_ghost_item_allowed(cfg,in))return false;
    if(!in.entity_is_other_player)return cfg.ignore_non_players;
    // Pinned original checks bots BEFORE friend/teammate flags.
    if((in.player_flags&2u)!=0u)return cfg.ignore_bots;
    if((in.player_flags&5u)!=0u)return cfg.ignore_friendlies;
    return cfg.ignore_enemies;
}

// The original Ghost Hand reach query separately sets the active marker
// in the source event. Entity ignore decision remains a separate helper.
struct RecoveredGhostHandDecision {
    bool ignore_entity=false;
    bool reach_query_marker=false;
};

// Owner-thread only OFFLINE event source. These two identifiers genuinely
// exist in the 66-native-manager catalog; the extra Chest Stealer GUI
// descriptor is not included. Nothing here installs a game hook.
class RecoveredEntityQueryOwner final {
public:
    RecoveredEntityQueryOwner() : owner_(std::this_thread::get_id()) {}
    RecoveredEntityQueryOwner(const RecoveredEntityQueryOwner&)=delete;
    RecoveredEntityQueryOwner& operator=(const RecoveredEntityQueryOwner&)=delete;

    bool transition(std::uint64_t generation,bool world_ready) noexcept {
        if(!on_owner() || retired_ || generation==0 ||
           generation<generation_ ||
           (generation==generation_ && !ready_ && world_ready))
            return false;
        generation_=generation;
        ready_=world_ready;
        return true;
    }
    bool select(RecoveredEntityQueryModule mod,bool enabled) noexcept {
        const auto n=static_cast<unsigned>(mod);
        if(!on_owner() || retired_ || n>=selected_.size())
            return false;
        selected_[n]=enabled;
        return true;
    }
    bool selected(RecoveredEntityQueryModule mod) const noexcept {
        const auto n=static_cast<unsigned>(mod);
        return n<selected_.size() && selected_[n];
    }
    std::optional<RecoveredHitboxesDecision> hitboxes(
            unsigned event,float expansion) const noexcept {
        if(!admitted(RecoveredEntityQueryModule::Hitboxes,event))
            return std::nullopt;
        return recovered_hitboxes_query(expansion);
    }
    std::optional<RecoveredGhostHandDecision> ghost_hand(
            unsigned event,const RecoveredGhostHandSettings& cfg,
            const RecoveredGhostHandQuery& in) const noexcept {
        if(!admitted(RecoveredEntityQueryModule::GhostHand,event))
            return std::nullopt;
        return RecoveredGhostHandDecision{
            recovered_ghost_should_ignore(cfg,in),true};
    }
    std::optional<ModuleArrayListSnapshot> snapshot(
            std::uint64_t sequence) const {
        if(!on_owner() || retired_ || generation_==0 || sequence==0)
            return std::nullopt;
        ModuleArrayListSnapshot result;
        result.source_sequence=sequence;
        result.world_generation=generation_;
        result.world_ready=ready_;
        for(std::size_t i=0;i<selected_.size();++i){
            const auto name=std::string_view(kRecoveredEntityQueryRegistrations[i].name);
            if(!candidate_native_manager_registered(name))
                return std::nullopt;
            result.modules.push_back({
                std::string(name),selected_[i],
                selected_[i] && ready_,generation_});
        }
        if(!make_module_arraylist_display(result))
            return std::nullopt;
        return result;
    }
    bool retire() noexcept {
        if(!on_owner() || retired_)return false;
        retired_=true;
        selected_={};
        ready_=false;
        return true;
    }
    bool retired() const noexcept{return retired_;}
private:
    bool on_owner() const noexcept {
        return std::this_thread::get_id()==owner_;
    }
    bool admitted(RecoveredEntityQueryModule mod,unsigned event) const noexcept {
        const auto index=static_cast<unsigned>(mod);
        return on_owner() && !retired_ && ready_ &&
               index<selected_.size() && selected_[index] &&
               event==kRecoveredEntityQueryRegistrations[index].event;
    }
    const std::thread::id owner_;
    std::array<bool,2> selected_{{false,false}};
    std::uint64_t generation_=0;
    bool ready_=false;
    bool retired_=false;
};
} // namespace slinkywinky
