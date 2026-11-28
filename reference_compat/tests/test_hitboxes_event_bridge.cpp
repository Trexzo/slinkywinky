#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void check(bool yes,const char* expr,int line){
    if(!yes)throw std::runtime_error("R265 Hitboxes "+std::to_string(line)+": "+expr);
}
#define CHECK(x) check(!!(x),#x,__LINE__)
static ExecutionFrame query(){
    ExecutionFrame f;f.event=kReachQueryEvent;
    f.world_present=f.player_present=true;
    ReachQueryPayload reach;
    reach.input.sprinting=true;reach.settings.distance=3.0f;
    f.reach=reach;
    GhostHandQueryPayload ghost;
    ghost.input.game_ready=true;ghost.input.module_enabled=true;
    ghost.input.holding_item=true;ghost.input.item_id=267;
    ghost.input.target_is_remote_player=true;
    ghost.settings.enemies=true;ghost.settings.allow_using_sword=true;
    f.ghost_hand=ghost;
    HitboxesQueryPayload hit;
    hit.input.expand=0.25f;
    hit.input.expansion_setting_configured=true;
    hit.input.dispatched_to_enabled_module=false; // forged caller
    f.hitboxes=hit;
    return f;
}
int main(){
    ModuleExecutionBridge b;
    const auto hit=b.add({"Hitboxes",0,{{8,50}},false});
    const auto reach=b.add({"Reach",0,{{8,40}},false});
    const auto ghost=b.add({"Ghost Hand",3,{{8,0}},false});
    CHECK(b.bind_hitboxes_from_catalog() && b.bind_reach(reach) &&
          b.bind_ghost_hand(ghost));
    CHECK(b.coverage().implemented_per_event[8]==3);
    CHECK(!b.bind_hitboxes(hit));
    CHECK(b.request(hit,true) && b.request(reach,true) &&
          b.request(ghost,true) && b.apply(true,true));
    auto frame=query();
    CHECK(b.dispatch(frame));
    CHECK((frame.executed_modules==std::vector<std::size_t>{hit,reach,ghost}));
    CHECK(frame.hitboxes->executions==1 &&
          frame.hitboxes->decision.admitted &&
          frame.hitboxes->decision.proposed_enable_event &&
          frame.hitboxes->decision.proposed_expansion==0.25 &&
          frame.hitboxes->decision.proposed_status_visible);
    CHECK(frame.reach->executions==1 &&
          frame.ghost_hand->executions==1 &&
          frame.ghost_hand->query_override);
    // Any subset of the three can share event8, but foreign typed
    // payloads or a wrong event cannot be smuggled into the frame.
    frame=query();frame.ghost_hand.reset();
    CHECK(b.dispatch(frame));
    CHECK((frame.executed_modules==std::vector<std::size_t>{hit,reach,ghost}));
    frame=query();frame.reach.reset();
    CHECK(b.dispatch(frame));
    frame=query();frame.hitboxes.reset();
    CHECK(b.dispatch(frame));
    frame=query();frame.event=14;
    CHECK(!b.dispatch(frame) && frame.hitboxes->executions==0);
    // The enabled handler controls event.enabled, not caller's flag.
    frame=query();
    frame.hitboxes->input.dispatched_to_enabled_module=false;
    CHECK(b.dispatch(frame) &&
          frame.hitboxes->decision.proposed_enable_event);
    // Invalid numeric settings cannot leave a nonfinite event proposal.
    frame=query();
    frame.hitboxes->input.expand=std::numeric_limits<float>::quiet_NaN();
    CHECK(b.dispatch(frame) && frame.hitboxes->executions==1 &&
          !frame.hitboxes->decision.admitted &&
          !frame.hitboxes->decision.proposed_enable_event);
    // A disabled Hitboxes module no longer supplies an expansion even
    // though Reach and Ghost Hand remain enabled on event8.
    CHECK(b.request(hit,false) && b.apply(true,true));
    frame=query();
    CHECK(b.dispatch(frame) && frame.hitboxes->executions==0 &&
          !frame.hitboxes->decision.proposed_enable_event &&
          frame.reach->executions==1 &&
          frame.ghost_hand->executions==1);
    // Exact metadata rejects changed name/category/event/extra slots.
    ModuleExecutionBridge wrong;
    auto wrongCategory=wrong.add({"Hitboxes",2,{{8,50}},true});
    auto wrongName=wrong.add({"Other",0,{{8,50}},true});
    CHECK(!wrong.bind_hitboxes(wrongCategory));
    CHECK(!wrong.bind_hitboxes(wrongName));
    ModuleExecutionBridge invalid;
    auto wrongSlot=invalid.add({"Hitboxes",0,{{13,0}},true});
    auto extraSlots=invalid.add({"Hitboxes Extra",0,{{8,0},{13,0}},true});
    CHECK(!invalid.bind_hitboxes(wrongSlot));
    CHECK(!invalid.bind_hitboxes(extraSlots));
    b.stop();
    frame=query();
    CHECK(!b.dispatch(frame) && !b.bind_hitboxes(hit));
    std::cout<<"R265_HITBOXES_REACH_GHOST_EVENT8_OFFLINE_PASS\n";
}
