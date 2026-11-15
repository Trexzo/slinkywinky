#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void require(bool value,const char* desc,int line) {
    if(!value)throw std::runtime_error(
        "R226 FastMine line "+std::to_string(line)+": "+desc);
}
#define CHECK(x) require(!!(x),#x,__LINE__)
static FastMineEarlyInput early() {
    return {true,true,0.0f,8,5.0f,true};
}
static FastMineLateInput late() {
    return {true,true,0.1f,2.0f};
}
static ExecutionFrame frame_early(bool all=false) {
    ExecutionFrame f;
    f.event=kSafeWalkEvent;
    f.world_present=f.player_present=true;
    f.fast_mine_early=FastMineEarlyPayload{early(),{},0};
    if(all) {
        SafeWalkInput edge{};
        edge.in_game=true;
        f.safe_walk=SafeWalkTickPayload{edge,{},0};
        FastPlaceInput place{};
        place.in_game=true;
        f.fast_place=FastPlaceTickPayload{place,{},0};
    }
    return f;
}
static ExecutionFrame frame_late(bool all=false) {
    ExecutionFrame f;
    f.event=kAntiDebuffEvent;
    f.world_present=f.player_present=true;
    f.fast_mine_late=FastMineLatePayload{late(),{},0};
    if(all) {
        NoUseDelayInput use{};
        use.in_game=true;
        f.no_use_delay=NoUseDelayTickPayload{use,{},0};
        f.anti_debuff=AntiDebuffTickPayload{{true,true,true},{},0};
    }
    return f;
}
int main() {
    FastMineController model;
    auto d=model.on_early(early());
    CHECK(d.admitted && d.proposed_block_hit_delay==5);
    CHECK(model.previous_block_hit_delay()==8);
    CHECK(model.previous_damage_progress()==0.0f);
    auto p=model.on_late(late());
    CHECK(p.admitted && p.proposed_damage_progress);
    CHECK(std::fabs(*p.proposed_damage_progress-0.6f)<1e-6f);
    // Early captures raw delay, not clamped: repeated delay==8 can
    // decrement on a later tick only if its configured cap is >=8.
    auto next=early();
    next.max_break_delay=10.0f;
    d=model.on_early(next);
    CHECK(d.admitted && d.proposed_block_hit_delay==7);
    next.block_hit_delay=6;
    d=model.on_early(next);
    CHECK(d.admitted && !d.proposed_block_hit_delay);
    next.block_hit_delay=6;
    CHECK(model.on_early(next).proposed_block_hit_delay==5);
    next.damage_progress=0.2f;
    CHECK(model.on_early(next).admitted);
    CHECK(!model.on_late(late()).proposed_damage_progress);
    next.controller_present=false;
    CHECK(model.on_early(next).admitted);
    CHECK(!model.on_late(late()).proposed_damage_progress);
    model.reset();
    CHECK(!model.on_late(late()).admitted);
    // Source finite-value safety vs unchecked reciprocal/divergent casts.
    next=early();
    next.max_break_delay=std::numeric_limits<float>::infinity();
    CHECK(!model.on_early(next).admitted);
    next=early();
    next.damage_progress=std::numeric_limits<float>::quiet_NaN();
    CHECK(!model.on_early(next).admitted);
    next=early();
    next.in_game=false;
    CHECK(!model.on_early(next).admitted);
    next=early();
    CHECK(model.on_early(next).admitted);
    auto bad=late();
    bad.speed=0.0f;
    CHECK(!model.on_late(bad).admitted);
    bad=late();
    bad.damage_progress=-0.1f;
    CHECK(!model.on_late(bad).admitted);
    bad=late();
    bad.damage_progress=std::numeric_limits<float>::infinity();
    CHECK(!model.on_late(bad).admitted);
    bad=late();
    bad.speed=2.0f;
    CHECK(model.on_late(bad).proposed_damage_progress);
    model.reset();

    ModuleExecutionBridge bridge;
    const auto safe=bridge.add({"Safe Walk",1,{{0,0}},false});
    const auto fastplace=bridge.add({"Fast Place",3,{{0,-1000}},false});
    const auto mine=bridge.add({"Fast Mine",3,{{0,0},{18,0}},false});
    const auto usedelay=bridge.add({"No Use Delay",4,{{18,0}},false});
    const auto anti=bridge.add({"Anti Debuff",2,{{18,0}},false});
    CHECK(bridge.bind_safe_walk_from_catalog());
    CHECK(bridge.bind_fast_place_from_catalog());
    CHECK(bridge.bind_fast_mine_from_catalog());
    CHECK(bridge.bind_no_use_delay_from_catalog());
    CHECK(bridge.bind_anti_debuff_from_catalog());
    CHECK(!bridge.bind_fast_mine(mine));
    const auto coverage=bridge.coverage();
    CHECK(coverage.native_callback_subscriptions==6);
    CHECK(coverage.executable_callback_bindings==6);
    CHECK(coverage.implemented_per_event[0]==3);
    CHECK(coverage.implemented_per_event[18]==3);
    for(auto id:{safe,fastplace,mine,usedelay,anti})
        CHECK(bridge.request(id,true));
    CHECK(!bridge.apply(false,false));
    CHECK(bridge.apply(true,true));
    auto f=frame_early(true);
    CHECK(bridge.dispatch(f));
    CHECK((f.executed_modules==std::vector<std::size_t>{safe,mine,fastplace}));
    CHECK(f.fast_mine_early->executions==1 &&
          f.fast_mine_early->decision.proposed_block_hit_delay==5);
    f=frame_late(true);
    CHECK(bridge.dispatch(f));
    CHECK(f.fast_mine_late->executions==1);
    CHECK(f.fast_mine_late->decision.proposed_damage_progress);
    CHECK(std::fabs(*f.fast_mine_late->decision.proposed_damage_progress-
                    0.6f)<1e-6f);
    CHECK(f.no_use_delay->executions==1 && f.anti_debuff->executions==1);
    f=frame_early(true);
    f.safe_walk->input.in_game=false;
    CHECK(!bridge.dispatch(f));
    f=frame_late(true);
    f.no_use_delay->input.in_game=false;
    CHECK(!bridge.dispatch(f));
    f=frame_early(true);
    f.event=16;
    CHECK(!bridge.dispatch(f));
    f=frame_early(true);
    f.world_present=false;
    CHECK(!bridge.dispatch(f));
    f.world_present=true;f.player_present=false;
    CHECK(!bridge.dispatch(f));
    CHECK(bridge.request(mine,false));
    CHECK(bridge.apply(true,true));
    f=frame_early();
    CHECK(bridge.dispatch(f));
    CHECK(f.fast_mine_early->executions==0);
    CHECK(!f.fast_mine_early->decision.proposed_block_hit_delay);
    bridge.reset_fast_mine_world_state();
    CHECK(bridge.request(mine,true));
    CHECK(bridge.apply(true,true));
    f=frame_late();
    CHECK(bridge.dispatch(f));
    CHECK(!f.fast_mine_late->decision.admitted);
    bridge.stop();
    CHECK(!bridge.dispatch(f));
    for(const auto& wrong:std::vector<ModuleSpec>{
        {"Fast Mine",1,{{0,0},{18,0}},false},
        {"Fast Mine",3,{{0,100},{18,0}},false},
        {"Fast Mine",3,{{0,0},{18,100}},false},
        {"Fast Mine",3,{{0,0}},false},
        {"Fast Mine",3,{{0,0},{18,0},{5,0}},false},
        {"Other",3,{{0,0},{18,0}},false}}) {
        ModuleExecutionBridge invalid;
        const auto id=invalid.add(wrong);
        CHECK(!invalid.bind_fast_mine(id));
        CHECK(!invalid.bind_fast_mine_from_catalog());
    }
    std::cout<<"R226_FAST_MINE_NATIVE_EARLY_AND_LATE_CALLBACKS_PASS "
                "event0=1 event18=1 firstProgress=0.6 "
                "safeWalkFastPlaceShared=1 useDelayAntiDebuffShared=1 "
                "gameplayWrites=0\n";
}
