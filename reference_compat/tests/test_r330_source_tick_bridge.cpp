#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace slinkywinky::reference_compat;
static void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){
try{
    ModuleExecutionBridge bridge;
    const auto bk=bridge.add({"Backtrack",0,{{2,0},{6,0},{18,0}},false});
    const auto lg=bridge.add({"Lag Range",0,{{3,-100},{6,0},{15,0},{18,0}},false});
    const auto tr=bridge.add({"Timer Range",0,{{0,-100},{1,0},{3,0},{5,0}},false});
    const auto iw=bridge.add({"Inv Walk",1,{{5,0},{17,0},{23,0},{25,0}},false});
    const auto malformed=bridge.add({"Backtrack Wrong",0,{{18,0}},false});
    check(!bridge.bind_backtrack_source_tick(malformed),"deny wrong owner");
    ModuleExecutionBridge wrong_priorities;
    const auto invalid_lag=wrong_priorities.add(
        {"Lag Range",0,{{3,0},{6,0},{15,0},{18,0}},false});
    check(!wrong_priorities.bind_lag_range_source_tick(invalid_lag),
          "reject incorrect original packet callback priority");
    const auto invalid_timer=wrong_priorities.add(
        {"Timer Range",0,{{0,0},{1,0},{3,0},{5,0}},false});
    check(!wrong_priorities.bind_timer_range_source_tick(invalid_timer),
          "reject incorrect original tick callback priority");
    check(!bridge.bind_inv_walk_source_tick(bk),"deny wrong category");
    check(bridge.bind_backtrack_source_tick(bk),"bind backtrack exact row");
    check(bridge.bind_lag_range_source_tick(lg),"bind lag exact row");
    check(bridge.bind_timer_range_source_tick(tr),"bind timer exact row");
    check(bridge.bind_inv_walk_source_tick(iw),"bind inventory exact row");
    check(!bridge.bind_backtrack_source_tick(bk),"deny repeated binding");
    check(bridge.implemented_event_bindings()==4,"four executable tick bindings");
    for(const auto id:{bk,lg,tr,iw})check(bridge.request(id,true),"request enable");
    check(bridge.apply(true,true),"apply enable");
    ExecutionFrame f{};
    f.world_present=f.player_present=true;f.event=18;
    BacktrackSourceTickPayload p{};
    p.settings.delay_ms=100;p.settings.max_range=5;p.settings.render_remote_pos=true;
    p.input.in_game=true;p.input.target=true;p.input.player_ticks=100;
    p.input.last_attack_tick=90;p.input.distance_sq=9;p.input.target_id=19;
    p.input.source_reach_eligible=true;
    f.backtrack_source_tick=p;
    LagRangeSourceTickPayload q{};
    q.input.in_game=true;q.input.target=true;q.input.target_distance_sq=9;
    q.input.sampled_max_range=11;q.input.target_server_position={3,0,0};
    f.lag_range_source_tick=q;
    check(bridge.dispatch(f),"source event18 combined");
    check(f.backtrack_source_tick->executions==1&&f.backtrack_source_tick->active,
          "real backtrack decision callback");
    check(f.lag_range_source_tick->executions==1&&f.lag_range_source_tick->active,
          "real lag range decision callback");
    check((f.executed_modules==std::vector<std::size_t>{bk,lg}),
          "callback order from replay priority");
    auto bad=f;bad.event=17;
    check(!bridge.dispatch(bad),"typed payload event mismatch rejected");
    bad=f;bad.timer_range_source_tick=TimerRangeSourceTickPayload{};
    check(!bridge.dispatch(bad),"mixed event18 and event0 denied");
    check(bridge.request(bk,false),"disable backtrack");
    check(bridge.apply(true,true),"apply disable backtrack");
    check(!bridge.enabled(bk),"backtrack disabled");
    check(bridge.request(bk,true)&&bridge.apply(true,true),"re-enable backtrack");
    f.backtrack_source_tick->input.source_reach_eligible=false;
    check(bridge.dispatch(f)&&!f.backtrack_source_tick->active,
          "disable resets old backtrack target state");

    f={};f.event=0;f.world_present=f.player_present=true;
    TimerRangeSourceTickPayload tp{};
    tp.input.in_game=true;tp.input.target=true;tp.input.target_distance_sq=9;
    tp.input.player_on_ground=true;tp.input.line_of_sight=true;tp.input.forward_input=1;
    f.timer_range_source_tick=tp;
    check(bridge.dispatch(f)&&f.timer_range_source_tick->executions==1,
          "timer source event0 executed");
    f={};f.event=25;f.world_present=f.player_present=true;
    InvWalkSourceTickPayload ip{};
    ip.input.in_game=true;ip.input.screen=InvWalkScreen::inventory;
    ip.input.physically_held={false,false,false,true,false};
    f.inv_walk_source_tick=ip;
    check(bridge.dispatch(f)&&f.inv_walk_source_tick->executions==1 &&
          f.inv_walk_source_tick->decision.update_keys &&
          f.inv_walk_source_tick->decision.pressed[3],"inventory source event25 executed");
    check(bridge.request(iw,false)&&bridge.apply(true,true),"disable inventory");
    check(bridge.request(iw,true)&&bridge.apply(true,true),"re-enable inventory");
    f.inv_walk_source_tick->input.screen=InvWalkScreen::none;
    check(bridge.dispatch(f)&&!f.inv_walk_source_tick->decision.update_keys,
          "disable clears old screen ownership");

    // An exception thrown by later callback must roll back earlier state.
    f={};f.event=18;f.world_present=f.player_present=true;
    f.backtrack_source_tick=p;f.lag_range_source_tick=q;
    f.backtrack_source_tick->input.source_reach_eligible=true;
    f.lag_range_source_tick->input.sampled_max_range=0; // rejected in later callback
    bool threw=false;
    try{bridge.dispatch(f);}catch(const std::invalid_argument&){threw=true;}
    check(threw,"invalid later frame propagates failure");
    f.lag_range_source_tick->input.sampled_max_range=11;
    f.backtrack_source_tick->input.source_reach_eligible=false;
    check(bridge.dispatch(f)&&!f.backtrack_source_tick->active,
          "cross-module exception rollback restored owner state");
    bridge.stop();
    check(!bridge.dispatch(f),"stopped bridge refuses dispatch");
    std::cout<<"R330_SOURCE_TICK_EXECUTION_BRIDGE_PASS\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
