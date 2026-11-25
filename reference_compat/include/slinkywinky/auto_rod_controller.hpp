#pragma once
#include "slinkywinky/inventory_snapshot_types.hpp"
namespace slinkywinky::reference_compat {
// Offline item-selection parity only; no aiming, throwing or timing.
class AutoRodController {
public:
    struct Match {int slot;bool fishing_rod;};
    static Match classify(const HotbarSample& slots,bool prefer_throwables) noexcept {
        Match found{-1,false};
        for(unsigned i=0;i<slots.size();++i){
            const auto& item=slots[i];
            if(!item.has_item())continue;
            if(item.id==332||item.id==344){
                if(prefer_throwables)return {static_cast<int>(i),false};
                continue;
            }
            if(item.id!=346)continue;
            found={static_cast<int>(i),true};
            if(!prefer_throwables)return found;
        }
        return found; // last rod is fallback if throwables allowed
    }
};
}
