#pragma once
#include "slinkywinky/inventory_snapshot_types.hpp"
#include <cmath>
#include <limits>
#include <stdexcept>
namespace slinkywinky::reference_compat {
// Offline source classification and activation accounting. No throw execution.
class AutoPotController {
public:
    struct Settings {bool non_health,prevent_waste,double_splash;};
    struct Count {int initial,extra;};
    static int first_slot(const HotbarSample& slots,bool non_health) noexcept {
        for(unsigned i=0;i<slots.size();++i)
            if(splash_potion(slots[i])&&(non_health||healing_potion(slots[i])))
                return static_cast<int>(i);
        return 9; // source sentinel
    }
    static Count count(const Settings& s,bool in_game,float health,float maximum){
        if(!in_game)return {0,0};
        if(!std::isfinite(health)||!std::isfinite(maximum)||health<0||maximum<health)
            throw std::invalid_argument("health bounds");
        const double raw=(static_cast<double>(maximum-health)+.5)*.125;
        if(raw>std::numeric_limits<int>::max())throw std::invalid_argument("health overflow");
        int needed=static_cast<int>(raw);
        if(needed==0){if(s.prevent_waste)return {0,0};needed=1;}
        return s.double_splash?Count{needed,0}:Count{1,s.prevent_waste?needed-1:5};
    }
};
}
