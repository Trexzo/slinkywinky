#pragma once
#include "slinkywinky/inventory_snapshot_types.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
#include <vector>
namespace slinkywinky::reference_compat {
// Caller supplies an explicit continuation trace rather than simulating input.
class AutoSoupController {
public:
    struct Match {int slot;bool leaves_bowl;};
    struct Threshold {bool first,second;};
    static Threshold health(float value,float single,float twice){
        if(!std::isfinite(value)||!std::isfinite(single)||!std::isfinite(twice))
            throw std::invalid_argument("health bounds");
        return {value<=single,value<=single&&value<=twice};
    }
    static Match classify(const HotbarSample& slots,unsigned current,bool busy,
            bool custom,const std::vector<unsigned>& ids,const std::array<bool,9>& keep_looking){
        if(current>=9)throw std::invalid_argument("selected slot");
        Match found{-1,false};
        unsigned visited=0;
        for(unsigned i=0;i<9;++i){
            unsigned slot=(current+i)%9;
            if(busy&&slot==current)continue;
            const auto& item=slots[slot];
            unsigned id=item.occupied?item.id:0;
            bool stew=id==282||id==413;
            if(custom?std::find(ids.begin(),ids.end(),id)==ids.end():!stew)continue;
            found={static_cast<int>(slot),stew};
            if(!keep_looking[visited++])break;
        }
        return found;
    }
};
}
