#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void check(bool yes,const char* why,int line) {
    if(!yes)throw std::runtime_error(
        "R227 Bridge Assist line "+std::to_string(line)+": "+why);
}
#define CHECK(x) check(!!(x),#x,__LINE__)
static BridgeEdgeSample unsupported() {
    BridgeEdgeSample e;
    e.next_x=3.25;e.next_z=3.25;e.margin=0.08f;
    e.solid[0]=false;
    return e;
}
static BridgeAssistTickInput walking() {
    BridgeAssistTickInput in;
    in.in_game=true;in.activation_mode=2;
    in.on_ground=true;in.edge_only_placing=true;
    in.edge=unsupported();
    return in;
}
static BridgeAssistPlaceInput valid_hit() {
    BridgeAssistPlaceInput p;
    p.holding_block=true;p.hit_is_block=true;
    p.on_ground=true;p.hit_face=3;p.back_key=true;p.yaw=45;
    p.player_x=.5;p.player_y=65;p.player_z=.5;
    p.hit_x=.5;p.hit_y=64.5;p.hit_z=.5;
    p.placed_block_x=0;p.placed_block_z=0;
    return p;
}
static void run() {
    CHECK(evaluate_bridge_edge(unsupported()).valid);
    CHECK(evaluate_bridge_edge(unsupported()).unsafe);
    auto e=unsupported();
    e.solid[0]=true;
    CHECK(evaluate_bridge_edge(e).valid && !evaluate_bridge_edge(e).unsafe);
    e=unsupported();e.next_x=4.0;e.next_z=3.25;
    CHECK(!evaluate_bridge_edge(e).valid); // second support corner needed
    e.solid[2]=false;
    CHECK(evaluate_bridge_edge(e).valid &&
          evaluate_bridge_edge(e).unsafe);
    e.solid[2]=true;
    CHECK(evaluate_bridge_edge(e).valid &&
          !evaluate_bridge_edge(e).unsafe);
    e.next_x=std::numeric_limits<double>::quiet_NaN();
    CHECK(!evaluate_bridge_edge(e).valid);

    BridgeAssistController native;
    auto in=walking();
    auto first=native.on_tick(in);
    CHECK(first.admitted && first.owned_sneak &&
          first.proposed_sneak_pressed==true &&
          first.restrict_placing && first.placing);
    const auto late=native.on_late();
    CHECK(late.placing && late.proposed_track_owner_toggle==true);
    in.effective_sneak_pressed=true;
    auto next=native.on_tick(in);
    CHECK(next.admitted && !next.proposed_sneak_pressed &&
          next.restrict_placing && next.owned_sneak);
    auto place=native.on_place(valid_hit());
    CHECK(place.admitted && place.proposed_allow &&
          native.cooldown()==0);
    auto miss=valid_hit();
    miss.hit_face=1;
    place=native.on_place(miss);
    CHECK(place.admitted && !place.proposed_allow &&
          place.cooldown_after==20);
    // Placement cooldown blocks duplicate place hook and gets decremented
    // on source event17's NEXT tick.
    CHECK(!native.on_place(valid_hit()).admitted);
    next=native.on_tick(in);
    CHECK(next.cooldown_left==19);
    auto disable=native.on_disable({true,false,true});
    CHECK(disable.proposed_restore_sneak_pressed==true);
    CHECK(!native.owns_sneak() && !native.placing());
    CHECK(!native.on_late().proposed_track_owner_toggle);
    native.reset();
    in=walking();
    in.only_when_looking_down=true;in.pitch=64.0f;
    next=native.on_tick(in);
    CHECK(next.admitted && !next.owned_sneak &&
          !next.proposed_sneak_pressed);
    in.pitch=65.0f;
    CHECK(native.on_tick(in).proposed_sneak_pressed==true);
    in.effective_sneak_pressed=true;
    in.edge->solid[0]=true;
    next=native.on_tick(in);
    CHECK(next.proposed_sneak_pressed==false && !next.restrict_placing);
    native.reset();
    in=walking();
    in.edge.reset(); // absent world ground evidence fails CLOSED
    CHECK(!native.on_tick(in).admitted && !native.owns_sneak());
    in.edge=unsupported();
    in.edge->margin=std::numeric_limits<float>::quiet_NaN();
    CHECK(!native.on_tick(in).admitted && !native.owns_sneak());
    in.edge=unsupported();
    in.only_few_ticks_after_blocks=true;
    CHECK(native.on_tick(in).admitted && !native.owns_sneak());
    in.holding_block=true;
    CHECK(native.on_tick(in).admitted && native.owns_sneak());
    in.holding_block=false;
    CHECK(native.on_tick(in).held_block_ticks_left==1);
    CHECK(native.on_tick(in).held_block_ticks_left==0);
    next=native.on_tick(in);
    CHECK(!next.owned_sneak && next.proposed_sneak_pressed==false);
    // Original source's relative positioning and diagonal yaw bounds.
    auto hit=valid_hit();
    CHECK(bridge_aimed_at_edge_below(hit));
    hit.yaw=29.0f;CHECK(!bridge_aimed_at_edge_below(hit));
    hit=valid_hit();hit.left_key=true;
    CHECK(!bridge_aimed_at_edge_below(hit));
    hit=valid_hit();hit.hit_y=63.0;
    CHECK(!bridge_aimed_at_edge_below(hit));
    hit=valid_hit();hit.hit_x=.05;hit.hit_z=.05;
    CHECK(!bridge_aimed_at_edge_below(hit));
    hit=valid_hit();hit.hit_x=.5;
    CHECK(bridge_aimed_at_edge_below(hit));

    ModuleExecutionBridge bridge;
    const auto id=bridge.add({"Bridge Assist",3,
        {{5,0},{14,0},{17,0},{18,0}},false});
    CHECK(id==0);
    CHECK(bridge.bind_bridge_assist_from_catalog());
    CHECK(!bridge.bind_bridge_assist(id));
    auto c=bridge.coverage();
    CHECK(c.executable_callback_bindings==4 &&
          c.implemented_per_event[5]==1 &&
          c.implemented_per_event[14]==1 &&
          c.implemented_per_event[17]==1 &&
          c.implemented_per_event[18]==1);
    CHECK(bridge.request(id,true));
    CHECK(!bridge.apply(false,false));
    CHECK(bridge.apply(true,true));
    ExecutionFrame f;
    f.event=17;f.world_present=f.player_present=true;
    f.bridge_tick=BridgeAssistTickPayload{walking(),{},0};
    CHECK(bridge.dispatch(f));
    CHECK(f.bridge_tick->executions==1 &&
          f.bridge_tick->decision.proposed_sneak_pressed==true);
    f=ExecutionFrame{};
    f.event=18;f.world_present=f.player_present=true;
    f.bridge_late=BridgeAssistLatePayload{};
    CHECK(bridge.dispatch(f));
    CHECK(f.bridge_late->executions==1 &&
          f.bridge_late->decision.proposed_track_owner_toggle==true);
    f=ExecutionFrame{};
    f.event=14;f.world_present=f.player_present=true;
    f.bridge_place=BridgeAssistPlacePayload{valid_hit(),{},0};
    CHECK(bridge.dispatch(f) && f.bridge_place->executions==1 &&
          f.bridge_place->decision.proposed_allow);
    f=ExecutionFrame{};
    f.event=5;f.world_present=f.player_present=true;
    f.bridge_disable=BridgeAssistDisablePayload{
        {false,true,true},{},0};
    f.notification_target=id;
    f.notification_new_state=false;
    CHECK(bridge.dispatch(f));
    CHECK(f.bridge_disable->executions==1 &&
          f.bridge_disable->decision.proposed_restore_sneak_pressed==true);
    // After source disable state is explicitly retired.
    f=ExecutionFrame{};
    f.event=18;f.world_present=f.player_present=true;
    f.bridge_late=BridgeAssistLatePayload{};
    CHECK(bridge.dispatch(f) &&
          !f.bridge_late->decision.proposed_track_owner_toggle);
    f=ExecutionFrame{};
    f.event=17;f.world_present=f.player_present=true;
    f.bridge_tick=BridgeAssistTickPayload{walking(),{},0};
    f.notification_target=id;
    CHECK(bridge.dispatch(f));
    f.event=14;CHECK(!bridge.dispatch(f)); // wrong typed event
    f=ExecutionFrame{};
    f.event=17;f.world_present=f.player_present=true;
    f.bridge_tick=BridgeAssistTickPayload{walking(),{},0};
    f.world_present=false;CHECK(!bridge.dispatch(f));
    CHECK(bridge.request(id,false));
    CHECK(bridge.apply(true,true));
    f.world_present=true;
    CHECK(bridge.dispatch(f) && f.bridge_tick->executions==0);
    bridge.reset_bridge_assist_world_state();
    bridge.stop();
    CHECK(!bridge.dispatch(f));
    for(const auto& m:std::vector<ModuleSpec>{
        {"Bridge Assist",1,{{5,0},{14,0},{17,0},{18,0}},false},
        {"Bridge Assist",3,{{5,1},{14,0},{17,0},{18,0}},false},
        {"Bridge Assist",3,{{5,0},{14,10},{17,0},{18,0}},false},
        {"Bridge Assist",3,{{5,0},{14,0},{17,0}},false},
        {"Bridge Assist",3,{{5,0},{14,0},{17,0},{18,0},{0,0}},false},
        {"Other",3,{{5,0},{14,0},{17,0},{18,0}},false}
    }) {
        ModuleExecutionBridge invalid;
        const auto n=invalid.add(m);
        CHECK(!invalid.bind_bridge_assist(n));
    }
    std::cout<<"R227_BRIDGE_ASSIST_NATIVE_SOURCE_4_CALLBACKS_PASS "
                "edge=bounded delay=modeled owner=closed gameWrites=0\n";
}
int main() {
    try {run();return 0;}
    catch(const std::exception& e){
        std::cerr<<e.what()<<"\n";return 1;
    }
}
