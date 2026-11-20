#include "slinkywinky/module_execution_bridge.hpp"
#include "slinkywinky/clutch_candidate_planner.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
using namespace slinkywinky::reference_compat;
static void req(bool x,const char* desc,int line) {
    if(!x)throw std::runtime_error(
        "R235 Clutch candidate line "+std::to_string(line)+": "+desc);
}
#define CHECK(x) req(!!(x),#x,__LINE__)
static void nearly(double got,double expected,double tolerance=0.0001) {
    if(!std::isfinite(got)||std::fabs(got-expected)>tolerance)
        throw std::runtime_error(
            "expected "+std::to_string(expected)+
            " got "+std::to_string(got));
}
static constexpr std::array<std::uint32_t,4> seed{{1,2,3,4}};
static ClutchCandidateWorld basic() {
    ClutchCandidateWorld w{};
    w.box_lo={.2,64.0,.2};
    w.box_hi={.8,65.8,.8};
    w.eye={.5,65.5,.5};
    w.current_yaw=-90;
    w.current_pitch=70;
    w.range_index=1;
    w.max_aim_degrees=300;
    w.clock_seconds=1.1;
    w.source_solid.fill(false);
    return w;
}
static void source_table() {
    CHECK(kClutchDistanceCounts.size()==8);
    CHECK(kClutchSourceOffsets.size()==167);
    CHECK((kClutchDistanceCounts==std::array<unsigned,8>{
        1,6,19,43,79,123,163,167}));
    CHECK((decode_clutch_candidate(kClutchSourceOffsets[0])==
           ClutchCandidatePos{0,0,0}));
    CHECK((decode_clutch_candidate(kClutchSourceOffsets[1])==
           ClutchCandidatePos{-1,0,0}));
    CHECK((decode_clutch_candidate(kClutchSourceOffsets[2])==
           ClutchCandidatePos{0,-1,0}));
    CHECK((decode_clutch_candidate(kClutchSourceOffsets[3])==
           ClutchCandidatePos{0,0,-1}));
    CHECK((decode_clutch_candidate(kClutchSourceOffsets[5])==
           ClutchCandidatePos{1,0,0}));
    CHECK((decode_clutch_candidate(kClutchSourceOffsets[166])==
           ClutchCandidatePos{3,-1,3}));
    for(unsigned rank=0;rank<8;++rank) {
        const unsigned lo=rank==0?0:kClutchDistanceCounts[rank-1];
        const unsigned hi=kClutchDistanceCounts[rank];
        CHECK(hi>lo && hi<=167);
        for(unsigned i=lo;i<hi;++i)
            CHECK(clutch_candidate_rank(
                decode_clutch_candidate(kClutchSourceOffsets[i]))==
                static_cast<int>(rank));
    }
}
static void target_geometry() {
    const ClutchCandidatePos base{0,63,0};
    const BlockInVec3 center{.5,63.01,.5};
    const BlockInVec3 eye{.5,64.5,.5};
    auto left=clutch_source_side_aim(
        {-1,0,0},base,center,eye,0,0,.1f,0,0,true,{});
    CHECK(left);
    nearly(left->target.x,0);
    nearly(left->target.y,63.5);
    nearly(left->target.z,.5);
    nearly(left->yaw_diff,90);
    nearly(left->pitch_diff,63.434949,0.001);
    CHECK(clutch_source_reachable(eye,left->target));
    auto right=clutch_source_side_aim(
        {1,0,0},base,center,eye,-90,0,.1f,0,0,true,{});
    CHECK(right);
    nearly(right->target.x,1);
    nearly(right->target.z,.5);
    nearly(right->yaw_diff,0);
    auto zplus=clutch_source_side_aim(
        {0,0,1},base,center,eye,0,0,.1f,0,0,true,{});
    CHECK(zplus);
    nearly(zplus->target.z,1);
    nearly(zplus->target.x,.5);
    const auto top=clutch_source_top_aim(
        base,{0,0,0},center,eye,0,0,.1f,0,0);
    CHECK(top);
    nearly(top->target.x,.5);
    nearly(top->target.y,64);
    nearly(top->target.z,.5);
    const auto farther=clutch_source_side_aim(
        {-2,-1,0},base,center,eye,0,0,.1f,0,0,true,{});
    CHECK(farther);
    nearly(farther->target.y,62.75);
    CHECK(!clutch_source_top_aim(
        base,{0,0,0},center,eye,0,0,.5f,0,0));
    CHECK(!clutch_source_look(eye,eye,0,0));
    CHECK(!clutch_source_reachable(eye,{.5,64.5,6.0}));
    CHECK(clutch_source_reachable(eye,{.5,64.5,4.98}));
    CHECK(!clutch_source_reachable(eye,{.5,64.5,4.99}));
}
static void source_priority() {
    SourceWalkBank<8> walker;
    CHECK(walker.configure(seed));
    auto w=basic();
    w.range_index=0;
    w.source_solid[0]=true;
    auto t=clutch_plan_original(w,walker);
    CHECK(t.admitted && t.found && t.face_up && t.source_index==0 &&
          t.visited==1);
    CHECK((t.selected==ClutchCandidatePos{0,63,0}));
    nearly(t.aim.y,64);
    // Directly below always wins, without source side reach/angle tests.
    SourceWalkBank<8> far;
    CHECK(far.configure(seed));
    w.eye={.5,150,.5};
    w.max_aim_degrees=0;
    t=clutch_plan_original(w,far);
    CHECK(t.admitted && t.found && t.face_up);
    SourceWalkBank<8> sideways;
    CHECK(sideways.configure(seed));
    w=basic();
    w.sideways_only=true;
    w.source_solid[0]=true;
    w.source_solid[5]=true;
    const auto before=sideways.rng_state();
    t=clutch_plan_original(w,sideways);
    CHECK(t.admitted && !t.found && t.visited==1);
    CHECK(sideways.rng_state()==before); // early source return

    // A source top candidate at rank1 overrides even an earlier
    // side candidate: the source returns immediately at (0,-1,0).
    SourceWalkBank<8> top_over_side;
    CHECK(top_over_side.configure(seed));
    w=basic();
    w.source_solid[1]=true;
    w.source_solid[2]=true;
    t=clutch_plan_original(w,top_over_side);
    CHECK(t.admitted && t.found && t.face_up &&
          t.source_index==2 && t.visited==3);
    CHECK((t.selected==ClutchCandidatePos{0,62,0}));

    // First occupied side at rank1 cuts off all farther ranks,
    // even if rank2 would offer a lower angular cost.
    SourceWalkBank<8> side;
    CHECK(side.configure(seed));
    w=basic();
    w.source_solid[1]=true; // left
    t=clutch_plan_original(w,side);
    CHECK(t.admitted && t.found && !t.face_up &&
          t.source_index==1 && t.visited==6);
    CHECK((t.selected==ClutchCandidatePos{-1,63,0}));
    CHECK(t.angular_cost>100);

    // Compare two occupied SAME rank: choose lower angular cost.
    SourceWalkBank<8> both;
    CHECK(both.configure(seed));
    w.source_solid[5]=true; // right, yaw -90 matches player's yaw
    t=clutch_plan_original(w,both);
    CHECK(t.admitted && t.found && t.source_index==5 &&
          t.visited==6 && !t.face_up);
    CHECK((t.selected==ClutchCandidatePos{1,63,0}));
    CHECK(t.angular_cost<30);
    const auto side_aim=t.aim;
    // Original checks reach on side blocks; top bypass above.
    SourceWalkBank<8> out_of_reach;
    CHECK(out_of_reach.configure(seed));
    w.eye={.5,150,.5};
    t=clutch_plan_original(w,out_of_reach);
    CHECK(t.admitted && !t.found && t.visited==6);
    SourceWalkBank<8> angle;
    CHECK(angle.configure(seed));
    w=basic();w.source_solid[5]=true;
    w.max_aim_degrees=2;
    t=clutch_plan_original(w,angle);
    CHECK(t.admitted && !t.found && t.visited==6);

    // Source uses bounded range index 0..7, not a radial distance.
    SourceWalkBank<8> ranks;
    CHECK(ranks.configure(seed));
    w=basic();w.range_index=0;
    w.source_solid[1]=true;
    t=clutch_plan_original(w,ranks);
    CHECK(t.admitted && !t.found && t.visited==1);
    CHECK(std::isfinite(side_aim.x));
}
static void previous_and_ground() {
    SourceWalkBank<8> prior_bank;
    CHECK(prior_bank.configure(seed));
    auto w=basic();
    w.range_index=0;
    ClutchSourcePrior old{};
    old.block={-1,63,0};
    old.last_aim={0,63.5,.5};
    old.solid_known=old.solid=old.ray_known=true;
    w.prior=old;
    auto selected=clutch_plan_original(w,prior_bank);
    CHECK(selected.admitted && selected.found &&
          selected.preserved_prior &&
          (selected.selected==old.block));
    // A wrong block on the world ray rejects the old target.
    SourceWalkBank<8> rejected;
    CHECK(rejected.configure(seed));
    w.prior->ray_hit=ClutchCandidatePos{2,63,0};
    selected=clutch_plan_original(w,rejected);
    CHECK(selected.admitted && !selected.found);
    SourceWalkBank<8> unseen;
    CHECK(unseen.configure(seed));
    w.prior->ray_known=false;
    const auto before=unseen.rng_state();
    selected=clutch_plan_original(w,unseen);
    CHECK(!selected.admitted && unseen.rng_state()==before);

    // R236: original call order. An unreachable old block must NOT
    // demand a raycast result; the candidate scan may still find a
    // different, reachable block. Before R236 this was rejected.
    SourceWalkBank<8> unreachable_prior;
    CHECK(unreachable_prior.configure(seed));
    auto fallback_world=basic();
    fallback_world.source_solid[5]=true;
    ClutchSourcePrior far_old{};
    far_old.block={20,63,0};
    far_old.last_aim={20,63.5,.5};
    far_old.solid_known=far_old.solid=true;
    far_old.ray_known=false; // no source raycast is necessary
    fallback_world.prior=far_old;
    selected=clutch_plan_original(fallback_world,unreachable_prior);
    CHECK(selected.admitted && selected.found &&
          !selected.preserved_prior && selected.source_index==5 &&
          (selected.selected==ClutchCandidatePos{1,63,0}));

    // The same missing ray evidence MUST still reject a genuinely
    // reachable previous candidate; otherwise an unverified block
    // could silently be retained as though it were visible.
    SourceWalkBank<8> reachable_prior;
    CHECK(reachable_prior.configure(seed));
    auto requires_ray=basic();
    ClutchSourcePrior near_old{};
    near_old.block={-1,63,0};
    near_old.last_aim={0,63.5,.5};
    near_old.solid_known=near_old.solid=true;
    near_old.ray_known=false;
    requires_ray.prior=near_old;
    const auto owned_rng=reachable_prior.rng_state();
    selected=clutch_plan_original(requires_ray,reachable_prior);
    CHECK(!selected.admitted && reachable_prior.rng_state()==owned_rng);

    SourceWalkBank<8> ground;
    CHECK(ground.configure(seed));
    w=basic();
    w.source_solid[0]=true;
    w.min_height=2.0f;
    w.ground_by_depth[0]=false;
    // Missing second depth cannot be silently assumed clear.
    selected=clutch_plan_original(w,ground);
    CHECK(!selected.admitted);
    w.ground_by_depth[1]=true;
    selected=clutch_plan_original(w,ground);
    CHECK(selected.admitted && !selected.found);
    w.ground_by_depth[1]=false;
    selected=clutch_plan_original(w,ground);
    CHECK(selected.admitted && selected.found && selected.face_up);

    SourceWalkBank<8> missing;
    CHECK(missing.configure(seed));
    w=basic();
    w.source_solid[0].reset();
    const auto snapshot=missing.rng_state();
    CHECK(!clutch_plan_original(w,missing).admitted);
    CHECK(missing.rng_state()==snapshot);
    w.range_index=8;
    CHECK(!clutch_plan_original(w,missing).admitted);
    CHECK(missing.rng_state()==snapshot);
    w=basic();
    ClutchSourcePrior invalid_prior{};
    invalid_prior.block={
        std::numeric_limits<int>::min(),63,0};
    invalid_prior.solid_known=invalid_prior.solid=
        invalid_prior.ray_known=true;
    w.prior=invalid_prior;
    CHECK(!clutch_plan_original(w,missing).admitted);
    CHECK(missing.rng_state()==snapshot);
}
// R243: the prior ray phase is a source-exact pure preflight, not a
// speculative visibility assumption or a second live RNG draw.
static void prior_ray_preflight_regression() {
    SourceWalkBank<8> initial;
    CHECK(initial.configure(seed));
    auto w=basic();
    w.range_index=0;
    const auto starting_rng=initial.rng_state();
    auto request=clutch_prior_ray_preflight(w,initial);
    CHECK(request.admitted && !request.needs_ray);
    CHECK(initial.rng_state()==starting_rng);

    ClutchSourcePrior old{};
    old.block={-1,63,0};
    old.last_aim={0,63.5,.5};
    old.solid_known=old.solid=true;
    w.prior=old;
    request=clutch_prior_ray_preflight(w,initial);
    CHECK(request.admitted && request.needs_ray);
    CHECK((request.expected_block==old.block));
    nearly(request.from.x,w.eye.x);
    nearly(request.from.y,w.eye.y);
    nearly(request.from.z,w.eye.z);
    CHECK(clutch_valid_vec(request.to));
    CHECK(clutch_source_reachable(request.from,request.to));
    CHECK(initial.rng_state()==starting_rng);

    // No-hit evidence is a verified null ray, which original logic treats
    // as the previous block; only phase two consumes planner randomness.
    SourceWalkBank<8> phase_two=initial;
    w.prior->ray_known=true;
    w.prior->ray_hit.reset();
    const auto selected=clutch_plan_original(w,phase_two);
    CHECK(selected.admitted && selected.found &&
          selected.preserved_prior && (selected.selected==old.block));
    nearly(selected.aim.x,request.to.x);
    nearly(selected.aim.y,request.to.y);
    nearly(selected.aim.z,request.to.z);
    CHECK(initial.rng_state()==starting_rng);
    CHECK(phase_two.rng_state()!=starting_rng);

    // A real ray hit on a different block MUST revoke the prior.
    SourceWalkBank<8> obstruction=initial;
    w.prior->ray_hit=ClutchCandidatePos{2,63,0};
    const auto blocked=clutch_plan_original(w,obstruction);
    CHECK(blocked.admitted && !blocked.preserved_prior && !blocked.found);
    w.prior->ray_known=false;
    w.prior->ray_hit.reset();
    CHECK(clutch_prior_ray_preflight(w,initial).needs_ray);

    // Far prior target cannot trigger a ray; the source scan may fall
    // through to an alternative block without any visibility evidence.
    w.range_index=1;
    w.source_solid[5]=true;
    w.prior->block={20,63,0};
    w.prior->last_aim={20,63.5,.5};
    request=clutch_prior_ray_preflight(w,initial);
    CHECK(request.admitted && !request.needs_ray);
    SourceWalkBank<8> fallback=initial;
    const auto alternative=clutch_plan_original(w,fallback);
    CHECK(alternative.admitted && alternative.found &&
          !alternative.preserved_prior && alternative.source_index==5);

    // Unknown prior block and pre-supplied ray results are not a
    // valid first phase, even if the caller invents a plausible aim.
    w.prior->solid_known=false;
    CHECK(!clutch_prior_ray_preflight(w,initial).admitted);
    w.prior->solid_known=true;
    w.prior->ray_known=true;
    CHECK(!clutch_prior_ray_preflight(w,initial).admitted);
    w.prior->ray_known=false;
    w.prior->block={-1,63,0};
    w.prior->last_aim={0,63.5,.5};
    w.source_solid[1].reset();
    CHECK(!clutch_prior_ray_preflight(w,initial).admitted);
    w.source_solid[1]=false;
    w.min_height=2.0f;
    w.ground_by_depth[0]=true;
    request=clutch_prior_ray_preflight(w,initial);
    CHECK(request.admitted && !request.needs_ray);
    w.ground_by_depth[0].reset();
    CHECK(!clutch_prior_ray_preflight(w,initial).admitted);
    CHECK(initial.rng_state()==starting_rng);
}

static ClutchLateInput late_input() {
    ClutchLateInput v{};
    v.in_game=true;
    v.rotation_priority_granted=true;
    v.holding_blocks=true;
    v.aim_speed=15;
    v.aim_multiplier=.5f;
    v.aim_multiplier_strength=.5f;
    v.use_source_walk=true;
    v.clock_seconds=1.1;
    v.source_randomisation=1;
    v.source_world=basic();
    v.source_world->source_solid[1]=true;
    return v;
}
static void controller_bridge_and_global() {
    ClutchController control,repeat;
    CHECK(control.configure_source_walk_seed(seed));
    CHECK(repeat.configure_source_walk_seed(seed));
    auto in=late_input();
    // Contradictory old caller-provided candidate must be overridden.
    in.has_planned_candidate=true;
    in.candidate_block={99,99,99};
    in.aim_yaw_diff=-99;
    const auto a=control.on_late(in);
    const auto b=repeat.on_late(in);
    CHECK(a.admitted && a.planned && a.source_plan &&
          a.source_plan->found &&
          a.source_plan->source_index==1);
    CHECK((control.planned_block()==ClutchBlock{-1,63,0}));
    CHECK(a.source_plan->selected==b.source_plan->selected);
    CHECK(a.proposed_yaw_delta==b.proposed_yaw_delta);
    CHECK(control.source_walks().rng_state()==
          repeat.source_walks().rng_state());
    CHECK(a.source_yaw_scale && a.proposed_yaw_delta);

    // Source scan fails closed on unknown first block and rolls back
    // all source clocks and previously established plan/rotation.
    const auto known=control.source_walks().rng_state();
    auto broken=late_input();
    broken.clock_seconds=2;
    broken.source_world->clock_seconds=2;
    broken.source_world->source_solid[0].reset();
    const auto denied=control.on_late(broken);
    CHECK(!denied.admitted);
    CHECK(control.source_walks().rng_state()==known);
    CHECK(control.planned() && control.on_rotation_flag().override_yaw);

    ModuleExecutionBridge native;
    const auto id=native.add({"Clutch",3,
        {{0,0},{13,-100},{18,-1000}},false});
    CHECK(native.bind_clutch_from_catalog());
    CHECK(native.request(id,true) && native.apply(true,true));
    CHECK(native.configure_shared_source_walk_seed(seed));
    // First occupied side causes real xoshiro draws. A *later*
    // missing candidate observation must reject the whole source
    // scan without committing the shared RNG or the plan/rotation.
    const auto shared_before=*native.shared_source_rng_state();
    ExecutionFrame failed{};
    failed.event=kAntiDebuffEvent;
    failed.world_present=failed.player_present=true;
    failed.clutch_late=ClutchLatePayload{late_input(),{},0};
    failed.clutch_late->input.source_world->source_solid[2].reset();
    CHECK(native.dispatch(failed));
    CHECK(failed.clutch_late->executions==1 &&
          !failed.clutch_late->decision.admitted);
    CHECK(*native.shared_source_rng_state()==shared_before);
    ExecutionFrame frame{};
    frame.event=kAntiDebuffEvent;
    frame.world_present=frame.player_present=true;
    frame.clutch_late=ClutchLatePayload{late_input(),{},0};
    CHECK(native.dispatch(frame));
    CHECK(frame.clutch_late->executions==1);
    CHECK(frame.clutch_late->decision.source_plan &&
          frame.clutch_late->decision.source_plan->found &&
          frame.clutch_late->decision.source_plan->source_index==1);
    CHECK(*native.shared_source_rng_state()!=seed);
    native.stop();
}
int main() {
    try {
        source_table();
        target_geometry();
        source_priority();
        previous_and_ground();
        prior_ray_preflight_regression();
        controller_bridge_and_global();
        std::cout<<"R235_ORIGINAL_CLUTCH_167_CANDIDATES_SOURCE_RANKING_PASS "
           "face=top_and_side reach=strict4.49 prior=ray_checked "
           "sourceWalk=shared gameWrites=0\n";
        return 0;
    }catch(const std::exception& e) {
        std::cerr<<e.what()<<"\n";
        return 1;
    }
}
