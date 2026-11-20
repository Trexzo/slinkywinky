#include "slinkywinky/block_in_controller.hpp"
#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace slinkywinky::reference_compat;
static void check(bool pass,const char* label,int line) {
    if(!pass)throw std::runtime_error(
        "R231 Block In line "+std::to_string(line)+": "+label);
}
#define CHECK(x) check(!!(x),#x,__LINE__)
static void close(double got,double want,double epsilon=0.00004) {
    if(!std::isfinite(got)||std::fabs(got-want)>epsilon)
        throw std::runtime_error(
            "R231 geometry expected "+std::to_string(want)+
            " got "+std::to_string(got));
}
static void point(const std::optional<BlockInVec3>& v,
                  double x,double y,double z) {
    CHECK(v);
    close(v->x,x);close(v->y,y);close(v->z,z);
}
static BlockInWorldInput valid_world() {
    BlockInWorldInput w{};
    w.in_game=true;
    w.on_ground=true;
    w.rotation_priority_granted=true;
    w.held_block_item=true;
    w.player_x=0.5;w.player_y=64;w.player_z=0.5;
    for(auto& roof:w.roof_block_ids)roof=0u;
    for(unsigned i=0;i<4;++i) {
        w.wall_intersects_player[i]=false;
        w.wall_block_ids[i]={1u,0u,1u,1u};
    }
    BlockInSourceAim source{};
    source.first_walk=.5f;
    source.second_walk=.5f;
    source.eye={.5,63.0,.5};
    w.source_aim=source;
    return w;
}
static void check_geometry() {
    constexpr int bx=10,by=63,bz=20;
    // Exact original rdata offsets and per-case C++ target equations.
    point(block_in_source_target(33,bx,by,bz,.5f,.5f),
          10,66.7,19.85);
    point(block_in_source_target(34,bx,by,bz,.5f,.5f),
          11.15,66.7,20);
    point(block_in_source_target(35,bx,by,bz,.5f,.5f),
          11,66.7,21.15);
    point(block_in_source_target(36,bx,by,bz,.5f,.5f),
          9.85,66.7,21);
    point(block_in_source_target(37,bx,by,bz,.5f,.5f),
          10,66.7,19.85);
    point(block_in_source_target(38,bx,by,bz,.5f,.5f),
          11.15,66.7,20);
    point(block_in_source_target(39,bx,by,bz,.5f,.5f),
          11,66.7,21.15);
    point(block_in_source_target(40,bx,by,bz,.5f,.5f),
          9.85,66.7,21);
    point(block_in_source_target(41,bx,by,bz,.5f,.5f),
          10.5,67,20.5); // final roof underside
    // Ordered original random walk bounds: -0.25..-0.4 foot
    // level, +0.25..+0.4 at head/above.
    point(block_in_source_target(0,bx,by,bz,.5f,.5f),
          10.5,63,21.825);
    point(block_in_source_target(1,bx,by,bz,.5f,.5f),
          10.5,64,21.175);
    point(block_in_source_target(2,bx,by,bz,.5f,.5f),
          10.5,65,21.175);
    point(block_in_source_target(3,bx,by,bz,.5f,.5f),
          9.175,63,20.5);
    point(block_in_source_target(4,bx,by,bz,.5f,.5f),
          9.825,64,20.5);
    point(block_in_source_target(6,bx,by,bz,.5f,.5f),
          10.5,63,19.175);
    point(block_in_source_target(7,bx,by,bz,.5f,.5f),
          10.5,64,19.825);
    point(block_in_source_target(9,bx,by,bz,.5f,.5f),
          11.825,63,20.5);
    point(block_in_source_target(10,bx,by,bz,.5f,.5f),
          11.175,64,20.5);
    // Units translate to original constant endpoints, even the
    // descending negative ranges.
    point(block_in_source_target(33,bx,by,bz,0.0f,1.0f),
          10,66.6,19.8);
    point(block_in_source_target(34,bx,by,bz,1.0f,0.0f),
          11.2,66.6,20);
    point(block_in_source_target(0,bx,by,bz,0.0f,1.0f),
          10.5,63,21.9);
    point(block_in_source_target(1,bx,by,bz,0.0f,1.0f),
          10.5,64,21.1);
    CHECK(!block_in_source_target(32,bx,by,bz,.5f,.5f));
    CHECK(!block_in_source_target(12,bx,by,bz,.5f,.5f));
    CHECK(!block_in_source_target(42,bx,by,bz,.5f,.5f));
    CHECK(!block_in_source_target(0,bx,by,bz,-.01f,.5f));
    CHECK(!block_in_source_target(0,bx,by,bz,.5f,1.01f));
    CHECK(!block_in_source_target(0,bx,by,bz,
          std::numeric_limits<float>::quiet_NaN(),.5f));
}
static void check_aim() {
    auto a=valid_world().source_aim.value();
    const auto look=block_in_source_look(0,0,63,0,a);
    CHECK(look);
    close(look->target.x,.5);
    close(look->target.y,63);
    close(look->target.z,1.825);
    close(look->absolute_yaw,0);
    close(look->absolute_pitch,0);
    close(look->diff_yaw,0);
    close(look->diff_pitch,0);
    a.current_yaw=350.0f;
    a.silent_yaw=20.0f;
    close(block_in_source_look(0,0,63,0,a)->diff_yaw,-10);
    a.current_yaw=170.0f;
    a.silent_yaw=20.0f;
    close(block_in_source_look(0,0,63,0,a)->diff_yaw,170);
    a.eye=look->target;
    CHECK(!block_in_source_look(0,0,63,0,a)); // no zero look vector
    a=valid_world().source_aim.value();
    a.eye.x=std::numeric_limits<double>::quiet_NaN();
    CHECK(!block_in_source_look(0,0,63,0,a));
    a=valid_world().source_aim.value();
    a.silent_pitch=std::numeric_limits<float>::infinity();
    CHECK(!block_in_source_look(0,0,63,0,a));
}
static void check_controller() {
    BlockInController c;
    auto w=valid_world();
    const auto decision=c.on_tick(w);
    CHECK(decision.admitted && decision.proposed_click &&
          decision.proposed_placement_bit==0u);
    point(decision.proposed_world_target,.5,63,1.825);
    // Cross-compiler trig/atan2 differences can produce sub-ULP
    // residuals, even for a mathematically zero look delta.
    // Assert the decision exists and compare with the same source
    // geometry tolerance as check_aim(), not exact float equality.
    CHECK(decision.proposed_absolute_yaw &&
          decision.proposed_absolute_pitch &&
          decision.proposed_yaw_delta &&
          decision.proposed_pitch_delta);
    close(*decision.proposed_absolute_yaw,0);
    close(*decision.proposed_absolute_pitch,0);
    close(*decision.proposed_yaw_delta,0);
    close(*decision.proposed_pitch_delta,0);
    CHECK(c.placed_mask()==1ull);
    const int previous_mode=c.movement_mode();
    // Even a valid source geometry input that collides with its
    // target cannot consume a placement mark or reset valid state.
    auto invalid=w;
    invalid.source_aim->eye={.5,64,0.5};
    invalid.player_y=64.02;
    invalid.roof_block_ids[1].reset();
    CHECK(!c.on_tick(invalid).admitted);
    CHECK(c.placed_mask()==1ull &&
          c.movement_mode()==previous_mode);
    invalid=w;
    invalid.source_aim->first_walk=1.3f;
    CHECK(!c.on_tick(invalid).admitted &&
          c.placed_mask()==1ull);
    // New 3D source target is authoritative over legacy supplied diff.
    c.reset();
    w.diff_yaw=177.0f;w.diff_pitch=66.0f;
    const auto overridden=c.on_tick(w);
    CHECK(overridden.admitted && overridden.proposed_click &&
          overridden.proposed_yaw_delta==0.0f &&
          overridden.proposed_pitch_delta==0.0f);
    // If no source aim evidence, the R230 typed-diff API is kept
    // independently, without fabricating a source world target.
    c.reset();
    w.source_aim.reset();
    w.diff_yaw=0;w.diff_pitch=0;
    const auto legacy=c.on_tick(w);
    CHECK(legacy.admitted && legacy.proposed_click &&
          !legacy.proposed_world_target);
    // R231: source-derived walk-to-center rotation is no longer
    // supplied by a mutable UI/preview helper. Target is floor+0.5.
    c.reset();
    w=valid_world();
    w.player_x=.2;w.player_z=.2;
    const auto move=c.on_tick(w);
    CHECK(move.admitted && move.active && move.movement_mode==1);
    const auto center=c.on_rotation();
    CHECK(center.override_yaw && center.override_pitch);
    CHECK(center.proposed_yaw && center.proposed_pitch);
    close(*center.proposed_yaw,-45.0);
    close(*center.proposed_pitch,0.0);
    // Source look vector must not be zero; reject without mutating the
    // live placement mask or movement ownership.
    c.reset();
    w=valid_world();
    w.source_aim->eye=*block_in_source_target(
        0u,0,63,0,.5f,.5f);
    const auto invalid_zero=c.on_tick(w);
    CHECK(!invalid_zero.admitted && c.placed_mask()==0ull &&
          c.movement_mode()==0);
    // Native event16 dispatch accepts the new source-aim route.
    ModuleExecutionBridge bridge;
    auto mod=bridge.add({"Block In",3,
          {{0,0},{5,0},{13,-2000},{15,-1000},{16,-500}},false});
    CHECK(bridge.bind_block_in_from_catalog());
    CHECK(bridge.request(mod,true) && bridge.apply(true,true));
    ExecutionFrame f;
    f.event=kMovementTickEvent;
    f.world_present=f.player_present=true;
    f.block_in_tick=BlockInTickPayload{valid_world(),{},0};
    CHECK(bridge.dispatch(f));
    CHECK(f.block_in_tick->executions==1 &&
          f.block_in_tick->decision.proposed_click);
    point(f.block_in_tick->decision.proposed_world_target,.5,63,1.825);
    bridge.stop();
}
int main() {
    try{
        check_geometry();
        check_aim();
        check_controller();
        std::cout<<"R231_SOURCE_TARGET_LOOK_AT_ROOF_WALL_AND_NATIVE_CALLBACK_PASS "
          "geometry=source_corners rotationalMath=source delta=0 "
          "rng=typed_external gameWrites=0\n";
        return 0;
    }catch(const std::exception& e) {
        std::cerr<<e.what()<<"\n";return 1;
    }
}
