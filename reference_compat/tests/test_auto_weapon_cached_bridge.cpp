#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void check(bool yes,const char* message,int line){
    if(!yes)throw std::runtime_error("R263 cache "+std::to_string(line)+": "+message);
}
#define CHECK(x) check(!!(x),#x,__LINE__)
static ExecutionFrame query(int tick,bool sword=true) {
    ExecutionFrame f;f.event=kAutoToolInventoryEvent;
    f.world_present=f.player_present=true;
    AutoWeaponInventoryPayload p;
    p.player_ticks_existed=tick;
    p.input.input_ready=true;
    p.input.mouse_pressed=true;
    p.input.hit_is_entity=true;
    p.input.remote_other_player=true;
    p.input.hostile=true;
    p.input.action_owner_granted=true;
    p.input.world_token=1;
    p.input.player_token=2;
    p.input.current_slot=0;
    p.input.activation_time=0.0f;
    p.input.first_allowed_slot=8; // must be replaced
    p.input.current_is_weapon=false; // must be replaced
    p.settings.swords=true;
    if(sword) p.inventory.slots[0]={true,276,0};
    p.inventory.slots[3]={true,267,0};
    f.auto_weapon_inventory=p;
    return f;
}
int main(){
    ModuleExecutionBridge b;
    const auto weapon=b.add({"Auto Weapon",0,{{25,0}},false});
    CHECK(b.bind_auto_weapon_from_catalog());
    CHECK(b.request(weapon,true) && b.apply(true,true));
    // Initially Cache.key==0, so a tick0 query returns source's default
    // 'false' even if a sword is held. firstAllowedSlot then runs.
    auto f=query(0);
    CHECK(b.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_cache_queried);
    CHECK(f.auto_weapon_inventory->source_cache.admitted);
    CHECK(!f.auto_weapon_inventory->source_cache.refreshed);
    CHECK(!f.auto_weapon_inventory->source_current_is_weapon);
    CHECK(f.auto_weapon_inventory->source_choice.first_allowed_slot==3 ||
          f.auto_weapon_inventory->source_choice.first_allowed_slot==0);
    // The selected sword item is eligible at tick1; no switch proposed.
    f=query(1);
    CHECK(b.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_cache.refreshed);
    CHECK(f.auto_weapon_inventory->source_current_is_weapon);
    CHECK(!f.auto_weapon_inventory->source_choice.admitted);
    CHECK(!f.auto_weapon_inventory->decision.proposed_slot_change);
    // Switch to a fist DURING tick1: cached true remains true.
    f=query(1,false);
    CHECK(b.dispatch(f));
    CHECK(!f.auto_weapon_inventory->source_cache.refreshed);
    CHECK(f.auto_weapon_inventory->source_current_is_weapon);
    CHECK(!f.auto_weapon_inventory->decision.proposed_slot_change);
    // Tick2 finally recaptures current fist; source can switch to slot3.
    f=query(2,false);
    CHECK(b.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_cache.refreshed);
    CHECK(!f.auto_weapon_inventory->source_current_is_weapon);
    CHECK(f.auto_weapon_inventory->source_choice.first_allowed_slot==3);
    CHECK(f.auto_weapon_inventory->decision.proposed_slot_change &&
          f.auto_weapon_inventory->decision.proposed_slot==3);
    // Source early gates skip Weapons.isWeapon entirely.
    f=query(3,false);
    f.auto_weapon_inventory->input.hit_is_entity=false;
    CHECK(b.dispatch(f));
    CHECK(!f.auto_weapon_inventory->source_cache_queried);
    CHECK(!f.auto_weapon_inventory->decision.proposed_slot_change);
    // Same tick3 now has a hostile entity: new cache read happens.
    f=query(3,false);
    CHECK(b.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_cache_queried &&
          f.auto_weapon_inventory->source_cache.refreshed &&
          !f.auto_weapon_inventory->source_current_is_weapon);
    // If settings changed on same tick3, classification is still false.
    f=query(3,false);
    f.auto_weapon_inventory->settings.fists=true;
    CHECK(b.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_cache.admitted &&
          !f.auto_weapon_inventory->source_cache.refreshed &&
          !f.auto_weapon_inventory->source_current_is_weapon);
    // Tick4 with fists enabled refreshes and suppresses Auto Weapon.
    f=query(4,false);
    f.auto_weapon_inventory->settings.fists=true;
    CHECK(b.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_current_is_weapon &&
          !f.auto_weapon_inventory->decision.proposed_slot_change);
    // Source owner guard rejects a world/player switch before cache reuse.
    f=query(5,false);f.auto_weapon_inventory->input.player_token=77;
    CHECK(b.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_cache_queried &&
          !f.auto_weapon_inventory->source_cache.admitted &&
          !f.auto_weapon_inventory->decision.admitted);
    // Next observation creates a new owner (safe divergent recovery).
    f=query(5,false);f.auto_weapon_inventory->input.player_token=77;
    CHECK(b.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_cache.refreshed &&
          f.auto_weapon_inventory->decision.proposed_slot_change);
    // An exception in a later callback must roll back tick cache and
    // AutoWeapon cooldown together.
    ModuleExecutionBridge tx;
    const auto aw=tx.add({"Auto Weapon",0,{{25,100}},false});
    const auto thrower=tx.add({"Thrower",0,{{25,-5}},true});
    CHECK(tx.bind_auto_weapon(aw));
    bool fail_once=true;
    CHECK(tx.bind(thrower,25,[&](const Delivery&,ExecutionFrame&){
        if(fail_once){fail_once=false;throw std::runtime_error("fixture");}
    }));
    CHECK(tx.request(aw,true) && tx.apply(true,true));
    bool threw=false;
    f=query(1,false);
    f.auto_weapon_inventory->input.activation_time=1.5f;
    try{tx.dispatch(f);}catch(const std::runtime_error&){threw=true;}
    CHECK(threw);
    f=query(1,false);f.auto_weapon_inventory->input.activation_time=1.5f;
    CHECK(tx.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_cache.refreshed);
    CHECK(f.auto_weapon_inventory->decision.waiting &&
          f.auto_weapon_inventory->decision.selection_delay_ticks==1);
    CHECK(tx.request(aw,false) && tx.apply(true,true));
    CHECK(tx.request(aw,true) && tx.apply(true,true));
    f=query(1,false);
    CHECK(tx.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_cache.refreshed &&
          f.auto_weapon_inventory->decision.proposed_slot_change);
    b.stop();tx.stop();
    std::cout<<"R263_AUTO_WEAPON_SOURCE_TICK_CACHE_BRIDGE_PASS\n";
}
