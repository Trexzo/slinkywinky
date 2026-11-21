#pragma once
// R260: first-party Weapons policy and 9-slot ordering from the user's
// reconstructed Java source. No inventory mutation or Minecraft access.
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

namespace slinkywinky::reference_compat {
struct WeaponItem {
    bool present=false;
    int identifier=0;
    unsigned enchant_groups=0; // sharpness=1, knockback=2, fire_aspect=4
};
struct WeaponsSettings {
    std::array<bool,9> selected_slots{};
    bool fists=false,swords=false,axes=false;
    bool sharpness=false,knockback=false,fire_aspect=false;
    std::vector<int> extra_item_identifiers{};
};
struct WeaponsInventory {
    std::array<WeaponItem,9> slots{};
};
struct WeaponsChoice {
    bool admitted=false;
    int first_allowed_slot=-1;
};
class WeaponsController final {
public:
    static unsigned enchantment_group(unsigned id) noexcept {
        if(id==16u)return 1u;
        if(id==19u)return 2u;
        if(id==20u)return 4u;
        return 0u;
    }
    static bool group_allowed(unsigned groups,
                              const WeaponsSettings& s) noexcept {
        return ((groups&1u) && s.sharpness) ||
               ((groups&2u) && s.knockback) ||
               (groups>=4u && s.fire_aspect);
    }
    static bool identifier_allowed(int id,
                                   const WeaponsSettings& s) noexcept {
        if(id==0)return s.fists;
        // Source's bitmasks for vanilla item IDs 258..286:
        switch(id) {
            case 267:case 268:case 272:case 276:case 283:
                return s.swords;
            case 258:case 271:case 275:case 279:case 286:
                return s.axes;
            default:
                return std::find(s.extra_item_identifiers.begin(),
                                 s.extra_item_identifiers.end(),id)!=
                       s.extra_item_identifiers.end();
        }
    }
    static bool allowed_current(const WeaponItem& held,int selected,
                                const WeaponsSettings& s) noexcept {
        if(selected<0 || selected>=9)return false;
        if(!held.present)return s.fists;
        return s.selected_slots[static_cast<std::size_t>(selected)] ||
               group_allowed(held.enchant_groups,s) ||
               identifier_allowed(held.identifier,s);
    }
    static WeaponsChoice first_allowed_slot(
        const WeaponsInventory& inventory,
        const WeaponsSettings& settings) noexcept {
        WeaponsChoice out;
        for(int id:settings.extra_item_identifiers)if(id<0)return out;
        for(const auto& item:inventory.slots)
            if((item.present && item.identifier<0) ||
               (item.enchant_groups&~7u)!=0u)return out;
        out.admitted=true;
        for(int slot=0;slot<9;++slot) {
            if(settings.selected_slots[static_cast<std::size_t>(slot)]) {
                out.first_allowed_slot=slot;
                return out;
            }
            const auto& item=inventory.slots[static_cast<std::size_t>(slot)];
            if(item.present &&
               (group_allowed(item.enchant_groups,settings) ||
                identifier_allowed(item.identifier,settings))) {
                out.first_allowed_slot=slot;
                return out;
            }
        }
        return out;
    }
    // Reconstructed event26 clears the extra-items list; never mutate it.
    static bool event26_requests_clear_extras(bool module_enabled) noexcept {
        return module_enabled;
    }
};
} // namespace slinkywinky::reference_compat
