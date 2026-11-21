#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace slinkywinky::reference_compat;
static void ck(bool ok,const char* expr,int line){
    if(!ok)throw std::runtime_error(
        "R256 Ghost Hand line "+std::to_string(line)+": "+expr);
}
#define CHECK(x) ck(!!(x),#x,__LINE__)
static ExecutionFrame frame8(){
    ExecutionFrame f;
    f.event=kReachQueryEvent;f.world_present=f.player_present=true;
    GhostHandQueryPayload q;
    q.input.module_enabled=true;q.input.game_ready=true;
    q.input.mouse_pressed=true;q.input.holding_item=true;
    q.input.item_id=267;q.input.target_is_remote_player=true;
    q.settings.enemies=true;q.settings.allow_using_sword=true;
    f.ghost_hand=q;
    return f;
}
int main(){
    ModuleExecutionBridge b;
    const auto reach=b.add({"Reach",0,{{8,7}},false});
    const auto gh=b.add({"Ghost Hand",0,{{8,0}},false});
    CHECK(b.bind_reach(reach));
    CHECK(b.bind_ghost_hand_from_catalog());
    CHECK(!b.bind_ghost_hand(gh));
    CHECK(b.coverage().implemented_per_event[8]==2);
    CHECK(b.request(reach,true) && b.request(gh,true));
    CHECK(b.apply(true,true));
    auto f=frame8();
    ReachQueryPayload r;
    r.input.sprinting=true;r.settings.distance=3.2f;
    r.settings.activation_ticks=3.0f;
    f.reach=r;
    CHECK(b.dispatch(f));
    CHECK((f.executed_modules==std::vector<std::size_t>{reach,gh}));
    CHECK(f.ghost_hand->executions==1 && f.ghost_hand->query_override);
    CHECK(f.ghost_hand->decision.admitted &&
          f.ghost_hand->decision.can_interact_through);
    CHECK(f.reach->executions==1 && f.reach->proposed_distance.has_value());
    // Event8 query override is independent of canInteractThrough.
    f=frame8();f.ghost_hand->settings.allow_using_sword=false;
    CHECK(b.dispatch(f) && f.ghost_hand->query_override &&
          !f.ghost_hand->decision.can_interact_through);
    f=frame8();f.ghost_hand->input.module_enabled=false;
    CHECK(b.dispatch(f) && f.ghost_hand->query_override &&
          !f.ghost_hand->decision.can_interact_through);
    f=frame8();f.ghost_hand->input.relationship_flags=2;
    CHECK(b.dispatch(f) && f.ghost_hand->query_override &&
          !f.ghost_hand->decision.can_interact_through);
    // A wrong typed event must be rejected before any callback executes.
    f=frame8();f.event=14;CHECK(!b.dispatch(f));
    CHECK(f.ghost_hand->executions==0 && !f.ghost_hand->query_override);
    // A disabled Ghost Hand callback must not set a query flag even if
    // Reach remains enabled and the Ghost Hand payload is present.
    CHECK(b.request(gh,false) && b.apply(true,true));
    f=frame8();f.reach=r;CHECK(b.dispatch(f));
    CHECK(f.ghost_hand->executions==0 && !f.ghost_hand->query_override);
    CHECK(f.reach->executions==1);
    // Reject malformed module category, name or subscription.
    ModuleExecutionBridge wrong;
    const auto badName=wrong.add({"Other",0,{{8,0}},true});
    const auto badCategory=wrong.add({"Ghost Hand",1,{{8,0}},true});
    CHECK(!wrong.bind_ghost_hand(badName));
    CHECK(!wrong.bind_ghost_hand(badCategory));
    ModuleExecutionBridge other;
    const auto wrongSlot=other.add({"Ghost Hand",0,{{13,0}},true});
    CHECK(!other.bind_ghost_hand(wrongSlot));
    ModuleExecutionBridge extra;
    const auto extraSub=extra.add({"Ghost Hand",0,{{8,0},{13,0}},true});
    CHECK(!extra.bind_ghost_hand(extraSub));
    b.stop();f=frame8();CHECK(!b.dispatch(f));
    CHECK(!b.bind_ghost_hand(gh));
    std::cout<<"R256_OFFLINE_GHOST_HAND_EVENT8_SHARED_REACH_PASS\n";
}
