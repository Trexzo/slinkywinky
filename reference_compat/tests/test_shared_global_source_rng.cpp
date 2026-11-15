#include "slinkywinky/module_execution_bridge.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void req(bool ok,const char* name,int line) {
    if(!ok)throw std::runtime_error(
        "R234 global RNG line "+std::to_string(line)+": "+name);
}
#define CHECK(x) req(!!(x),#x,__LINE__)
static constexpr std::array<std::uint32_t,4> kSeed{{1,2,3,4}};
static constexpr std::array<std::uint32_t,4> kOtherSeed{{5,6,7,8}};
static BridgeAssistTickInput bridge_input(double now) {
    BridgeAssistTickInput v{};
    v.in_game=true;
    v.activation_mode=2;
    v.randomise=true;
    v.use_source_walk=true;
    v.clock_seconds=now;
    v.on_ground=true;
    BridgeEdgeSample e{};
    e.next_x=3.25;
    e.next_z=3.25;
    e.margin=.08f;
    e.solid[0]=false;
    v.edge=e;
    return v;
}
static ClutchLateInput clutch_input(double now) {
    ClutchLateInput v{};
    v.in_game=true;
    v.rotation_priority_granted=true;
    v.has_planned_candidate=true;
    v.holding_blocks=true;
    v.candidate_block={1,2,3};
    v.aim_yaw_diff=40;
    v.aim_pitch_diff=20;
    v.aim_speed=15;
    v.aim_multiplier=.5f;
    v.aim_multiplier_strength=.5f;
    v.use_source_walk=true;
    v.clock_seconds=now;
    v.source_randomisation=1;
    v.source_sample_candidate=true;
    v.source_candidate_top=false;
    return v;
}
static BlockInWorldInput block_input(double now) {
    BlockInWorldInput w{};
    w.in_game=w.on_ground=w.rotation_priority_granted=
        w.held_block_item=true;
    w.player_x=.5;w.player_y=64.;w.player_z=.5;
    for(auto& cell:w.roof_block_ids)cell=0u;
    for(unsigned i=0;i<4;++i) {
        w.wall_intersects_player[i]=false;
        w.wall_block_ids[i]={1u,0u,1u,1u};
    }
    BlockInSourceAim aim{};
    aim.use_reconstructed_walk=true;
    aim.clock_seconds=now;
    aim.eye={.5,63.,.5};
    w.source_aim=aim;
    return w;
}
struct IDs {std::size_t bridge,clutch,block;};
static IDs prepare(ModuleExecutionBridge& exec) {
    const auto bridge=exec.add({"Bridge Assist",3,
        {{5,0},{14,0},{17,0},{18,0}},false});
    const auto clutch=exec.add({"Clutch",3,
        {{0,0},{13,-100},{18,-1000}},false});
    const auto block=exec.add({"Block In",3,
        {{0,0},{5,0},{13,-2000},{15,-1000},{16,-500}},false});
    CHECK(exec.bind_bridge_assist_from_catalog());
    CHECK(exec.bind_clutch_from_catalog());
    CHECK(exec.bind_block_in_from_catalog());
    CHECK(exec.request(bridge,true));
    CHECK(exec.request(clutch,true));
    CHECK(exec.request(block,true));
    CHECK(exec.apply(true,true));
    return {bridge,clutch,block};
}
static BridgeAssistTickDecision deliver_bridge(
    ModuleExecutionBridge& m,double now) {
    ExecutionFrame f{};
    f.event=kPlayerUpdateEvent;
    f.world_present=f.player_present=true;
    f.bridge_tick=BridgeAssistTickPayload{bridge_input(now),{},0};
    CHECK(m.dispatch(f));
    CHECK(f.bridge_tick->executions==1);
    return f.bridge_tick->decision;
}
static ClutchLateDecision deliver_clutch(
    ModuleExecutionBridge& m,double now) {
    ExecutionFrame f{};
    f.event=kAntiDebuffEvent;
    f.world_present=f.player_present=true;
    f.clutch_late=ClutchLatePayload{clutch_input(now),{},0};
    CHECK(m.dispatch(f));
    CHECK(f.clutch_late->executions==1);
    return f.clutch_late->decision;
}
static BlockInTickDecision deliver_block(
    ModuleExecutionBridge& m,double now) {
    ExecutionFrame f{};
    f.event=kMovementTickEvent;
    f.world_present=f.player_present=true;
    f.block_in_tick=BlockInTickPayload{block_input(now),{},0};
    CHECK(m.dispatch(f));
    CHECK(f.block_in_tick->executions==1);
    return f.block_in_tick->decision;
}
static void global_owner_tests() {
    ModuleExecutionBridge shared;
    CHECK(!shared.has_shared_source_walk());
    CHECK(!shared.shared_source_rng_state());
    prepare(shared);
    CHECK(!shared.configure_shared_source_walk_seed({0,0,0,0}));
    CHECK(!shared.has_shared_source_walk());
    CHECK(shared.configure_shared_source_walk_seed(kSeed));
    CHECK(shared.has_shared_source_walk());
    CHECK(*shared.shared_source_rng_state()==kSeed);
    CHECK(!shared.configure_shared_source_walk_seed(kOtherSeed));
    // A single owner prevents individual module seeds from silently
    // diverging after ownership was established.
    CHECK(!shared.configure_bridge_assist_source_walk_seed(kOtherSeed));
    CHECK(!shared.configure_clutch_source_walk_seed(kOtherSeed));
    CHECK(!shared.configure_block_in_source_walk_seed(kOtherSeed));
    CHECK(*shared.shared_source_rng_state()==kSeed);

    // Independent direct-model oracle: explicitly pass the same
    // global generator to each module in source event delivery order.
    BridgeAssistController oracle_bridge;
    ClutchController oracle_clutch;
    BlockInController oracle_block;
    SourceWalkRng oracle_rng;
    CHECK(oracle_rng.seed(kSeed));
    CHECK(oracle_bridge.configure_source_walk_seed(kSeed));
    CHECK(oracle_clutch.configure_source_walk_seed(kSeed));
    CHECK(oracle_block.configure_source_walk_seed(kSeed));

    const auto b=deliver_bridge(shared,11.);
    CHECK(b.admitted && b.sampled_edge_reduction);
    CHECK(oracle_bridge.adopt_source_rng(oracle_rng));
    const auto b_expected=oracle_bridge.on_tick(bridge_input(11.));
    CHECK(b_expected.admitted && b_expected.sampled_edge_reduction);
    oracle_rng=oracle_bridge.source_rng_snapshot();
    CHECK(*shared.shared_source_rng_state()==oracle_rng.state());
    CHECK(*b.sampled_edge_reduction==*b_expected.sampled_edge_reduction);
    CHECK(*shared.shared_source_rng_state()!=kSeed);

    const auto c=deliver_clutch(shared,11.);
    CHECK(c.admitted && c.source_candidate_inset &&
          c.source_yaw_scale && c.source_yaw_jitter);
    CHECK(oracle_clutch.adopt_source_rng(oracle_rng));
    const auto c_expected=oracle_clutch.on_late(clutch_input(11.));
    CHECK(c_expected.admitted && c_expected.source_candidate_inset &&
          c_expected.source_yaw_scale);
    oracle_rng=oracle_clutch.source_rng_snapshot();
    CHECK(*shared.shared_source_rng_state()==oracle_rng.state());
    CHECK(*c.source_candidate_inset==*c_expected.source_candidate_inset);
    CHECK(*c.source_yaw_scale==*c_expected.source_yaw_scale);
    CHECK(*c.proposed_yaw_delta==*c_expected.proposed_yaw_delta);

    const auto block=deliver_block(shared,11.);
    CHECK(block.admitted && block.proposed_walk_first &&
          block.proposed_jitter_yaw);
    CHECK(oracle_block.adopt_source_rng(oracle_rng));
    const auto block_expected=oracle_block.on_tick(block_input(11.));
    CHECK(block_expected.admitted && block_expected.proposed_walk_first);
    oracle_rng=oracle_block.source_rng_snapshot();
    CHECK(*shared.shared_source_rng_state()==oracle_rng.state());
    CHECK(*block.proposed_walk_first==*block_expected.proposed_walk_first);
    CHECK(*block.proposed_jitter_yaw==*block_expected.proposed_jitter_yaw);
    CHECK(block.proposed_world_target->z==
          block_expected.proposed_world_target->z);

    // A different source delivery order consumes the SAME generator
    // in a different sequence: the output traces must differ.
    ModuleExecutionBridge swapped;
    prepare(swapped);
    CHECK(swapped.configure_shared_source_walk_seed(kSeed));
    auto block_first=deliver_block(swapped,11.);
    auto bridge_second=deliver_bridge(swapped,11.);
    auto clutch_last=deliver_clutch(swapped,11.);
    CHECK(block_first.admitted && bridge_second.admitted &&
          clutch_last.admitted);
    CHECK(*swapped.shared_source_rng_state()!=
          *shared.shared_source_rng_state() ||
          *block_first.proposed_walk_first!=
          *block.proposed_walk_first);
    shared.stop();
    CHECK(!shared.has_shared_source_walk());
    CHECK(!shared.shared_source_rng_state());
    CHECK(!shared.configure_shared_source_walk_seed(kSeed));
    swapped.stop();
}
static void rollback_and_legacy_tests() {
    ModuleExecutionBridge m;
    const auto ids=prepare(m);
    CHECK(m.configure_shared_source_walk_seed(kSeed));
    const auto start=*m.shared_source_rng_state();
    auto bad=block_input(10.0);
    bad.roof_block_ids[1].reset();
    ExecutionFrame denied{};
    denied.event=kMovementTickEvent;
    denied.world_present=denied.player_present=true;
    denied.block_in_tick=BlockInTickPayload{bad,{},0};
    // The event is delivered, but the typed module rejects corrupt
    // world data and must not commit its random draws.
    CHECK(m.dispatch(denied));
    CHECK(!denied.block_in_tick->decision.admitted);
    CHECK(*m.shared_source_rng_state()==start);
    auto invalid=ExecutionFrame{};
    invalid.event=kMovementTickEvent;
    invalid.world_present=false;
    invalid.player_present=true;
    invalid.block_in_tick=BlockInTickPayload{block_input(10.0),{},0};
    CHECK(!m.dispatch(invalid));
    CHECK(*m.shared_source_rng_state()==start);
    const auto valid=deliver_block(m,10.0);
    CHECK(valid.admitted);
    const auto progress=*m.shared_source_rng_state();
    CHECK(progress!=start);
    // A second independent session at the same seed and valid input
    // must yield the same source trace: refused events did not steal
    // random draws or corrupt one module's walk timers.
    ModuleExecutionBridge clean;
    prepare(clean);
    CHECK(clean.configure_shared_source_walk_seed(kSeed));
    const auto expected=deliver_block(clean,10.0);
    CHECK(valid.proposed_walk_first==expected.proposed_walk_first);
    CHECK(valid.proposed_jitter_yaw==expected.proposed_jitter_yaw);
    CHECK(*clean.shared_source_rng_state()==progress);

    // An unrelated module can emit a callback with no random draw
    // without mutating the global generator.
    const auto before=progress;
    ExecutionFrame frame{};
    frame.event=kClutchRotationFlagEvent;
    frame.world_present=frame.player_present=true;
    frame.clutch_rotation=ClutchRotationPayload{};
    frame.block_in_rotation=BlockInRotationPayload{};
    CHECK(m.dispatch(frame));
    CHECK(*m.shared_source_rng_state()==before);

    // A disabled modeled module never receives its source late-phase
    // callback, therefore cannot consume another module's PRNG stream.
    CHECK(m.request(ids.clutch,false));
    CHECK(m.apply(true,true));
    ExecutionFrame disabled{};
    disabled.event=kAntiDebuffEvent;
    disabled.world_present=disabled.player_present=true;
    disabled.clutch_late=ClutchLatePayload{clutch_input(11.0),{},0};
    CHECK(m.dispatch(disabled));
    CHECK(disabled.clutch_late->executions==0);
    CHECK(*m.shared_source_rng_state()==before);

    // The local per-module seed API remains fully backward compatible
    // when no shared owner is active.
    ModuleExecutionBridge legacy;
    prepare(legacy);
    CHECK(legacy.configure_block_in_source_walk_seed(kSeed));
    CHECK(legacy.configure_clutch_source_walk_seed(kSeed));
    CHECK(legacy.configure_bridge_assist_source_walk_seed(kSeed));
    CHECK(!legacy.has_shared_source_walk());
    CHECK(!legacy.shared_source_rng_state());
    CHECK(deliver_clutch(legacy,11.).admitted);
    CHECK(deliver_bridge(legacy,11.).admitted);
    CHECK(deliver_block(legacy,11.).admitted);
    legacy.stop();
    m.stop();
    clean.stop();
}
static void exception_rollback_test() {
    ModuleExecutionBridge fault;
    const auto edge=fault.add({"Bridge Assist",3,
        {{5,0},{14,0},{17,0},{18,0}},false});
    const auto bomb=fault.add({"R234 Fail After Draw",3,
        {{kPlayerUpdateEvent,-1000}},false});
    CHECK(fault.bind_bridge_assist_from_catalog());
    CHECK(fault.bind(bomb,kPlayerUpdateEvent,
        [](const Delivery&,ExecutionFrame&){
            throw std::runtime_error("R234 injected post-draw exception");
        }));
    CHECK(fault.request(edge,true) && fault.request(bomb,true));
    CHECK(fault.apply(true,true));
    CHECK(fault.configure_shared_source_walk_seed(kSeed));
    ExecutionFrame rejected{};
    rejected.event=kPlayerUpdateEvent;
    rejected.world_present=rejected.player_present=true;
    rejected.bridge_tick=BridgeAssistTickPayload{bridge_input(11.0),{},0};
    bool caught=false;
    try {
        (void)fault.dispatch(rejected);
    } catch(const std::runtime_error& e) {
        caught=std::string(e.what())==
               "R234 injected post-draw exception";
    }
    CHECK(caught);
    CHECK(rejected.bridge_tick->executions==1);
    // Even when Bridge Assist generated the first proposal before the
    // following callback threw, global PRNG and per-module walk
    // clocks were BOTH restored by the bridge frame transaction.
    CHECK(*fault.shared_source_rng_state()==kSeed);
    CHECK(fault.request(bomb,false));
    CHECK(fault.apply(true,true));
    const auto replay=deliver_bridge(fault,11.0);
    CHECK(replay.admitted && replay.sampled_edge_reduction);
    ModuleExecutionBridge baseline;
    const auto baseline_edge=baseline.add({"Bridge Assist",3,
        {{5,0},{14,0},{17,0},{18,0}},false});
    CHECK(baseline.bind_bridge_assist_from_catalog());
    CHECK(baseline.request(baseline_edge,true));
    CHECK(baseline.apply(true,true));
    CHECK(baseline.configure_shared_source_walk_seed(kSeed));
    const auto expected=deliver_bridge(baseline,11.0);
    CHECK(replay.sampled_edge_reduction==expected.sampled_edge_reduction);
    CHECK(*fault.shared_source_rng_state()==
          *baseline.shared_source_rng_state());
    fault.stop();
    baseline.stop();
}
int main(){
    try{
        global_owner_tests();
        rollback_and_legacy_tests();
        exception_rollback_test();
        std::cout<<"R234_ONE_GLOBAL_XOSHIRO_SOURCE_DISPATCH_PASS "
            "modules=3 interleaving=source_order seeded=explicit "
            "rollback=frame_atomic legacy=isolation gameWrites=0\n";
        return 0;
    } catch(const std::exception& e){
        std::cerr<<e.what()<<"\n";
        return 1;
    }
}
