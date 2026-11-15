#include "slinkywinky/source_random_walk.hpp"
#include "slinkywinky/block_in_controller.hpp"
#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace slinkywinky::reference_compat;
static void req(bool value,const char* why,int line) {
    if(!value)throw std::runtime_error(
        "R232 source walk line "+std::to_string(line)+": "+why);
}
#define CHECK(x) req(!!(x),#x,__LINE__)
static void nearly(double actual,double expected,double eps=0.0001) {
    if(!std::isfinite(actual)||std::fabs(actual-expected)>eps)
        throw std::runtime_error("R232 numeric expected "+
            std::to_string(expected)+" got "+std::to_string(actual));
}
static BlockInWorldInput world() {
    BlockInWorldInput w{};
    w.in_game=w.on_ground=w.rotation_priority_granted=
        w.held_block_item=true;
    w.player_x=.5;w.player_y=64.;w.player_z=.5;
    for(auto& r:w.roof_block_ids)r=0u;
    for(unsigned i=0;i<4;++i) {
        w.wall_intersects_player[i]=false;
        w.wall_block_ids[i]={1u,0u,1u,1u};
    }
    BlockInSourceAim source{};
    source.use_reconstructed_walk=true;
    source.eye={.5,63.,.5};
    source.clock_seconds=0.0;
    w.source_aim=source;
    return w;
}
static void rng_tests() {
    SourceWalkRng rng;
    CHECK(!rng.seed({0u,0u,0u,0u}));
    CHECK(rng.seed({1u,2u,3u,4u}));
    // Exact first xoshiro128+ outputs and known post-step state,
    // independently calculated from the source shift/xor/rotate recipe.
    CHECK(rng.next()==5u);
    CHECK((rng.state()==std::array<std::uint32_t,4>{
        7u,0u,1026u,12288u}));
    CHECK(rng.next()==12295u);
    CHECK((rng.state()==std::array<std::uint32_t,4>{
        12295u,1029u,1029u,25165824u}));
    CHECK(rng.next()==25178119u);
    CHECK(rng.next()==27286542u);
    CHECK(rng.next()==39879690u);
    CHECK(rng.next()==1140358681u);
    nearly(rng.unit(),0.7628258466720581,1e-7);
    nearly(rng.unit(),0.9569878578186035,1e-7);
}
static void walk_tests() {
    SourceBlockInWalks w;
    CHECK(!w.configured());
    CHECK(!w.configure({0,0,0,0}));
    CHECK(!w.sample_unit(0,.5,1.0));
    CHECK(w.configure({1,2,3,4}));
    CHECK(w.sample_unit(0,.5,0.0)==0.0f);
    CHECK(w.sample_unit(0,.5,.5)==0.0f); // strict source > boundary
    auto initial=w.state(0);
    CHECK(initial.a==0.0f && initial.b==0.0f);
    const auto first=w.sample_unit(0,.5,1.0);
    CHECK(first && *first>=0 && *first<=1);
    CHECK(w.state(0).t0==1.0);
    CHECK(std::fabs(w.state(0).a-w.state(0).b)>=.2f);
    nearly(*first,w.state(0).a,1e-7);
    auto same=w.sample_unit(0,.5,1.0);
    CHECK(same && *same==*first);
    const auto halfway=w.sample_unit(0,.5,1.25);
    CHECK(halfway);
    nearly(*halfway,
        .5*static_cast<double>(w.state(0).a)+
        .5*static_cast<double>(w.state(0).b));
    const auto boundary=w.sample_unit(0,.5,1.5);
    CHECK(boundary);
    nearly(*boundary,w.state(0).b);
    const auto old_target=w.state(0).b;
    const auto retarget=w.sample_unit(0,.5,1.50001);
    CHECK(retarget);
    nearly(w.state(0).a,old_target);
    CHECK(std::fabs(w.state(0).a-w.state(0).b)>=.2f);
    CHECK(!w.sample_unit(0,.5,1.0)); // clock must not roll back
    CHECK(!w.sample_unit(0,0.0,2.0));
    CHECK(!w.sample_unit(6,.5,2.0));
    CHECK(!w.sample_unit(0,.5,
        std::numeric_limits<double>::quiet_NaN()));
    CHECK(!w.sample_placement(32,2.0));
    CHECK(!w.sample_placement(42,2.0));
    SourceBlockInWalks other;
    CHECK(other.configure({1,2,3,4}));
    CHECK(other.sample_unit(0,.5,0.0)==0.0f);
    CHECK(other.sample_unit(0,.5,.5)==0.0f);
    CHECK(other.sample_unit(0,.5,1.0)==*first);
    CHECK(other.sample_unit(0,.5,1.25)==*halfway);
    CHECK(other.sample_unit(0,.5,1.5)==*boundary);
}
static void integration() {
    BlockInController a,b;
    CHECK(!a.configure_source_walk_seed({0,0,0,0}));
    CHECK(a.configure_source_walk_seed({1,2,3,4}));
    CHECK(b.configure_source_walk_seed({1,2,3,4}));
    auto w=world();
    auto first=a.on_tick(w);
    auto clone=b.on_tick(w);
    CHECK(first.admitted && first.proposed_click &&
          first.proposed_placement_bit==0u);
    CHECK(first.proposed_walk_first==0.0f &&
          first.proposed_walk_second==0.0f);
    CHECK(first.proposed_jitter_yaw==.8f &&
          first.proposed_jitter_pitch==.8f);
    CHECK(first.proposed_world_target && clone.proposed_world_target);
    // At zero initial random-walk state, the source's foot-level
    // [-0.25,-0.4] range starts at -0.25: z = 1.5-(-0.25)=1.75.
    nearly(first.proposed_world_target->z,1.75,0.00001);
    nearly(first.proposed_world_target->z,
           clone.proposed_world_target->z);
    CHECK(a.source_walks().state(2).a==0.0f);
    CHECK(a.source_walks().state(4).a==0.0f);
    w.source_aim->clock_seconds=1.0;
    // The first two world walks (third/fourth source states) and
    // both yaw/pitch jitter walks now retarget using shared xoshiro.
    first=a.on_tick(w);
    clone=b.on_tick(w);
    CHECK(first.admitted && clone.admitted);
    CHECK(first.proposed_walk_first && first.proposed_walk_second);
    CHECK(first.proposed_jitter_yaw && first.proposed_jitter_pitch);
    nearly(*first.proposed_walk_first,*clone.proposed_walk_first,1e-7);
    nearly(*first.proposed_walk_second,*clone.proposed_walk_second,1e-7);
    nearly(*first.proposed_jitter_yaw,*clone.proposed_jitter_yaw,1e-7);
    nearly(*first.proposed_jitter_pitch,*clone.proposed_jitter_pitch,1e-7);
    CHECK(std::fabs(a.source_walks().state(2).a-
                    a.source_walks().state(2).b)>=.2f);
    CHECK(std::fabs(a.source_walks().state(4).a-
                    a.source_walks().state(4).b)>=.2f);

    // Source's roof walks have an independent pair of states but
    // share the global PRNG. No external clock injection beyond input.
    a.reset();b.reset();
    auto roof=world();
    roof.roof_block_ids[1]=1u;
    roof.source_aim->clock_seconds=1.1;
    roof.source_aim->eye={0.,63.,-.2};
    auto r=a.on_tick(roof);
    auto r2=b.on_tick(roof);
    CHECK(r.proposed_placement_bit==33u);
    CHECK(r.proposed_world_target && r2.proposed_world_target);
    nearly(r.proposed_world_target->y,r2.proposed_world_target->y);
    CHECK(a.source_walks().state(0).t0==1.1);
    CHECK(a.source_walks().state(1).t0==1.1);

    // Negative world input is transactional for both the placement
    // mask and all six walk states/shared xoshiro words.
    BlockInController guard;
    CHECK(guard.configure_source_walk_seed({1,2,3,4}));
    auto valid=world();
    CHECK(guard.on_tick(valid).admitted);
    const auto before_rng=guard.source_walks().rng_state();
    const auto before_mask=guard.placed_mask();
    auto broken=valid;
    broken.source_aim->clock_seconds=2.0;
    broken.player_y+=.05;
    broken.roof_block_ids[1].reset();
    CHECK(!guard.on_tick(broken).admitted);
    CHECK(guard.source_walks().rng_state()==before_rng);
    CHECK(guard.placed_mask()==before_mask);
    broken=valid;
    broken.source_aim->clock_seconds=
        std::numeric_limits<double>::infinity();
    CHECK(!guard.on_tick(broken).admitted);
    CHECK(guard.source_walks().rng_state()==before_rng);
    // A seeded walker without the typed source-aim opt-in never
    // consumes randomness or changes the legacy R230 API.
    BlockInController legacy;
    CHECK(legacy.configure_source_walk_seed({1,2,3,4}));
    const auto pre=legacy.source_walks().rng_state();
    w=world();
    w.source_aim.reset();
    auto classic=legacy.on_tick(w);
    CHECK(classic.admitted && !classic.proposed_walk_first &&
          legacy.source_walks().rng_state()==pre);
    // Bridge dispatch uses the same opt-in source-walk path.
    ModuleExecutionBridge bridge;
    const auto id=bridge.add({"Block In",3,
        {{0,0},{5,0},{13,-2000},{15,-1000},{16,-500}},false});
    CHECK(bridge.bind_block_in_from_catalog());
    CHECK(bridge.request(id,true) && bridge.apply(true,true));
    // The bridge's owner has an explicit deterministic seed API.
    CHECK(bridge.configure_block_in_source_walk_seed({1,2,3,4}));
    ExecutionFrame f{};
    f.event=kMovementTickEvent;
    f.world_present=f.player_present=true;
    f.block_in_tick=BlockInTickPayload{world(),{},0};
    CHECK(bridge.dispatch(f) && f.block_in_tick->executions==1 &&
          f.block_in_tick->decision.proposed_jitter_yaw);
    bridge.stop();
}
int main() {
    try{
        rng_tests();walk_tests();integration();
        std::cout<<"R232_XOSHIRO128_SOURCE_WALK_AND_BLOCK_IN_PIPELINE_PASS "
          "seed=explicit sixWalks=isolated periods=0.5,0.2 "
          "rollback=verified gameWrites=0\n";
        return 0;
    }catch(const std::exception& e) {
        std::cerr<<e.what()<<"\n";return 1;
    }
}
