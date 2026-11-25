#pragma once
#include "slinkywinky/inventory_snapshot_types.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace slinkywinky::reference_compat {
// Source slot classification only; offsets are explicit evidence inputs.
class RefillController {
public:
    struct Settings {bool potions;unsigned non_health_offset,soup_offset;};
    static std::array<unsigned,27> classify(const MainInventorySample& slots,const Settings& s){
        if(s.non_health_offset>=255||s.soup_offset>=255)
            throw std::invalid_argument("class offset outside source byte");
        std::array<unsigned,27> result{};
        for(unsigned i=0;i<slots.size();++i){
            const auto& item=slots[i];
            if(!item.has_item())continue;
            unsigned kind=1;
            if(item.id==282)kind=s.soup_offset+1;
            else if(s.potions&&splash_potion(item))
                kind=healing_potion(item)?2:s.non_health_offset+1;
            result[i]=kind;
        }
        return result;
    }
    static bool hotbar_has_space(const HotbarSample& slots) noexcept{
        return std::any_of(slots.begin(),slots.end(),[](const InventorySampleItem& i){return !i.has_item();});
    }
};
}
