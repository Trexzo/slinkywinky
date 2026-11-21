#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void check(bool ok,const char* message,int line) {
    if(!ok)throw std::runtime_error(
        "R261 event25/26 line "+std::to_string(line)+": "+message);
}
#define CHECK(x) check(!!(x),#x,__LINE__)
static AutoWeaponInventoryPayload weapon_frame(float delay=1.5f) {
    AutoWeaponInventoryPayload p;
    p.input.input_ready=true;
    p.input.mouse_pressed=true;
    p.input.hit_is_entity=true;
    p.input.remote_other_player=true;
    p.input.hostile=true;
    p.input.action_owner_granted=true;
    p.input.world_token=43;p.input.player_token=56;
    p.input.current_slot=0;
    p.input.activation_time=delay;
    p.player_ticks_existed=1;
    p.input.current_is_weapon=true; // forged untrusted caller hint
    p.input.first_allowed_slot=8;    // must be ignored
    p.settings.swords=true;
    p.inventory.slots[3]={true,267,0};
    return p;
}
static ExecutionFrame frame25(float delay=1.5f,bool both=false) {
    ExecutionFrame f;f.event=kAutoToolInventoryEvent;
    f.world_present=f.player_present=true;
    f.auto_weapon_inventory=weapon_frame(delay);
    if(both) {
        AutoToolTickInput t;
        t.in_game=true;t.current_slot=0;t.held=AutoToolHeldKind::fist;
        t.using_item=false;t.look_at_block=false;
        f.auto_tool_inventory=AutoToolInventoryPayload{t,{},0};
    }
    return f;
}
static ExecutionFrame frame26() {
    ExecutionFrame f;f.event=kAutoToolShutdownEvent;
    f.world_present=f.player_present=true;
    f.weapons_clear_extras=WeaponsClearExtrasPayload{};
    f.auto_tool_shutdown=AutoToolShutdownPayload{};
    return f;
}
int main(){
    ModuleExecutionBridge bridge;
    const auto aw=bridge.add({"Auto Weapon",0,{{25,0}},false});
    const auto tool=bridge.add({"Auto Tool",3,{{25,0},{26,0}},false});
    const auto weapons=bridge.add({"Weapons",0,{{26,0}},false});
    CHECK(bridge.bind_auto_weapon_from_catalog());
    CHECK(bridge.bind_auto_tool_from_catalog());
    CHECK(bridge.bind_weapons_from_catalog());
    CHECK(!bridge.bind_auto_weapon(aw) && !bridge.bind_weapons(weapons));
    CHECK(bridge.coverage().implemented_per_event[25]==2);
    CHECK(bridge.coverage().implemented_per_event[26]==2);
    CHECK(bridge.request(aw,true)&&bridge.request(tool,true)&&
          bridge.request(weapons,true)&&bridge.apply(true,true));
    auto f=frame25(1.5f,true);
    CHECK(bridge.dispatch(f));
    CHECK((f.executed_modules==std::vector<std::size_t>{aw,tool}));
    CHECK(f.auto_weapon_inventory->source_choice.admitted &&
          f.auto_weapon_inventory->source_choice.first_allowed_slot==3 &&
          !f.auto_weapon_inventory->source_current_is_weapon);
    CHECK(f.auto_weapon_inventory->executions==1 &&
          f.auto_weapon_inventory->decision.admitted &&
          f.auto_weapon_inventory->decision.waiting);
    f=frame25(1.5f,true);CHECK(bridge.dispatch(f));
    CHECK(f.auto_weapon_inventory->decision.waiting &&
          f.auto_weapon_inventory->decision.selection_delay_ticks==2);
    f=frame25(1.5f,true);CHECK(bridge.dispatch(f));
    CHECK(f.auto_weapon_inventory->decision.proposed_slot_change &&
          f.auto_weapon_inventory->decision.proposed_slot==3);
    // Source Weapon rule forbids changing hotbar when current qualifies.
    f=frame25(0.0f);
    f.auto_weapon_inventory->settings.fists=true;
    f.auto_weapon_inventory->player_ticks_existed=2;
    CHECK(bridge.dispatch(f));
    CHECK(f.auto_weapon_inventory->source_current_is_weapon &&
          !f.auto_weapon_inventory->decision.proposed_slot_change);
    // Both source modules see event26; neither alters game/settings here.
    f=frame26();CHECK(bridge.dispatch(f));
    CHECK((f.executed_modules==std::vector<std::size_t>{tool,weapons}));
    CHECK(f.weapons_clear_extras->executions==1 &&
          f.weapons_clear_extras->proposed_clear_extra_items);
    CHECK(f.auto_tool_shutdown->executions==1 &&
          f.auto_tool_shutdown->decision.proposed_release_allowed_block_list);
    // Contradictory single-cursor claims are denied before dispatch.
    f=frame25(1.5f,true);
    f.auto_tool_inventory->input.look_at_block=true;
    CHECK(!bridge.dispatch(f));
    CHECK(f.executed_modules.empty() && f.auto_weapon_inventory->executions==0);
    f=frame25(1.5f,true);
    f.auto_tool_inventory->input.current_slot=2;
    CHECK(!bridge.dispatch(f));
    f=frame25(1.5f,true);
    f.auto_tool_inventory->input.using_item=true;
    CHECK(!bridge.dispatch(f));
    f=frame25();f.event=24;
    CHECK(!bridge.dispatch(f));
    // Malformed slot list refuses a new AutoWeapon transition.
    f=frame25(0.0f);
    f.auto_weapon_inventory->inventory.slots[8]={true,-3,0};
    CHECK(bridge.dispatch(f));
    CHECK(!f.auto_weapon_inventory->source_choice.admitted &&
          !f.auto_weapon_inventory->decision.admitted);
    // A native source handle is never allowed to run after disable.
    CHECK(bridge.request(aw,false) && bridge.apply(true,true));
    f=frame25(0.0f);CHECK(bridge.dispatch(f));
    CHECK(f.auto_weapon_inventory->executions==0 &&
          !f.auto_weapon_inventory->decision.proposed_slot_change);
    CHECK(bridge.request(aw,true) && bridge.apply(true,true));
    f=frame25(0.0f);CHECK(bridge.dispatch(f));
    CHECK(f.auto_weapon_inventory->decision.proposed_slot_change);
    // Source binding demands exact category, name and event subscriptions.
    ModuleExecutionBridge wrong;
    const auto wrongCategory=wrong.add({"Auto Weapon",3,{{25,0}},true});
    const auto wrongName=wrong.add({"Other",0,{{25,0}},true});
    CHECK(!wrong.bind_auto_weapon(wrongCategory));
    CHECK(!wrong.bind_auto_weapon(wrongName));
    ModuleExecutionBridge extra;
    const auto wrongEvents=extra.add({"Auto Weapon",0,{{25,0},{26,0}},true});
    CHECK(!extra.bind_auto_weapon(wrongEvents));
    ModuleExecutionBridge bad;
    const auto wrongWeapons=bad.add({"Weapons",0,{{25,0}},true});
    CHECK(!bad.bind_weapons(wrongWeapons));
    // Reject false acceptance after an explicit bridge shutdown.
    bridge.stop();
    f=frame25(0.0f);
    CHECK(!bridge.dispatch(f));
    CHECK(!bridge.bind_auto_weapon(aw));
    std::cout<<"R261_OFFLINE_AUTO_WEAPON_WEAPONS_EVENT25_EVENT26_PASS\n";
}
