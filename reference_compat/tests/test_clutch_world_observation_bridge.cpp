#include "slinkywinky/clutch_observation_bridge.hpp"
#include <array>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <string>
using namespace slinkywinky::reference_compat;

static void check(bool okay,const char* what,int line) {
    if(!okay)throw std::runtime_error("R238 line "+
        std::to_string(line)+": "+what);
}
#define CHECK(x) check(!!(x),#x,__LINE__)

static ClutchObservedFrame frame() {
    ClutchObservedFrame f{};
    f.generation=2;f.tick=1;f.world_token=11;f.player_token=12;
    f.box_lo={.2,64,.2};f.box_hi={.8,65.8,.8};
    f.eye={.5,65.5,.5};
    f.base_x=0;f.base_y=63;f.base_z=0;
    f.range_index=1;
    f.current_yaw=-90;f.current_pitch=70;
    f.max_aim_degrees=300;
    f.clock_seconds=1.1;
    f.candidate_block_ids={0,0,0,0,0,5};
    return f;
}
static void real_source_planner_intake() {
    ClutchObservationOwner session;
    auto f=frame();
    const auto w=session.consume(f);
    CHECK(w && !session.closed());
    CHECK(w->range_index==1 && w->source_solid[5] &&
          !*w->source_solid[0] && !w->source_solid[6]);
    SourceWalkBank<8> rng;
    CHECK(rng.configure({1,2,3,4}));
    auto plan=clutch_plan_original(*w,rng);
    CHECK(plan.admitted && plan.found && !plan.face_up &&
          plan.source_index==5 && plan.visited==6 &&
          (plan.selected==ClutchCandidatePos{1,63,0}));

    // Source ordering cannot depend on object lifetime or caller writes.
    f.candidate_block_ids[5]=0;
    CHECK(*w->source_solid[5]);
    f.tick=2;
    CHECK(session.consume(f));
    CHECK(!session.closed());
    f.tick=2;
    CHECK(!session.consume(f) && session.closed());
    f.tick=3;
    CHECK(!session.consume(f)); // quarantine is irreversible
}
static void strict_negative_controls() {
    {
        ClutchObservationOwner own;auto f=frame();
        f.base_y=64;CHECK(!own.consume(f) && own.closed());
    }
    {
        ClutchObservationOwner own;auto f=frame();
        f.candidate_block_ids.pop_back();CHECK(!own.consume(f));
    }
    {
        ClutchObservationOwner own;auto f=frame();
        f.candidate_block_ids[3]=4096;CHECK(!own.consume(f));
    }
    {
        ClutchObservationOwner own;auto f=frame();
        f.ground_block_ids[0]=0;CHECK(!own.consume(f));
    }
    {
        ClutchObservationOwner own;auto f=frame();
        f.generation=0;CHECK(!own.consume(f));
    }
    {
        ClutchObservationOwner own;auto f=frame();
        f.world_token=0;CHECK(!own.consume(f));
    }
    {
        ClutchObservationOwner own;auto f=frame();
        CHECK(own.consume(f));f.tick=2;f.world_token=100;
        CHECK(!own.consume(f) && own.closed());
    }
    {
        ClutchObservationOwner own;auto f=frame();
        CHECK(own.consume(f));f.tick=2;f.player_token=999;
        CHECK(!own.consume(f) && own.closed());
    }
    {
        ClutchObservationOwner own;auto f=frame();
        CHECK(own.consume(f));f.tick=2;f.generation=3;
        CHECK(!own.consume(f) && own.closed());
    }
    {
        ClutchObservationOwner own;auto f=frame();
        bool accepted=false;
        std::thread foreign([&]{accepted=own.consume(f).has_value();});
        foreign.join();
        CHECK(!accepted && own.closed());
    }
}
static void source_ground_before_candidates() {
    ClutchObservationOwner own;
    auto f=frame();
    f.min_height=2;
    f.ground_block_ids[0]=0;
    f.ground_block_ids[1]=4;
    f.ground_blocked=true;
    f.candidate_block_ids.assign(6,0); // source stopped before these reads
    auto w=own.consume(f);
    CHECK(w && !own.closed());
    CHECK(w->ground_by_depth[0] && !*w->ground_by_depth[0]);
    CHECK(w->ground_by_depth[1] && *w->ground_by_depth[1]);
    CHECK(!w->source_solid[0]);
    SourceWalkBank<8> rng;
    CHECK(rng.configure({1,2,3,4}));
    const auto pre=rng.rng_state();
    const auto p=clutch_plan_original(*w,rng);
    CHECK(p.admitted && !p.found && p.visited==0);
    CHECK(rng.rng_state()==pre);

    ClutchObservationOwner forged;
    f.ground_blocked=false;
    CHECK(!forged.consume(f));
    ClutchObservationOwner unsampled;
    f.ground_blocked=true;
    f.candidate_block_ids[1]=1;
    CHECK(!unsampled.consume(f));
    ClutchObservationOwner extra;
    f.candidate_block_ids[1]=0;
    f.ground_block_ids[2]=0;
    CHECK(!extra.consume(f));
}
static void source_prior_evidence_without_forged_ray() {
    ClutchObservationOwner owner;
    auto f=frame();
    f.previous=ClutchSourcePrior{};
    f.previous->block={-1,63,0};
    f.previous->last_aim={0,63.5,.5};
    f.previous_block_id=1;
    auto w=owner.consume(f);
    CHECK(w && w->prior && w->prior->solid_known &&
          w->prior->solid && !w->prior->ray_known);
    SourceWalkBank<8> rng;
    CHECK(rng.configure({1,2,3,4}));
    const auto before=rng.rng_state();
    const auto p=clutch_plan_original(*w,rng);
    CHECK(!p.admitted && rng.rng_state()==before);

    ClutchObservationOwner forgedRay;
    f.previous->ray_known=true;
    f.previous->ray_hit=ClutchCandidatePos{-1,63,0};
    CHECK(!forgedRay.consume(f) && forgedRay.closed());

    ClutchObservationOwner noObservation;
    f.previous->ray_known=false;f.previous->ray_hit.reset();
    f.previous_block_id=-1;
    CHECK(!noObservation.consume(f));

    ClutchObservationOwner airPrior;
    f.previous_block_id=0;
    const auto air=airPrior.consume(f);
    CHECK(air && air->prior && !air->prior->solid);
    SourceWalkBank<8> fresh;
    CHECK(fresh.configure({1,2,3,4}));
    const auto fallback=clutch_plan_original(*air,fresh);
    CHECK(fallback.admitted && fallback.found &&
          fallback.source_index==5);
}
int main() {
    try {
        real_source_planner_intake();
        strict_negative_controls();
        source_ground_before_candidates();
        source_prior_evidence_without_forged_ray();
        std::cout<<"R238_NATIVE_CLUTCH_OBSERVATION_GATE_PASS\n";
    } catch(const std::exception& e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
    return 0;
}
