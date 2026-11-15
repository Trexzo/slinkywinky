#include "slinkywinky/movement_proposal_resolution.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace slinkywinky::reference_compat;
static void require(bool value,const char* why,int line) {
    if(!value)throw std::runtime_error(
        "R223 line "+std::to_string(line)+": "+why);
}
#define CHECK(x) require(static_cast<bool>(x),#x,__LINE__)

static ExecutionFrame sprint_sample() {
    ExecutionFrame frame;
    frame.event=kStrafeSnapshotEvent;
    frame.world_present=frame.player_present=true;
    frame.strafe_snapshot=StrafeSnapshotPayload{false,0};
    return frame;
}
static ExecutionFrame motion_sample() {
    ExecutionFrame frame;
    frame.event=kStrafeSteeringEvent;
    frame.world_present=frame.player_present=true;
    StrafeMotionInput strafe{};
    strafe.forward=1.0f;
    strafe.on_ground=true;
    strafe.motion_x=0.3;
    strafe.motion_y=0.4;
    strafe.motion_z=0.0;
    FastAccelInput accel{};
    accel.forward=strafe.forward;
    accel.strafe=strafe.strafe;
    accel.on_ground=strafe.on_ground;
    accel.motion_x=strafe.motion_x;
    accel.motion_y=strafe.motion_y;
    accel.motion_z=strafe.motion_z;
    accel.movement_speed=0.4f;
    accel.settings.acceleration=0.5f;
    frame.strafe_steering=StrafeSteeringPayload{strafe,{},0};
    frame.fast_accel=FastAccelTickPayload{accel,{},0};
    return frame;
}
static void run() {
    ModuleExecutionBridge bridge;
    const auto strafe=bridge.add({"Strafe",1,{{21,1000},{22,100}},false});
    const auto accel=bridge.add({"Fast Accel",1,{{22,0}},false});
    CHECK(bridge.bind_strafe_from_catalog());
    CHECK(bridge.bind_fast_accel_from_catalog());
    CHECK(bridge.request(strafe,true) && bridge.request(accel,true));
    CHECK(bridge.apply(true,true));

    auto frame=motion_sample();
    CHECK(bridge.dispatch(frame));
    CHECK(!frame.strafe_steering->decision.proposed_motion);
    CHECK(frame.fast_accel->decision.proposed_motion);
    auto selected=resolve_event22_motion(frame);
    CHECK(selected.owner==MovementProposalOwner::fast_accel_priority_0);
    CHECK(selected.motion && std::isfinite(selected.motion->x));
    CHECK(selected.motion->y==frame.fast_accel->input.motion_y);

    frame=sprint_sample();
    CHECK(bridge.dispatch(frame));
    frame=motion_sample();
    CHECK(bridge.dispatch(frame));
    CHECK(frame.strafe_steering->decision.proposed_motion);
    CHECK(frame.fast_accel->decision.proposed_motion);
    selected=resolve_event22_motion(frame);
    CHECK(selected.owner==MovementProposalOwner::strafe_priority_100);
    CHECK(selected.motion);
    CHECK(std::abs(selected.motion->x-
          frame.strafe_steering->decision.proposed_motion->x)<1e-12);
    CHECK(std::abs(selected.motion->z-
          frame.strafe_steering->decision.proposed_motion->z)<1e-12);

    auto bad=frame;
    bad.event=21;
    CHECK(!resolve_event22_motion(bad).motion);
    bad=frame;
    bad.world_present=false;
    CHECK(!resolve_event22_motion(bad).motion);
    bad=frame;
    bad.player_present=false;
    CHECK(!resolve_event22_motion(bad).motion);
    bad=frame;
    bad.executed_modules.clear();
    CHECK(!resolve_event22_motion(bad).motion);
    bad=frame;
    bad.fast_accel->input.motion_y+=0.01;
    CHECK(!resolve_event22_motion(bad).motion);
    bad=frame;
    bad.fast_accel->input.forward=0.0f;
    CHECK(!resolve_event22_motion(bad).motion);
    bad=frame;
    bad.strafe_steering->decision.proposed_motion->x=
        std::numeric_limits<double>::quiet_NaN();
    CHECK(!resolve_event22_motion(bad).motion);
    bad=frame;
    bad.fast_accel->decision.proposed_motion->z=
        std::numeric_limits<double>::infinity();
    CHECK(!resolve_event22_motion(bad).motion);
    bad=frame;
    bad.strafe_steering->decision.proposed_motion->y+=0.01;
    CHECK(!resolve_event22_motion(bad).motion);
    bad=frame;
    bad.fast_accel->executions=0; // stale decision cannot be used
    CHECK(!resolve_event22_motion(bad).motion);
    bad=frame;
    bad.strafe_steering->executions=2; // double callback not admissible
    CHECK(!resolve_event22_motion(bad).motion);

    CHECK(bridge.request(strafe,false));
    CHECK(bridge.apply(true,true));
    frame=motion_sample();
    CHECK(bridge.dispatch(frame));
    selected=resolve_event22_motion(frame);
    CHECK(selected.owner==MovementProposalOwner::fast_accel_priority_0);
    CHECK(selected.motion);
    CHECK(bridge.request(accel,false));
    CHECK(bridge.request(strafe,true));
    CHECK(bridge.apply(true,true));
    frame=sprint_sample();
    CHECK(bridge.dispatch(frame));
    frame=motion_sample();
    CHECK(bridge.dispatch(frame));
    selected=resolve_event22_motion(frame);
    CHECK(selected.owner==MovementProposalOwner::strafe_priority_100);
    CHECK(selected.motion);
    bridge.stop();
    CHECK(!bridge.dispatch(frame));
    // A stale frame cannot be re-resolved after owner teardown in any
    // production adapter: invalidate its admission data explicitly.
    frame.executed_modules.clear();
    CHECK(!resolve_event22_motion(frame).motion);

    std::cout<<"R223_EVENT22_READONLY_SINGLE_OWNER_MOTION_RESOLUTION_PASS\n";
}
int main(){
    try {run();return 0;}
    catch(const std::exception& e){
        std::cerr<<e.what()<<'\n';return 1;
    }
}
