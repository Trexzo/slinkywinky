#pragma once
// R262: first-party value-only model of WeaponsModule.Cache:
// key initially 0; only refresh when player.ticksExisted differs.
// A separate world/player identity fence deliberately refuses stale data
// rather than carrying an original cache into a new Minecraft session.
#include "slinkywinky/weapons_controller.hpp"
#include <cstdint>
#include <limits>

namespace slinkywinky::reference_compat {
struct WeaponsTickObservation {
    std::uint64_t world_token=0,player_token=0;
    int player_ticks_existed=0;
    int current_slot=0;
    WeaponsInventory inventory{};
    WeaponsSettings settings{};
};
struct WeaponsTickReceipt {
    bool admitted=false,refreshed=false;
    bool cached_allowed=false;
    unsigned cached_enchant_groups=0;
    int cached_tick=0;
};
class WeaponsTickCache final {
public:
    WeaponsTickReceipt on_query(const WeaponsTickObservation& observation) noexcept {
        WeaponsTickReceipt out;
        if(!valid(observation))return out;
        if(world_!=0 && (world_!=observation.world_token ||
                         player_!=observation.player_token ||
                         observation.player_ticks_existed<last_tick_)) {
            reset();
            return out;
        }
        if(world_==0) {
            world_=observation.world_token;
            player_=observation.player_token;
        }
        out.admitted=true;
        // Exact Java semantics: initially key=0 so a tick-0 query reuses
        // false and 0 without taking any classification snapshot.
        if(last_tick_!=observation.player_ticks_existed) {
            last_tick_=observation.player_ticks_existed;
            const auto& stack=observation.inventory.slots[
                static_cast<std::size_t>(observation.current_slot)];
            cached_groups_=stack.present?stack.enchant_groups:0u;
            cached_allowed_=WeaponsController::allowed_current(
                stack,observation.current_slot,observation.settings);
            out.refreshed=true;
        }
        out.cached_tick=last_tick_;
        out.cached_enchant_groups=cached_groups_;
        out.cached_allowed=cached_allowed_;
        return out;
    }
    void reset() noexcept {
        world_=player_=0;
        last_tick_=0;
        cached_groups_=0;
        cached_allowed_=false;
    }
    int key() const noexcept {return last_tick_;}
private:
    static bool valid(const WeaponsTickObservation& o) noexcept {
        if(o.world_token==0 || o.player_token==0 ||
           o.player_ticks_existed<0 || o.current_slot<0 ||
           o.current_slot>=9)return false;
        const auto& held=o.inventory.slots[
            static_cast<std::size_t>(o.current_slot)];
        if((held.present && held.identifier<0) ||
           (held.enchant_groups & ~7u)!=0u)return false;
        for(int id:o.settings.extra_item_identifiers)if(id<0)return false;
        return true;
    }
    std::uint64_t world_=0,player_=0;
    int last_tick_=0;
    unsigned cached_groups_=0;
    bool cached_allowed_=false;
};
} // namespace slinkywinky::reference_compat
