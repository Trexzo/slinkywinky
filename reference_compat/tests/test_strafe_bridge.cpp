#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using namespace slinkywinky::reference_compat;

static void require(bool ok, const char* why, int line) {
    if (!ok) throw std::runtime_error("R222 line " + std::to_string(line) +
                                     ": " + why);
}
#define CHECK(x) require(static_cast<bool>(x), #x, __LINE__)
static bool close_to(double a, double b, double tol=1e-8) {
    return std::abs(a-b) < tol;
}

static StrafeMotionInput straight() {
    StrafeMotionInput in;
    in.forward = 1.0f;
    in.motion_x = 0.3;
    in.motion_y = 0.4;
    in.motion_z = 0;
    in.settings = {10, 0.5, 1, 1, false};
    return in;
}
static ExecutionFrame snapshot(bool sprinting=false) {
    ExecutionFrame f;
    f.event=kStrafeSnapshotEvent;
    f.world_present=true;
    f.player_present=true;
    f.strafe_snapshot=StrafeSnapshotPayload{sprinting,0};
    return f;
}
static ExecutionFrame steering() {
    ExecutionFrame f;
    f.event=kStrafeSteeringEvent;
    f.world_present=true;
    f.player_present=true;
    f.strafe_steering=StrafeSteeringPayload{straight(),{},0};
    return f;
}

static int run() {
    StrafeController policy;
    auto in=straight();
    CHECK(!policy.evaluate(in).proposed_motion); // no event21 observation
    policy.observe_sprinting(false);
    auto d=policy.evaluate(in);
    CHECK(d.proposed_motion && close_to(d.proposed_motion->x,0.0));
    CHECK(close_to(d.proposed_motion->y,0.4));
    CHECK(close_to(d.proposed_motion->z,0.3));

    in.jump_phase=true;
    d=policy.evaluate(in);
    CHECK(d.proposed_motion && close_to(d.proposed_motion->z,0.5)); // jump boost
    policy.observe_sprinting(true);
    d=policy.evaluate(in);
    CHECK(d.proposed_motion && close_to(d.proposed_motion->z,0.3)); // no boost

    in.jump_phase=false;
    in.on_ground=false;
    in.yaw_degrees=90;
    d=policy.evaluate(in);
    CHECK(d.proposed_motion && close_to(d.proposed_motion->x,0.0));
    CHECK(close_to(d.proposed_motion->z,0.0));
    CHECK(close_to(d.proposed_motion->y,0.4));

    in=straight();
    in.strafe=1;
    d=policy.evaluate(in);
    CHECK(d.proposed_motion &&
          close_to(d.proposed_motion->x,0.3/std::sqrt(2.0)));
    CHECK(close_to(d.proposed_motion->z,0.3/std::sqrt(2.0)));
    in.forward=-1;
    d=policy.evaluate(in);
    CHECK(d.proposed_motion &&
          close_to(d.proposed_motion->x,0.3/std::sqrt(2.0)));
    CHECK(close_to(d.proposed_motion->z,-0.3/std::sqrt(2.0)));

    in=straight();
    in.settings.require_weapon=true;
    in.weapon_accepted=false;
    CHECK(!policy.evaluate(in).proposed_motion);
    in.weapon_accepted=true;
    in.hurt_time=11;
    CHECK(!policy.evaluate(in).proposed_motion);
    in.hurt_time=10;
    CHECK(policy.evaluate(in).proposed_motion);
    in.forward=0;
    in.strafe=0;
    CHECK(!policy.evaluate(in).proposed_motion);
    in=straight();
    in.settings.strength_ground=0;
    CHECK(!policy.evaluate(in).proposed_motion);
    in.settings.strength_ground=1.2;
    CHECK(!policy.evaluate(in).proposed_motion);
    in.settings.strength_ground=1;
    in.motion_x=std::numeric_limits<double>::infinity();
    CHECK(!policy.evaluate(in).proposed_motion);
    in=straight();
    in.yaw_degrees=std::numeric_limits<float>::quiet_NaN();
    CHECK(!policy.evaluate(in).proposed_motion);
    in=straight();
    in.settings.max_hurt_time=std::numeric_limits<double>::infinity();
    CHECK(!policy.evaluate(in).proposed_motion);
    policy.reset();
    CHECK(!policy.has_snapshot() && !policy.evaluate(straight()).proposed_motion);

    ModuleExecutionBridge bridge;
    const auto strafe_id=bridge.add({"Strafe",1,{{21,1000},{22,100}},false});
    const auto no_slow_id=bridge.add({"No Slow",1,{{21,200}},false});
    const auto fast_id=bridge.add({"Fast Accel",1,{{22,0}},false});
    CHECK(strafe_id==0 && no_slow_id==1 && fast_id==2);
    CHECK(bridge.bind_strafe_from_catalog());
    CHECK(!bridge.bind_strafe(strafe_id));
    CHECK(bridge.bind_no_slow_from_catalog());
    CHECK(bridge.bind_fast_accel_from_catalog());
    auto coverage=bridge.coverage();
    CHECK(coverage.native_callback_subscriptions==4);
    CHECK(coverage.executable_callback_bindings==4);
    CHECK(coverage.implemented_per_event[21]==2);
    CHECK(coverage.implemented_per_event[22]==2);

    CHECK(bridge.request(strafe_id,true));
    CHECK(bridge.request(no_slow_id,true));
    CHECK(bridge.request(fast_id,true));
    CHECK(!bridge.apply(false,false));
    CHECK(bridge.apply(true,true));

    auto frame=steering();
    CHECK(bridge.dispatch(frame)); // event22 has no event21 snapshot yet
    CHECK(frame.strafe_steering->executions==1);
    CHECK(!frame.strafe_steering->decision.proposed_motion);
    frame=snapshot(false);
    frame.no_slow=NoSlowTickPayload{};
    frame.no_slow->input.forward=1;
    CHECK(bridge.dispatch(frame));
    CHECK(frame.strafe_snapshot->executions==1);
    CHECK(frame.no_slow->executions==1);
    CHECK((frame.executed_modules == std::vector<std::size_t>{strafe_id,no_slow_id}));
    frame=steering();
    CHECK(bridge.dispatch(frame));
    CHECK(frame.strafe_steering->executions==1);
    CHECK(frame.strafe_steering->decision.proposed_motion);
    CHECK(close_to(frame.strafe_steering->decision.proposed_motion->z,0.3));
    CHECK((frame.executed_modules == std::vector<std::size_t>{strafe_id,fast_id})); // subscription dispatch includes absent typed payload

    frame=steering();
    frame.fast_accel=FastAccelTickPayload{};
    frame.fast_accel->input.forward=frame.strafe_steering->input.forward;
    frame.fast_accel->input.motion_x=frame.strafe_steering->input.motion_x;
    frame.fast_accel->input.motion_y=frame.strafe_steering->input.motion_y;
    frame.fast_accel->input.motion_z=frame.strafe_steering->input.motion_z;
    frame.fast_accel->input.on_ground=frame.strafe_steering->input.on_ground;
    frame.fast_accel->input.movement_speed=0.1f;
    CHECK(bridge.dispatch(frame));
    CHECK((frame.executed_modules == std::vector<std::size_t>{strafe_id,fast_id}));
    CHECK(frame.strafe_steering->executions==1 && frame.fast_accel->executions==1);
    frame=steering();
    frame.fast_accel=FastAccelTickPayload{};
    frame.fast_accel->input.forward=0; // mismatched movement source
    CHECK(!bridge.dispatch(frame));
    CHECK(frame.executed_modules.empty());
    frame=steering();
    frame.event=21; // incorrectly typed payload for event21
    CHECK(!bridge.dispatch(frame));
    frame=steering();
    frame.player_present=false;
    CHECK(!bridge.dispatch(frame));
    frame=snapshot();
    frame.world_present=false;
    CHECK(!bridge.dispatch(frame));

    bridge.reset_strafe_world_state();
    frame=steering();
    CHECK(bridge.dispatch(frame) && !frame.strafe_steering->decision.proposed_motion);
    CHECK(bridge.request(strafe_id,false));
    CHECK(bridge.apply(true,true));
    frame=snapshot(false);
    CHECK(bridge.dispatch(frame) && frame.strafe_snapshot->executions==0);
    CHECK((frame.executed_modules==std::vector<std::size_t>{no_slow_id}));
    bridge.stop();
    CHECK(bridge.implemented_event_bindings()==0);
    CHECK(!bridge.dispatch(frame));
    CHECK(!bridge.bind_strafe_from_catalog());

    auto invalid=[](ModuleSpec spec) {
        ModuleExecutionBridge b;
        const auto id=b.add(spec);
        CHECK(!b.bind_strafe(id));
        CHECK(b.implemented_event_bindings()==0);
    };
    invalid({"Strafe",0,{{21,1000},{22,100}},false});
    invalid({"Strafe",1,{{21,999},{22,100}},false});
    invalid({"Strafe",1,{{21,1000},{22,101}},false});
    invalid({"Strafe",1,{{21,1000}},false});
    invalid({"Strafe",1,{{21,1000},{22,100},{17,0}},false});
    invalid({"Other",1,{{21,1000},{22,100}},false});

    std::cout<<"R222 Strafe PASS: actual candidate two-event metadata, "
                "steering vectors, 0.2 jump boost, state gate, shared "
                "event21/22 priority, reset, negative source and teardown\n";
    return 0;
}
int main() {
    try { return run(); }
    catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
