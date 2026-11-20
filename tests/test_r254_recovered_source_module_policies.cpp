#include "slinkywinky/recovered_source_module_policies.hpp"
#include "slinkywinky/menu.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <string>
using namespace slinkywinky;
#define CHECK(c) do {if(!(c))throw std::runtime_error( \
    "R254 line "+std::to_string(__LINE__)+": " #c);}while(false)
static bool close(double a,double b){
    return std::abs(a-b)<1e-9;
}
int main(){
    CHECK(std::string(kRecoveredSourceRegistrations[0].name)=="No Hit Delay");
    CHECK(std::string(kRecoveredSourceRegistrations[1].name)=="Safe Walk");
    CHECK(std::string(kRecoveredSourceRegistrations[2].name)=="Fast Place");
    CHECK(kRecoveredSourceRegistrations[0].category==4 &&
          kRecoveredSourceRegistrations[0].event==14 &&
          kRecoveredSourceRegistrations[0].priority==0);
    CHECK(kRecoveredSourceRegistrations[1].category==1 &&
          kRecoveredSourceRegistrations[1].event==0 &&
          kRecoveredSourceRegistrations[1].priority==0);
    CHECK(kRecoveredSourceRegistrations[2].category==3 &&
          kRecoveredSourceRegistrations[2].event==0 &&
          kRecoveredSourceRegistrations[2].priority==-1000);
    const auto& catalog=official_module_catalog();
    CHECK(std::string(catalog[60].name)=="No Hit Delay");
    CHECK(std::string(catalog[24].name)=="Safe Walk");
    CHECK(std::string(catalog[47].name)=="Fast Place");
    CHECK(candidate_native_manager_registered("No Hit Delay"));
    CHECK(candidate_native_manager_registered("Safe Walk"));
    CHECK(candidate_native_manager_registered("Fast Place"));

    RecoveredNoHitInput attack;
    attack.attack_click=true;attack.jvm_ready=true;attack.blocked_ticks=10;
    CHECK(decide_no_hit_delay(attack).reset_counter);
    attack.mode=1;
    CHECK(decide_no_hit_delay(attack).propose_swing);
    attack.using_item=true;
    CHECK(!decide_no_hit_delay(attack).propose_swing);
    attack.using_item=false;
    attack.target_present=true;attack.target_kind=1;
    CHECK(decide_no_hit_delay(attack).reset_counter);
    attack.blocked_ticks=0;
    CHECK(!decide_no_hit_delay(attack).reset_counter);
    attack.blocked_ticks=10;

    RecoveredSafeWalkInput walk;
    walk.in_game=true;walk.pitch=65.0f;walk.holding_blocks=true;
    walk.on_ground=true;walk.motion_x=0.8;
    walk.motion_y=0.25;walk.motion_z=-0.4;
    walk.settings={true,true,true,0.5f};
    auto movement=decide_safe_walk(walk);
    CHECK(movement.edge_guard && *movement.edge_guard &&
          movement.motion && close(movement.motion->x,0.4) &&
          close(movement.motion->y,0.25) &&
          close(movement.motion->z,-0.2));
    walk.pitch=64.9f;
    movement=decide_safe_walk(walk);
    CHECK(movement.edge_guard && !*movement.edge_guard && !movement.motion);
    walk.pitch=65.0f;
    walk.settings.horizontal_multiplier=
        std::numeric_limits<float>::quiet_NaN();
    CHECK(!decide_safe_walk(walk).edge_guard.has_value());
    walk.settings.horizontal_multiplier=0.5f;

    RecoveredSourcePolicyOwner owner;
    CHECK(!owner.transition(0,true));
    CHECK(owner.transition(100,true));
    CHECK(!owner.attack(14,attack));  // disabled is never a decision
    CHECK(!owner.safe_walk(0,walk));
    CHECK(!owner.fast_place(0,{}));
    CHECK(owner.select(RecoveredSourceModule::NoHitDelay,true));
    CHECK(owner.select(RecoveredSourceModule::SafeWalk,true));
    CHECK(owner.select(RecoveredSourceModule::FastPlace,true));
    CHECK(owner.attack(14,attack)->reset_counter);
    CHECK(!owner.attack(0,attack));
    CHECK(owner.safe_walk(0,walk).has_value());
    CHECK(!owner.safe_walk(14,walk));
    auto initial=owner.snapshot(1);
    CHECK(initial && initial->modules.size()==3);
    auto display=make_module_arraylist_display(*initial);
    CHECK(display && display->active_names.size()==3);
    CHECK(initial->modules[0].canonical_name=="No Hit Delay");
    CHECK(initial->modules[1].canonical_name=="Safe Walk");
    CHECK(initial->modules[2].canonical_name=="Fast Place");
    RecoveredFastPlaceInput input;
    input.in_game=true;input.use_held=true;input.holds_block=true;
    input.target_block=true;input.cps=100.0f;
    auto fast=owner.fast_place(0,input);
    CHECK(fast && fast->valid && fast->proposed_presses==0);
    fast=owner.fast_place(0,input);
    CHECK(fast && fast->valid && fast->proposed_presses>0);
    CHECK(!owner.fast_place(14,input));
    CHECK(owner.transition(100,false));
    CHECK(!owner.attack(14,attack) && !owner.safe_walk(0,walk) &&
          !owner.fast_place(0,input));
    auto worldless=owner.snapshot(2);
    CHECK(worldless && !worldless->world_ready &&
          worldless->modules[0].requested_enabled &&
          !worldless->modules[0].active_in_world);
    CHECK(make_module_arraylist_display(*worldless)->active_names.empty());
    CHECK(!owner.transition(100,true));
    CHECK(owner.transition(101,true));
    fast=owner.fast_place(0,input);
    CHECK(fast && fast->valid && fast->proposed_presses==0); // new world first press
    CHECK(!owner.transition(100,true)); // stale generation
    CHECK(owner.select(RecoveredSourceModule::FastPlace,false));
    CHECK(!owner.fast_place(0,input));
    CHECK(owner.select(RecoveredSourceModule::FastPlace,true));
    fast=owner.fast_place(0,input);
    CHECK(fast && fast->valid && fast->proposed_presses==0); // new first press
    bool foreign_admitted=true;
    std::thread foreign([&]{
        foreign_admitted=owner.transition(102,true) ||
            owner.select(RecoveredSourceModule::SafeWalk,false) ||
            owner.attack(14,attack).has_value() ||
            owner.snapshot(3).has_value();
    });
    foreign.join();
    CHECK(!foreign_admitted && owner.generation()==101);
    CHECK(owner.retire());
    CHECK(owner.retired() && !owner.snapshot(4) &&
          !owner.attack(14,attack) && !owner.fast_place(0,input) &&
          !owner.transition(102,true) &&
          !owner.select(RecoveredSourceModule::SafeWalk,true));
    std::cout<<"R254_THREE_SOURCE_MODULE_REGISTRATIONS_AND_GUI_IDENTITIES_PASS\n";
    std::cout<<"R254_NO_HIT_SAFE_WALK_FAST_PLACE_OFFLINE_POLICIES_PASS\n";
    std::cout<<"R254_NATIVE_OWNER_WORLD_LOSS_PRIORITY_AND_UNLOAD_PASS\n";
    std::cout<<"R254_NO_MINECRAFT_HOOK_INPUT_INJECTION_OR_GAME_WRITES\n";
}
