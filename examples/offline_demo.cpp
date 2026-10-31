#include "slinkywinky/combat.hpp"
#include "slinkywinky/settings.hpp"
#include <iostream>
using namespace slinkywinky;
int main(int argc, char** argv) {
    ClientSettings config;
    if(argc > 1) {
        auto read = load_settings(argv[1]);
        if(!read.valid) {std::cerr<<read.error<<'\n';return 2;}
        config=read.settings;
    } else {
        config.combat.auto_clicker_enabled=true;
        config.combat.aim_enabled=true;
    }
    CombatCoordinator coordinator({1,2,3,4});
    TickSnapshot snapshot;
    snapshot.world_valid=true;
    snapshot.generation=1;
    snapshot.holding_weapon=true;
    snapshot.mouse_left=true;
    snapshot.target=Target{5,{2,0,0},true,true};
    std::cout<<"time_ms,attack,left_edge,right_edge,yaw,pitch\n";
    for (std::uint64_t i=0;i<30;++i) {
        snapshot.elapsed_ms=100 + i*20;
        const auto result=coordinator.tick(snapshot,config.combat);
        std::cout<<snapshot.elapsed_ms<<','<<result.attempt_attack<<','
                 <<static_cast<int>(result.left_edge)<<','<<static_cast<int>(result.right_edge)<<',';
        if(result.rotation) std::cout<<result.rotation->yaw<<','<<result.rotation->pitch;
        else std::cout<<"none,none";
        std::cout<<'\n';
    }
    return 0;
}
