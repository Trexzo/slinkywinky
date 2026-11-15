#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;
static void require(bool ok,const char* label,int line) {
    if(!ok)throw std::runtime_error(
        "R225 FastPlace "+std::to_string(line)+": "+label);
}
#define CHECK(x) require(!!(x),#x,__LINE__)
static FastPlaceInput sample() {
    FastPlaceInput a;
    a.in_game=true;
    a.use_key_held=true;
    a.block_item_held=true;
    a.target_is_block=true;
    a.max_cps=20.0f;
    return a;
}
static ExecutionFrame tick(const FastPlaceInput& in,
                           bool with_safe=false) {
    ExecutionFrame f;
    f.event=kSafeWalkEvent;
    f.world_present=true;
    f.player_present=true;
    f.fast_place=FastPlaceTickPayload{in,{},0};
    if(with_safe) {
        SafeWalkInput edge;
        edge.in_game=in.in_game;
        edge.on_ground=true;
        edge.holding_blocks=true;
        f.safe_walk=SafeWalkTickPayload{edge,{},0};
    }
    return f;
}
int main() {
    FastPlaceController model;
    auto in=sample();
    auto out=model.evaluate(in);
    CHECK(out.admitted && out.extra_use_press_count==0);
    CHECK(model.was_held() && model.timer()==1.0f);
    out=model.evaluate(in);
    CHECK(out.extra_use_press_count==1 && model.timer()==1.0f);
    // Pending vanilla right-click blocks extra proposed use presses.
    in.use_press_already_pending=true;
    out=model.evaluate(in);
    CHECK(out.admitted && out.extra_use_press_count==0);
    in.use_press_already_pending=false;
    out=model.evaluate(in);
    CHECK(out.extra_use_press_count==1);
    // A held block overlapping the player is not placeable.
    in.adjacent_block_overlaps_player=true;
    out=model.evaluate(in);
    CHECK(out.extra_use_press_count==0);
    in.random_bypass_granted=true;
    out=model.evaluate(in);
    CHECK(out.extra_use_press_count==1);
    in.random_bypass_granted=false;
    in.item_is_ladder=true;
    out=model.evaluate(in);
    CHECK(out.extra_use_press_count==1);
    in.item_is_ladder=false;
    in.target_is_block=false;
    out=model.evaluate(in);
    CHECK(out.extra_use_press_count==0);
    in.target_is_block=true;
    in.block_item_held=false;
    out=model.evaluate(in);
    CHECK(out.extra_use_press_count==0);

    // The original tick updates activation delay only when it observes
    // a release; no extra use click is fabricated on the first press.
    model.reset();
    in=sample();
    in.activation_delay_ticks=2.0f;
    in.use_key_held=false;
    CHECK(model.evaluate(in).extra_use_press_count==0);
    CHECK(model.activation_left()==2);
    in.use_key_held=true;
    CHECK(model.evaluate(in).extra_use_press_count==0 &&
          model.activation_left()==1);
    CHECK(model.evaluate(in).extra_use_press_count==0 &&
          model.activation_left()==0);
    CHECK(model.evaluate(in).extra_use_press_count==1);
    in.use_key_held=false;
    model.evaluate(in);
    CHECK(model.activation_left()==2);

    model.reset();
    in=sample();
    in.max_cps=80.0f;
    auto first=model.evaluate(in);
    auto second=model.evaluate(in);
    CHECK(first.extra_use_press_count==0);
    CHECK(second.extra_use_press_count>=3 &&
          second.extra_use_press_count<=6);
    CHECK(model.timer()>0.0f);
    model.reset();
    in=sample();
    in.max_cps=std::numeric_limits<float>::quiet_NaN();
    CHECK(!model.evaluate(in).admitted && !model.was_held());
    in.max_cps=0.0f;
    CHECK(!model.evaluate(in).admitted);
    in.max_cps=101.0f;
    CHECK(!model.evaluate(in).admitted);
    in.max_cps=10.0f;
    in.activation_delay_ticks=-1.0f;
    CHECK(!model.evaluate(in).admitted);
    in.activation_delay_ticks=201.0f;
    CHECK(!model.evaluate(in).admitted);
    in.activation_delay_ticks=0.0f;
    CHECK(model.evaluate(in).admitted);
    model.reset();
    in.in_game=false;
    CHECK(model.evaluate(in).extra_use_press_count==0 &&
          !model.was_held());

    ModuleExecutionBridge bridge;
    const auto safe=bridge.add({"Safe Walk",1,{{0,0}},false});
    const auto fast=bridge.add({"Fast Place",3,{{0,-1000}},false});
    const auto early=bridge.add({"Source Observer",2,{{0,100}},true});
    const auto unrelated=bridge.add({"Reach",0,{{8,0}},false});
    CHECK(unrelated==3u);
    unsigned seen=0;
    CHECK(bridge.bind(early,kSafeWalkEvent,
          [&](const Delivery& d,ExecutionFrame& frame){
              CHECK(d.recipient==early && frame.event==0);
              ++seen;
          }));
    CHECK(bridge.bind_safe_walk_from_catalog());
    CHECK(bridge.bind_fast_place_from_catalog());
    CHECK(!bridge.bind_fast_place(fast));
    CHECK(bridge.bind_reach_from_catalog());
    const auto c=bridge.coverage();
    CHECK(c.native_callback_subscriptions==4);
    CHECK(c.executable_callback_bindings==4);
    CHECK(c.modules_with_executable_bindings==4);
    CHECK(c.implemented_per_event[0]==3);
    CHECK(bridge.request(safe,true));
    CHECK(bridge.request(fast,true));
    CHECK(bridge.apply(true,true));

    auto f=tick(sample(),true);
    CHECK(bridge.dispatch(f));
    CHECK((f.executed_modules==std::vector<std::size_t>{early,safe,fast}));
    CHECK(f.fast_place->executions==1);
    CHECK(f.fast_place->decision.extra_use_press_count==0);
    CHECK(f.safe_walk->executions==1);
    f=tick(sample(),true);
    CHECK(bridge.dispatch(f));
    CHECK(f.fast_place->decision.extra_use_press_count==1);
    CHECK(seen==2);
    // Two event0 view payloads must agree on in-game state.
    f=tick(sample(),true);
    f.safe_walk->input.in_game=false;
    CHECK(!bridge.dispatch(f));
    f=tick(sample(),true);
    f.event=16;
    CHECK(!bridge.dispatch(f));
    f=tick(sample(),true);
    f.world_present=false;
    CHECK(!bridge.dispatch(f));
    f=tick(sample(),true);
    f.player_present=false;
    CHECK(!bridge.dispatch(f));
    // Disabled Fast Place must not retain an executable decision.
    CHECK(bridge.request(fast,false));
    CHECK(bridge.apply(true,true));
    f=tick(sample(),true);
    CHECK(bridge.dispatch(f));
    CHECK(f.fast_place->executions==0 &&
          f.fast_place->decision.extra_use_press_count==0);
    CHECK((f.executed_modules==std::vector<std::size_t>{early,safe}));
    bridge.reset_fast_place_world_state();
    CHECK(bridge.request(fast,true));
    CHECK(bridge.apply(true,true));
    f=tick(sample());
    CHECK(bridge.dispatch(f));
    CHECK(f.fast_place->executions==1 &&
          f.fast_place->decision.extra_use_press_count==0);
    bridge.stop();
    CHECK(!bridge.dispatch(f) && !bridge.bind_fast_place_from_catalog());
    CHECK(!bridge.request(fast,true));

    for(const auto& wrong : std::vector<ModuleSpec>{
          {"Fast Place",1,{{0,-1000}},false},
          {"Fast Place",3,{{0,0}},false},
          {"Fast Place",3,{{1,-1000}},false},
          {"Fast Place",3,{{0,-1000},{5,0}},false},
          {"Other",3,{{0,-1000}},false}}) {
        ModuleExecutionBridge bad;
        const auto id=bad.add(wrong);
        CHECK(!bad.bind_fast_place(id));
        CHECK(!bad.bind_fast_place_from_catalog());
    }
    std::cout<<"R225_FAST_PLACE_NATIVE_EVENT0_FIRSTPARTY_CALLBACK_PASS "
                "category=3 priority=-1000 overlap=refused "
                "ladder=admitted firstPress=notDuplicated "
                "sourceSafeWalkPriority=0 independentGameWrites=0\n";
}
