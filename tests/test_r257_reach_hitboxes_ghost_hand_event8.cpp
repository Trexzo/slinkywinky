// R257 — three source-defined event-8 proposals on the same
// offline owner thread. No assumptions about unverified callback order.
#include "slinkywinky/recovered_event8_query_owner.hpp"
#include "slinkywinky/menu.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
using namespace slinkywinky;
#define CHECK(x) do{if(!(x))throw std::runtime_error( \
    "R257 line "+std::to_string(__LINE__)+": " #x);}while(false)

int main(){
    const auto& catalog=official_module_catalog();
    CHECK(std::string(catalog[12].name)=="Reach");
    CHECK(std::string(catalog[7].name)=="Hitboxes");
    CHECK(std::string(catalog[48].name)=="Ghost Hand");
    CHECK(candidate_native_manager_registered("Reach"));
    CHECK(candidate_native_manager_registered("Hitboxes"));
    CHECK(candidate_native_manager_registered("Ghost Hand"));
    CHECK(!candidate_native_manager_registered("Chest Stealer"));

    RecoveredEvent8Owner owner;
    RecoveredEvent8Input input;
    input.reach.sprinting=true;
    input.reach_settings.distance=4.0f;
    input.reach_settings.activation_ticks=3.0f;
    input.expansion=0.5f;
    input.ghost_settings.allow_swords=true;
    input.ghost_settings.ignore_enemies=true;
    input.ghost_query.held_item_present=true;
    input.ghost_query.held_item_id=267;
    input.ghost_query.entity_is_other_player=true;
    CHECK(!owner.query(8,input) && !owner.transition(0,true));
    CHECK(owner.transition(10,true));
    auto disabled=owner.query(8,input);
    CHECK(disabled && !disabled->reach_callback_ran &&
          !disabled->proposed_reach && !disabled->hitboxes &&
          !disabled->ghost_hand);
    CHECK(owner.select(RecoveredEvent8Module::Reach,true));
    CHECK(owner.select(RecoveredEvent8Module::Hitboxes,true));
    CHECK(owner.select(RecoveredEvent8Module::GhostHand,true));
    const auto enabled=owner.snapshot(1u);
    CHECK(enabled && enabled->modules.size()==3);
    CHECK(enabled->modules[0].canonical_name=="Reach");
    CHECK(enabled->modules[1].canonical_name=="Hitboxes");
    CHECK(enabled->modules[2].canonical_name=="Ghost Hand");
    CHECK(make_module_arraylist_display(*enabled)->active_names.size()==3);

    auto all=owner.query(8,input);
    CHECK(all && all->reach_callback_ran && all->proposed_reach &&
          *all->proposed_reach==4.0 && all->hitboxes &&
          all->hitboxes->proposed_active &&
          all->hitboxes->expansion_per_side==0.5 &&
          all->ghost_hand && all->ghost_hand->reach_query_marker &&
          all->ghost_hand->ignore_entity);
    CHECK(owner.reach_cooldown_ticks()==0);
    input.reach.last_attacker_time=1;
    all=owner.query(8,input);
    CHECK(all && all->reach_callback_ran &&
          !all->proposed_reach && owner.reach_cooldown_ticks()==9);
    CHECK(all->hitboxes && all->ghost_hand); // independent proposals
    for(int i=1;i<=7;++i){
        all=owner.query(8,input);
        CHECK(all);
        CHECK(all->proposed_reach.has_value()==(i==7));
        CHECK(owner.reach_cooldown_ticks()==9-i);
        CHECK(all->hitboxes && all->ghost_hand);
    }
    CHECK(!owner.query(14,input));
    CHECK(owner.reach_cooldown_ticks()==2); // invalid event never ticks
    input.reach_settings.distance=100.0f;
    all=owner.query(8,input);
    CHECK(all && all->reach_callback_ran && !all->proposed_reach);
    CHECK(owner.reach_cooldown_ticks()==2); // invalid bound no state change
    CHECK(all->hitboxes && all->ghost_hand);
    input.reach_settings.distance=4.0f;
    CHECK(owner.select(RecoveredEvent8Module::Reach,false));
    CHECK(!owner.selected(RecoveredEvent8Module::Reach));
    CHECK(owner.reach_cooldown_ticks()==0);
    all=owner.query(8,input);
    CHECK(all && !all->reach_callback_ran &&
          all->hitboxes && all->ghost_hand);
    CHECK(owner.select(RecoveredEvent8Module::Reach,true));
    all=owner.query(8,input);
    CHECK(all && !all->proposed_reach &&
          owner.reach_cooldown_ticks()==9); // new disabled state resets
    CHECK(owner.transition(10,false));
    CHECK(!owner.query(8,input));
    auto worldless=owner.snapshot(2u);
    CHECK(worldless && worldless->modules.size()==3);
    CHECK(!worldless->world_ready &&
          worldless->modules[0].requested_enabled &&
          !worldless->modules[0].active_in_world);
    CHECK(make_module_arraylist_display(*worldless)->active_names.empty());
    CHECK(!owner.transition(10,true));
    CHECK(owner.transition(11,true));
    CHECK(owner.reach_cooldown_ticks()==0);
    CHECK(!owner.transition(10,true));
    bool foreign=false;
    std::thread worker([&]{
        foreign=owner.transition(12,true) ||
           owner.select(RecoveredEvent8Module::GhostHand,false) ||
           owner.query(8,input).has_value() ||
           owner.snapshot(3u).has_value();
    });
    worker.join();
    CHECK(!foreign && owner.generation()==11);
    CHECK(owner.retire() && owner.retired());
    CHECK(!owner.transition(12,true));
    CHECK(!owner.select(RecoveredEvent8Module::Reach,true));
    CHECK(!owner.snapshot(4u) && !owner.query(8,input));
    std::cout<<"R257_REACH_HITBOXES_GHOSTHAND_SHARED_EVENT8_PROPOSALS_PASS\n";
    std::cout<<"R257_ATTACK_COOLDOWN_WORLD_GENERATION_AND_OWNER_FENCE_PASS\n";
    std::cout<<"R257_NO_SOURCE_DISPATCH_PRIORITY_OR_GAME_HOOK_CLAIM\n";
}
