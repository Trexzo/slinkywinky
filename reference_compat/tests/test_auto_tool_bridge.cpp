#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void require(bool ok,const char* desc,int line) {
    if(!ok)throw std::runtime_error(
        "R228 Auto Tool line "+std::to_string(line)+": "+desc);
}
#define CHECK(x) require(!!(x),#x,__LINE__)
static AutoToolTickInput source() {
    AutoToolTickInput s;
    s.in_game=true;s.current_slot=2;
    s.attack_held=true;s.look_at_block=s.block_exists=true;
    s.held=AutoToolHeldKind::fist;
    s.switch_back=true;s.hotbar_claim_granted=true;
    s.stack_present[2]=true;s.stack_present[4]=true;
    s.stack_present[7]=true;
    s.mining_speed[2]=2.5f;s.mining_speed[4]=5.0f;
    s.mining_speed[7]=5.0f; // tie must prefer first cyclic slot
    return s;
}
static ExecutionFrame inventory_frame(const AutoToolTickInput& s) {
    ExecutionFrame f;f.event=kAutoToolInventoryEvent;
    f.world_present=f.player_present=true;
    f.auto_tool_inventory=AutoToolInventoryPayload{s,{},0};
    return f;
}
static void run() {
    CHECK(auto_tool_item_kind(false,-1)==AutoToolHeldKind::fist);
    for(int id:{256,257,258,269,270,271,273,274,275,
                277,278,279,284,285,286,359})
        CHECK(auto_tool_item_kind(true,id)==AutoToolHeldKind::tool);
    for(int id:{267,268,272,276,283})
        CHECK(auto_tool_item_kind(true,id)==AutoToolHeldKind::sword);
    CHECK(auto_tool_item_kind(true,260)==AutoToolHeldKind::other);
    AutoToolController model;
    auto s=source();
    s.attack_held=false;s.activation_time=2.9f;
    auto out=model.on_tick(s);
    CHECK(out.admitted && !out.conditions_met &&
          out.countdown_remaining==2 && out.propose_release_hotbar);
    s.attack_held=true;
    out=model.on_tick(s);
    CHECK(out.conditions_met && !out.proposed_selected_slot &&
          out.countdown_remaining==1 && out.propose_release_hotbar);
    out=model.on_tick(s);
    CHECK(!out.proposed_selected_slot && out.countdown_remaining==0);
    s.hotbar_claim_granted=false;
    out=model.on_tick(s);
    CHECK(!out.proposed_selected_slot && !out.propose_claim_hotbar);
    s.hotbar_claim_granted=true;
    out=model.on_tick(s);
    CHECK(out.propose_claim_hotbar && out.proposed_selected_slot==4 &&
          out.remembered_slot==2 && out.owned_selected_slot==4);
    // User changes hotbar manually: cancel previous restore, do not
    // override their chosen slot or retain this module's ownership.
    s.current_slot=6;
    out=model.on_tick(s);
    CHECK(out.admitted && out.propose_release_hotbar &&
          !out.proposed_selected_slot && out.remembered_slot==-1);
    model.reset();
    s=source();s.activation_time=0.0f;
    out=model.on_tick(s);
    CHECK(out.proposed_selected_slot==4);
    s.current_slot=4;
    out=model.on_tick(s);
    CHECK(!out.proposed_selected_slot && out.remembered_slot==2);
    s.attack_held=false;
    out=model.on_tick(s);
    CHECK(out.propose_release_hotbar && out.proposed_selected_slot==2 &&
          out.remembered_slot==-1);
    // Exact gating on held item class, physical sneak, target, block IDs.
    model.reset();
    s=source();s.allow_fists=false;
    out=model.on_tick(s);
    CHECK(!out.conditions_met && !out.proposed_selected_slot);
    s.held=AutoToolHeldKind::sword;s.allow_swords=false;
    CHECK(!model.on_tick(s).conditions_met);
    s.allow_swords=true;s.using_item=true;
    CHECK(!model.on_tick(s).conditions_met);
    s.using_item=false;s.require_sneak=true;
    CHECK(!model.on_tick(s).conditions_met);
    s.sneak_held=true;s.restrict_allowed_blocks=true;
    CHECK(!model.on_tick(s).conditions_met);
    s.block_is_allowed=true;
    CHECK(model.on_tick(s).proposed_selected_slot==4);

    model.reset();
    s=source();s.switch_back=false;
    out=model.on_tick(s);
    CHECK(out.proposed_selected_slot==4 && out.remembered_slot==-1);
    s.current_slot=4;s.attack_held=false;
    out=model.on_tick(s);
    CHECK(out.propose_release_hotbar && !out.proposed_selected_slot);

    model.reset();
    s=source();s.stack_present[4]=false;
    // Best speed ties at 7 only; no empty-stack selection.
    out=model.on_tick(s);
    CHECK(out.proposed_selected_slot==7);
    model.reset();
    s=source();s.current_slot=4;
    CHECK(!model.on_tick(s).proposed_selected_slot); // current wins tie
    model.reset();
    s=source();s.current_slot=9;
    CHECK(!model.on_tick(s).admitted);
    s=source();s.mining_speed[4]=
        std::numeric_limits<float>::quiet_NaN();
    CHECK(!model.on_tick(s).admitted);
    s=source();s.activation_time=-1.0f;
    CHECK(!model.on_tick(s).admitted);
    s=source();s.in_game=false;
    CHECK(!model.on_tick(s).admitted);
    s=source();
    CHECK(model.on_tick(s).admitted);
    CHECK(model.on_shutdown().proposed_release_allowed_block_list);
    CHECK(!model.on_shutdown().proposed_release_allowed_block_list);
    CHECK(!model.on_tick(s).admitted);
    model.reset();
    CHECK(model.on_tick(s).admitted);

    ModuleExecutionBridge bridge;
    const auto id=bridge.add({"Auto Tool",3,
        {{kAutoToolInventoryEvent,0},{kAutoToolShutdownEvent,0}},false});
    CHECK(bridge.bind_auto_tool_from_catalog());
    CHECK(!bridge.bind_auto_tool(id));
    auto coverage=bridge.coverage();
    CHECK(coverage.native_callback_subscriptions==2);
    CHECK(coverage.executable_callback_bindings==2);
    CHECK(coverage.implemented_per_event[kAutoToolInventoryEvent]==1);
    CHECK(coverage.implemented_per_event[kAutoToolShutdownEvent]==1);
    CHECK(bridge.request(id,true));
    CHECK(!bridge.apply(false,false));
    CHECK(bridge.apply(true,true));
    auto frame=inventory_frame(source());
    CHECK(bridge.dispatch(frame));
    CHECK(frame.auto_tool_inventory->executions==1 &&
          frame.auto_tool_inventory->decision.proposed_selected_slot==4);
    frame=inventory_frame(source());
    frame.event=kAutoToolShutdownEvent;
    CHECK(!bridge.dispatch(frame));
    frame=inventory_frame(source());frame.player_present=false;
    CHECK(!bridge.dispatch(frame));
    frame=inventory_frame(source());frame.world_present=false;
    CHECK(!bridge.dispatch(frame));
    frame=ExecutionFrame{};
    frame.event=kAutoToolShutdownEvent;
    frame.world_present=frame.player_present=true;
    frame.auto_tool_shutdown=AutoToolShutdownPayload{};
    CHECK(bridge.dispatch(frame) &&
          frame.auto_tool_shutdown->executions==1 &&
          frame.auto_tool_shutdown->decision.proposed_release_allowed_block_list);
    CHECK(bridge.dispatch(frame) &&
          !frame.auto_tool_shutdown->decision.proposed_release_allowed_block_list);
    frame=inventory_frame(source());
    CHECK(bridge.dispatch(frame) &&
          frame.auto_tool_inventory->executions==1 &&
          !frame.auto_tool_inventory->decision.admitted);
    bridge.reset_auto_tool_world_state();
    CHECK(bridge.dispatch(frame) &&
          frame.auto_tool_inventory->decision.proposed_selected_slot==4);
    CHECK(bridge.request(id,false));
    CHECK(bridge.apply(true,true));
    CHECK(bridge.dispatch(frame) && frame.auto_tool_inventory->executions==0);
    bridge.stop();
    CHECK(!bridge.dispatch(frame));
    for(const auto& bad:std::vector<ModuleSpec>{
        {"Auto Tool",1,{{25,0},{26,0}},false},
        {"Auto Tool",3,{{25,1},{26,0}},false},
        {"Auto Tool",3,{{25,0},{26,1}},false},
        {"Auto Tool",3,{{24,0},{26,0}},false},
        {"Auto Tool",3,{{25,0}},false},
        {"Auto Tool",3,{{25,0},{26,0},{5,0}},false},
        {"Other",3,{{25,0},{26,0}},false},
    }) {
        ModuleExecutionBridge invalid;
        const auto wrong=invalid.add(bad);
        CHECK(!invalid.bind_auto_tool(wrong));
        CHECK(!invalid.bind_auto_tool_from_catalog());
    }
    std::cout<<"R228_AUTO_TOOL_ORIGINAL_NATIVE_EVENT25_26_POLICY_PASS "
        "cyclicSpeed=1 manualOverride=1 restore=1 shutdown=1 "
        "gameplayWrites=0\n";
}
int main(){
    try{run();return 0;}
    catch(const std::exception& e){
        std::cerr<<e.what()<<"\n";return 1;
    }
}
