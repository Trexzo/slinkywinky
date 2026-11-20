#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void require(bool yes,const char* what,int line) {
    if(!yes)throw std::runtime_error("R229 Clutch line "+
        std::to_string(line)+": "+what);
}
#define CHECK(x) require(!!(x),#x,__LINE__)
static ClutchLateInput late_aim() {
    ClutchLateInput x;
    x.in_game=true;
    x.on_ground=false;
    x.has_planned_candidate=true;
    x.holding_blocks=true;
    x.candidate_block={1,2,3};
    x.face_up=true;
    x.rotation_priority_granted=true;
    x.aim_yaw_diff=40.0f;
    x.aim_pitch_diff=20.0f;
    x.aim_speed=10.0f;
    x.aim_multiplier=0.5f;
    x.aim_multiplier_strength=0.5f;
    x.snapback_duration=3.0f;
    x.snapback_delay=1.0f;
    x.snapback_delay_scale=1.0f;
    x.disable_after=true;
    x.select_blocks_mode=2;
    return x;
}
static ClutchClickInput click() {
    ClutchClickInput x;
    x.in_game=true;
    x.clicks_per_second=10.0f;
    x.hit_is_block=true;
    x.hit_top=true;
    x.hit={1,2,3};
    x.adjacent={1,3,3};
    return x;
}
static void unit_policy() {
    ClutchController c;
    CHECK(!c.on_rotation_flag().override_yaw);
    auto a=late_aim();
    auto decision=c.on_late(a);
    CHECK(decision.admitted && decision.planned && decision.rotating &&
          decision.proposed_track_owner && decision.proposed_restore_hotbar);
    CHECK(decision.proposed_yaw_delta &&
          std::fabs(*decision.proposed_yaw_delta-15.0f)<1e-6f);
    CHECK(decision.proposed_pitch_delta &&
          std::fabs(*decision.proposed_pitch_delta-10.0f)<1e-6f);
    CHECK(c.on_rotation_flag().override_yaw &&
          c.on_rotation_flag().override_pitch);
    CHECK((c.planned_block()==ClutchBlock{1,2,3}));
    auto place=c.on_tick(click());
    CHECK(place.admitted && place.proposed_use_click &&
          (place.new_planned_block==ClutchBlock{1,3,3}) &&
          (c.planned_block()==ClutchBlock{1,3,3}) &&
          place.cooldown_after==2.0f);
    place=c.on_tick(click());
    CHECK(!place.proposed_use_click && place.cooldown_after==1.0f);
    place=c.on_tick(click());
    CHECK(!place.proposed_use_click && place.cooldown_after==2.0f);
    auto wrong=click();
    wrong.hit={1,3,3};wrong.hit_top=false;
    c.on_tick(wrong);
    wrong.hit_top=true;
    place=c.on_tick(wrong);
    CHECK(place.proposed_use_click);
    c.reset();
    CHECK(!c.on_rotation_flag().override_yaw && !c.planned());

    // R248: a real ray face, when present, owns the only valid adjacent
    // coordinate; a caller cannot forge an arbitrary next block.
    ClutchController verified;
    auto selected=late_aim();
    CHECK(verified.on_late(selected).planned);
    auto vf=click();vf.observed_face=5; // contradicts legacy top flag
    CHECK(!verified.on_tick(vf).admitted &&
          verified.cooldown()==0.0f);
    vf.observed_face=1;vf.adjacent={2,2,3};
    CHECK(!verified.on_tick(vf).admitted &&
          (verified.planned_block()==ClutchBlock{1,2,3}) &&
          verified.cooldown()==0.0f);
    vf.adjacent={1,3,3};
    auto exact=verified.on_tick(vf);
    CHECK(exact.admitted && exact.proposed_use_click &&
          exact.cooldown_after==2.0f &&
          (exact.new_planned_block==ClutchBlock{1,3,3}));
    exact=verified.on_tick(vf);
    CHECK(exact.admitted && !exact.proposed_use_click &&
          exact.cooldown_after==1.0f);
    verified.reset();
    selected=late_aim();selected.face_up=false;
    CHECK(verified.on_late(selected).planned);
    vf=click();vf.hit_top=false;vf.observed_face=5;
    vf.adjacent={2,2,3};
    exact=verified.on_tick(vf);
    CHECK(exact.admitted && exact.proposed_use_click &&
          (exact.new_planned_block==ClutchBlock{2,2,3}));
    verified.reset();
    selected=late_aim();
    CHECK(verified.on_late(selected).planned);
    vf=click();vf.observed_face=1;vf.hit_is_block=false;
    CHECK(!verified.on_tick(vf).admitted);
    vf=click();vf.observed_face=7;
    CHECK(!verified.on_tick(vf).admitted);
    vf.observed_face=1;vf.clicks_per_second=21.0f;
    CHECK(!verified.on_tick(vf).admitted);
    // Source still advances click cooldown when no block is planned.
    auto empty=c.on_tick(click());
    CHECK(empty.admitted && !empty.proposed_use_click &&
          empty.cooldown_after==2.0f);
    auto invalid=click();
    invalid.clicks_per_second=0.0f;
    CHECK(!c.on_tick(invalid).admitted && c.cooldown()==2.0f);
    invalid.clicks_per_second=
        std::numeric_limits<float>::quiet_NaN();
    CHECK(!c.on_tick(invalid).admitted);
    invalid.clicks_per_second=101.0f;
    CHECK(!c.on_tick(invalid).admitted);

    c.reset();
    a=late_aim();
    a.require_air=true;
    a.on_ground=true;
    decision=c.on_late(a);
    CHECK(decision.admitted && !decision.planned &&
          !decision.proposed_yaw_delta);
    a.on_ground=false;
    a.require_damage=true;
    a.hurt_time=0;
    // Source counter starts at 0, increments with hurt_time == 0.
    for(int i=0;i<11;++i)c.on_late(a);
    decision=c.on_late(a);
    CHECK(!decision.planned);
    a.hurt_time=3;
    decision=c.on_late(a);
    CHECK(decision.planned); // damage gate resets on actual hurt
    a.has_planned_candidate=false;
    a.silent_rotation_active=true;
    a.current_yaw=170.0f;
    a.render_yaw=-170.0f;
    a.current_pitch=20.0f;
    a.render_pitch=80.0f;
    // On first post-clutch tick delay 1 -> 0, duration 3 ticks.
    decision=c.on_late(a);
    CHECK(!decision.planned && decision.rotating &&
          decision.snapback_delay_remaining==0 &&
          decision.snapback_ticks_remaining==3);
    CHECK(decision.proposed_yaw_delta &&
          std::fabs(*decision.proposed_yaw_delta-20.0f/3.0f)<1e-5f);
    CHECK(decision.proposed_pitch_delta &&
          std::fabs(*decision.proposed_pitch_delta-20.0f)<1e-5f);
    CHECK(c.on_rotation_flag().override_yaw);
    decision=c.on_late(a);
    CHECK(decision.snapback_ticks_remaining==2);
    decision=c.on_late(a);
    CHECK(decision.snapback_ticks_remaining==1);
    decision=c.on_late(a);
    CHECK(decision.snapback_ticks_remaining==0);
    decision=c.on_late(a);
    CHECK(decision.proposed_disable_module);
    decision=c.on_late(a);
    CHECK(!decision.proposed_disable_module);

    c.reset();
    a=late_aim();
    a.require_backwards=true;
    a.moving_backwards=false;
    CHECK(!c.on_late(a).planned);
    a.moving_backwards=true;
    CHECK(c.on_late(a).planned);
    a.has_planned_candidate=false;
    a.on_ground=true;
    a.jump_held=true;
    a.silent_rotation_active=true;
    a.instant_jump_snapback=true;
    a.render_yaw=15.0f;
    a.current_yaw=0.0f;
    a.render_pitch=45.0f;
    a.current_pitch=20.0f;
    a.jump_wiggle_yaw=5.0f;
    a.jump_wiggle_pitch=-2.0f;
    decision=c.on_late(a);
    CHECK(decision.admitted && !decision.planned && decision.rotating &&
          decision.proposed_yaw_delta==20.0f &&
          decision.proposed_pitch_delta==23.0f &&
          decision.snapback_delay_remaining==3);
    CHECK(c.on_rotation_flag().override_yaw);
    c.reset();
    a=late_aim();
    a.aim_speed=std::numeric_limits<float>::infinity();
    CHECK(!c.on_late(a).admitted &&
          !c.on_rotation_flag().override_yaw);
    a=late_aim();
    a.snapback_duration=-1.0f;
    CHECK(!c.on_late(a).admitted);
    a=late_aim();
    a.in_game=false;
    CHECK(c.on_late(a).admitted && !c.planned());
}
static ExecutionFrame click_frame(bool all) {
    ExecutionFrame f;
    f.event=kSafeWalkEvent;f.world_present=f.player_present=true;
    f.clutch_click=ClutchClickPayload{click(),{},0};
    if(all) {
        SafeWalkInput s{};
        s.in_game=true;
        f.safe_walk=SafeWalkTickPayload{s,{},0};
        FastPlaceInput fp{};
        fp.in_game=true;
        f.fast_place=FastPlaceTickPayload{fp,{},0};
        FastMineEarlyInput fm{};
        fm.in_game=true;
        f.fast_mine_early=FastMineEarlyPayload{fm,{},0};
    }
    return f;
}
static ExecutionFrame late_frame(bool all) {
    ExecutionFrame f;
    f.event=kAntiDebuffEvent;f.world_present=f.player_present=true;
    f.clutch_late=ClutchLatePayload{late_aim(),{},0};
    if(all) {
        NoUseDelayInput n{};
        n.in_game=true;
        f.no_use_delay=NoUseDelayTickPayload{n,{},0};
        f.anti_debuff=AntiDebuffTickPayload{};
        FastMineLateInput m{};
        m.in_game=true;
        f.fast_mine_late=FastMineLatePayload{m,{},0};
        f.bridge_late=BridgeAssistLatePayload{};
    }
    return f;
}
static void bridge_integration() {
    ModuleExecutionBridge bridge;
    const auto safe=bridge.add({"Safe Walk",1,{{0,0}},false});
    const auto place=bridge.add({"Fast Place",3,{{0,-1000}},false});
    const auto mine=bridge.add({"Fast Mine",3,{{0,0},{18,0}},false});
    const auto slow=bridge.add({"No Use Delay",4,{{18,0}},false});
    const auto anti=bridge.add({"Anti Debuff",2,{{18,0}},false});
    const auto assist=bridge.add({"Bridge Assist",3,
        {{5,0},{14,0},{17,0},{18,0}},false});
    const auto clutch=bridge.add({"Clutch",3,
        {{0,0},{13,-100},{18,-1000}},false});
    CHECK(bridge.bind_safe_walk_from_catalog() &&
          bridge.bind_fast_place_from_catalog() &&
          bridge.bind_fast_mine_from_catalog() &&
          bridge.bind_no_use_delay_from_catalog() &&
          bridge.bind_anti_debuff_from_catalog() &&
          bridge.bind_bridge_assist_from_catalog() &&
          bridge.bind_clutch_from_catalog());
    CHECK(!bridge.bind_clutch(clutch));
    const auto count=bridge.coverage();
    CHECK(count.executable_callback_bindings==13);
    CHECK(count.implemented_per_event[0]==4 &&
          count.implemented_per_event[13]==1 &&
          count.implemented_per_event[18]==5);
    for(auto id:{safe,place,mine,slow,anti,assist,clutch})
        CHECK(bridge.request(id,true));
    CHECK(!bridge.apply(false,false));
    CHECK(bridge.apply(true,true));
    auto f=late_frame(true);
    CHECK(bridge.dispatch(f));
    CHECK(f.clutch_late->executions==1 &&
          f.clutch_late->decision.planned);
    CHECK((f.executed_modules==std::vector<std::size_t>{
        mine,slow,anti,assist,clutch})); // priority -1000 last
    f=ExecutionFrame{};
    f.event=kClutchRotationFlagEvent;
    f.world_present=f.player_present=true;
    f.clutch_rotation=ClutchRotationPayload{};
    CHECK(bridge.dispatch(f) && f.clutch_rotation->executions==1 &&
          f.clutch_rotation->decision.override_yaw);
    f=click_frame(true);
    CHECK(bridge.dispatch(f) && f.clutch_click->executions==1 &&
          f.clutch_click->decision.proposed_use_click);
    CHECK((f.executed_modules==std::vector<std::size_t>{
        safe,mine,clutch,place})); // priorities 0, -1000
    f=click_frame(true);
    f.safe_walk->input.in_game=false;
    CHECK(!bridge.dispatch(f));
    f=late_frame(true);
    f.event=17;
    CHECK(!bridge.dispatch(f));
    f=late_frame(true);
    f.world_present=false;
    CHECK(!bridge.dispatch(f));
    f=late_frame(true);
    f.player_present=false;
    CHECK(!bridge.dispatch(f));
    CHECK(bridge.request(clutch,false));
    CHECK(bridge.apply(true,true));
    f=click_frame(false);
    CHECK(bridge.dispatch(f) && f.clutch_click->executions==0 &&
          !f.clutch_click->decision.proposed_use_click);
    bridge.reset_clutch_world_state();
    CHECK(bridge.request(clutch,true));
    CHECK(bridge.apply(true,true));
    f=click_frame(false);
    CHECK(bridge.dispatch(f) && !f.clutch_click->decision.proposed_use_click);
    bridge.stop();
    CHECK(!bridge.dispatch(f));
    for(const auto& invalid:std::vector<ModuleSpec>{
        {"Clutch",1,{{0,0},{13,-100},{18,-1000}},false},
        {"Clutch",3,{{0,1},{13,-100},{18,-1000}},false},
        {"Clutch",3,{{0,0},{13,-99},{18,-1000}},false},
        {"Clutch",3,{{0,0},{13,-100},{18,-999}},false},
        {"Clutch",3,{{0,0},{13,-100}},false},
        {"Clutch",3,{{0,0},{13,-100},{18,-1000},{5,0}},false},
        {"Clutch",3,{{0,0},{14,-100},{18,-1000}},false},
        {"Other",3,{{0,0},{13,-100},{18,-1000}},false}
    }) {
        ModuleExecutionBridge bad;
        const auto id=bad.add(invalid);
        CHECK(!bad.bind_clutch(id));
        CHECK(!bad.bind_clutch_from_catalog());
    }
    std::cout<<"R229_CLUTCH_SOURCE_CLICK_ROTATION_SNAPBACK_CALLBACKS_PASS "
                "click=0 rotation=13 late=18 priority=0,-100,-1000 "
                "worldCandidates=caller_provided gameWrites=0\n";
}
int main() {
    try {unit_policy();bridge_integration();return 0;}
    catch(const std::exception& e) {
        std::cerr<<e.what()<<"\n";return 1;
    }
}
