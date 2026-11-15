#include "slinkywinky/module_execution_bridge.hpp"
#include "slinkywinky/source_walk_bank.hpp"
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace slinkywinky::reference_compat;
static void require(bool passed,const char* label,int line) {
    if(!passed)throw std::runtime_error(
        "R233 source walks line "+std::to_string(line)+": "+label);
}
#define CHECK(x) require(!!(x),#x,__LINE__)
static void near(double actual,double expected,double eps=0.00002) {
    if(!std::isfinite(actual)||std::fabs(actual-expected)>eps)
        throw std::runtime_error("R233 expected "+std::to_string(expected)+
                                 ", actual "+std::to_string(actual));
}
static void bank_test() {
    SourceWalkBank<8> timed;
    CHECK(!timed.configure({0,0,0,0}));
    CHECK(!timed.sample_timed(0,1,10,.75f,1.0f));
    CHECK(timed.configure({1,2,3,4}));
    const auto initial=timed.rng_state();
    const auto a=timed.sample_timed(0,1,10,.75f,1.0f);
    const auto b=timed.sample_timed(0,1,10,-0.05f,0.05f);
    CHECK(a && b);
    near(*a,.75);near(*b,-.05);
    CHECK(timed.state(0).t0==2.0);
    CHECK(timed.rng_state()==initial);
    for(int i=0;i<8;++i) {
        CHECK(timed.sample_timed(0,1,10,.75f,1.0f));
    }
    CHECK(timed.state(0).t0==10.0);
    CHECK(timed.sample_timed(0,1,10,.75f,1.0f));
    CHECK(timed.state(0).t0==0.0); // >, never >=
    CHECK(std::fabs(timed.state(0).a-timed.state(0).b)>=.2f);
    const auto after=timed.rng_state();
    CHECK(!timed.sample_timed(0,-1,10,.75f,1.0f));
    CHECK(!timed.sample_timed(9,1,10,.75f,1.0f));
    CHECK(!timed.sample_clock(0,-1,1,0,1));
    CHECK(timed.rng_state()==after);
    SourceWalkBank<8> pair;
    CHECK(pair.configure({1,2,3,4}));
    CHECK(pair.sample_clock(0,1.1,1,0,1));
    const auto w=pair.state(0);
    CHECK(w.t0==1.1 && std::fabs(w.a-w.b)>=.2f);
    CHECK(!pair.sample_clock(0,.9,1,0,1)); // no backwards clock
    CHECK(pair.state(0).t0==1.1);
}
static BridgeAssistTickInput bridge_input() {
    BridgeAssistTickInput in;
    in.in_game=true;in.activation_mode=2;
    in.randomise=true;in.use_source_walk=true;
    in.clock_seconds=0;
    in.on_ground=true;
    BridgeEdgeSample edge;
    edge.next_x=3.25;edge.next_z=3.25;
    edge.margin=.08f;edge.solid[0]=false;
    in.edge=edge;
    return in;
}
static void bridge_test() {
    BridgeAssistController m,n;
    auto in=bridge_input();
    CHECK(!m.on_tick(in).admitted); // no unseeded guesses
    CHECK(!m.configure_source_walk_seed({0,0,0,0}));
    CHECK(m.configure_source_walk_seed({1,2,3,4}));
    CHECK(n.configure_source_walk_seed({1,2,3,4}));
    auto out=m.on_tick(in);
    auto twin=n.on_tick(in);
    CHECK(out.admitted && out.proposed_sneak_pressed==true &&
          out.sampled_edge_reduction && twin.sampled_edge_reduction);
    near(*out.sampled_edge_reduction,0);
    CHECK(!out.sampled_release_delta);
    near(*out.sampled_edge_reduction,*twin.sampled_edge_reduction);
    CHECK(m.source_walks().state(0).t0==0.0);
    CHECK(m.source_walks().state(1).t0==0.0);
    in.effective_sneak_pressed=true;
    out=m.on_tick(in);
    CHECK(out.admitted && !out.sampled_release_delta);
    in.edge->solid[0]=true;
    out=m.on_tick(in);
    CHECK(out.admitted && out.proposed_sneak_pressed==false &&
          out.sampled_edge_reduction && out.sampled_release_delta);
    near(*out.sampled_release_delta,-.5);
    // Original release_timer -= 1 before adding release delay.
    near(out.release_timer,.5,0.0001);
    CHECK(m.source_walks().state(1).t0==0.0);
    CHECK(m.source_walks().state(2).t0==0.0);
    // A later edge check consumes only the source margin state.
    BridgeAssistController later;
    CHECK(later.configure_source_walk_seed({1,2,3,4}));
    in=bridge_input();
    in.clock_seconds=5.1;
    out=later.on_tick(in);
    CHECK(out.admitted && out.sampled_edge_reduction);
    CHECK(later.source_walks().state(0).t0==5.1);
    CHECK(later.source_walks().state(1).t0==0);
    CHECK(std::fabs(later.source_walks().state(0).a-
                    later.source_walks().state(0).b)>=.2f);
    // Original edge_check may sample DIFFERENT world blocks after
    // margin shrinks. Reject if the caller supplied only pre-shrink
    // support observations for a different cell grid.
    const auto previous=later.source_walks().rng_state();
    const auto previous_state=later.source_walks().state(0);
    in.clock_seconds=10.2;
    in.edge->next_x=3.0;
    in.edge->next_z=3.0;
    in.edge->margin=.0001f;
    out=later.on_tick(in);
    CHECK(!out.admitted);
    CHECK(later.source_walks().rng_state()==previous);
    CHECK(later.source_walks().state(0).t0==previous_state.t0);
    // Explicit reset doesn't leave a stuck own-sneak decision.
    later.reset();
    CHECK(!later.owns_sneak());
}
static ClutchLateInput clutch_input() {
    ClutchLateInput c;
    c.in_game=true;c.on_ground=false;c.rotation_priority_granted=true;
    c.has_planned_candidate=true;c.holding_blocks=true;
    c.candidate_block={1,2,3};
    c.aim_yaw_diff=40;c.aim_pitch_diff=20;
    c.aim_speed=15;c.aim_multiplier=.5f;
    c.aim_multiplier_strength=.5f;
    c.use_source_walk=true;c.clock_seconds=0;c.source_randomisation=1;
    return c;
}
static void clutch_test() {
    ClutchController a,b;
    auto input=clutch_input();
    CHECK(!a.on_late(input).admitted);
    CHECK(a.configure_source_walk_seed({1,2,3,4}));
    CHECK(b.configure_source_walk_seed({1,2,3,4}));
    const auto initial=a.source_walks().rng_state();
    const auto side=a.source_candidate_samples(false,0,1);
    const auto twin=b.source_candidate_samples(false,0,1);
    CHECK(side && twin);
    near(side->inset,.1);
    near(side->shift_x,-.01);
    near(side->shift_z,-.01);
    near(side->inset,twin->inset);
    CHECK(a.source_walks().rng_state()==initial);
    CHECK(a.source_walks().state(1).t0==0.0 &&
          a.source_walks().state(2).t0==0.0);
    auto result=a.on_late(input);
    auto other=b.on_late(input);
    CHECK(result.admitted && result.planned &&
          result.source_yaw_scale && result.source_pitch_scale &&
          result.source_yaw_jitter && result.source_pitch_jitter);
    near(*result.source_yaw_scale,.75);
    near(*result.source_pitch_scale,.75);
    near(*result.source_yaw_jitter,-.05);
    near(*result.source_pitch_jitter,-.05);
    near(*result.proposed_yaw_delta,14.625);
    near(*result.proposed_pitch_delta,10.125);
    near(*result.proposed_yaw_delta,*other.proposed_yaw_delta);
    CHECK(a.source_walks().state(6).t0==2.0 &&
          a.source_walks().state(7).t0==2.0);
    // Both timed states advance TWICE per actual aim callback:
    // capped speed, then fractional multiplier jitter.
    input.clock_seconds=0;
    for(int i=0;i<5;++i) {
        CHECK(a.on_late(input).admitted);
    }
    CHECK(a.source_walks().state(6).t0==1.0 &&
          a.source_walks().state(7).t0==1.0);
    CHECK(std::fabs(a.source_walks().state(6).a-
                    a.source_walks().state(6).b)>=.2f);
    // Separate source top-face planner walks #1/#4/#5.
    ClutchController top;
    CHECK(top.configure_source_walk_seed({1,2,3,4}));
    const auto t=top.source_candidate_samples(true,1.1f,1);
    CHECK(t && t->inset>=.1f && t->inset<=.15f);
    CHECK(t->shift_x>=-.1f && t->shift_x<=.1f);
    CHECK(top.source_walks().state(3).t0==1.1f);
    CHECK(top.source_walks().state(4).t0==1.1f);
    // Explicit vertical aim additionally uses source walk #6
    // before the two timed pairs (the original straight-down branch).
    ClutchController straight;
    CHECK(straight.configure_source_walk_seed({1,2,3,4}));
    input=clutch_input();
    input.straight_down_aim=true;
    input.clock_seconds=1.0;
    auto steep=straight.on_late(input);
    CHECK(steep.admitted && steep.source_straight_down_yaw);
    CHECK(straight.source_walks().state(5).t0==1.0);
    CHECK(std::isfinite(*steep.proposed_yaw_delta));
    const auto prior=straight.source_walks().rng_state();
    input.clock_seconds=
        std::numeric_limits<double>::quiet_NaN();
    CHECK(!straight.on_late(input).admitted &&
          straight.source_walks().rng_state()==prior);
}
static void bridge_integration_test() {
    ModuleExecutionBridge bridge;
    const auto bid=bridge.add({"Bridge Assist",3,
        {{5,0},{14,0},{17,0},{18,0}},false});
    const auto cid=bridge.add({"Clutch",3,
        {{0,0},{13,-100},{18,-1000}},false});
    CHECK(bridge.bind_bridge_assist_from_catalog() &&
          bridge.bind_clutch_from_catalog());
    CHECK(bridge.configure_bridge_assist_source_walk_seed({1,2,3,4}));
    CHECK(bridge.configure_clutch_source_walk_seed({1,2,3,4}));
    CHECK(bridge.request(bid,true) && bridge.request(cid,true) &&
          bridge.apply(true,true));
    ExecutionFrame b;
    b.event=kPlayerUpdateEvent;
    b.world_present=b.player_present=true;
    b.bridge_tick=BridgeAssistTickPayload{bridge_input(),{},0};
    CHECK(bridge.dispatch(b) && b.bridge_tick->executions==1 &&
          b.bridge_tick->decision.sampled_edge_reduction);
    ExecutionFrame f;
    f.event=kAntiDebuffEvent;
    f.world_present=f.player_present=true;
    f.clutch_late=ClutchLatePayload{clutch_input(),{},0};
    f.bridge_late=BridgeAssistLatePayload{};
    CHECK(bridge.dispatch(f) && f.clutch_late->executions==1 &&
          f.clutch_late->decision.source_yaw_scale);
    CHECK(!bridge.configure_clutch_source_walk_seed({0,0,0,0}));
    bridge.stop();
    CHECK(!bridge.configure_bridge_assist_source_walk_seed({1,2,3,4}));
}
int main(){
    try {
        bank_test();bridge_test();clutch_test();bridge_integration_test();
        std::cout<<"R233_SOURCE_BRIDGE_CLUTCH_WALKS_PASS "
          "clock=3 timed=2 planner=5 seed=explicit "
          "worldEvidence=callerProvided gameplayWrites=0\n";
        return 0;
    }catch(const std::exception& e) {
        std::cerr<<e.what()<<"\n";return 1;
    }
}
