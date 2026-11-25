#include "slinkywinky/module_execution_bridge.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace slinkywinky::reference_compat;
static void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
static ExecutionFrame ev(unsigned e){
    ExecutionFrame f{};f.event=e;f.world_present=f.player_present=true;return f;
}
int main(){try{
    TimerRangeSourceObservation o{};
    o.player_eye={0,1.62,0};
    o.target_client_pos={0,0,2.8};
    o.target_server_pos=o.target_client_pos;
    o.forward_input=1;o.movement_factor=0.1;
    TimerRangeSettings settings{};
    settings.mode=TimerRangeMode::freeze_after;settings.duration_ticks=6;settings.min_dist=2;
    auto steps=source_timer_range_predict(o,settings);
    check(steps.size()==1&&steps[0].hitbox_intersects&&
          std::abs(steps[0].nearest_distance_sq-5.76)<1e-8,
          "source ray hit within three blocks and exact nearest AABB distance");
    o.target_client_pos={20,0,2.8};
    o.target_server_pos=o.target_client_pos;
    steps=source_timer_range_predict(o,settings);
    check(steps.size()==6&&!steps[0].hitbox_intersects,
          "sideways out-of-range target not ray visible");
    settings.mode=TimerRangeMode::freeze_before;
    settings.duration_legit_ticks=2;
    o.target_client_pos={0,0,2.8};
    o.target_server_pos=o.target_client_pos;
    steps=source_timer_range_predict(o,settings);
    check(steps.size()==1&&steps[0].hitbox_intersects,
          "freeze-before source target interpolation supported");
    auto broken=o;broken.player_eye.x=std::nan("");
    bool threw=false;
    try{source_timer_range_predict(broken,settings);}
    catch(const std::invalid_argument&){threw=true;}
    check(threw,"invalid source snapshot denied");

    SourceBoxObservation geom{};
    geom.previous={2,4,6};geom.current={4,8,10};
    geom.camera={1,1,1};geom.partial_ticks=0.5f;
    geom.yaw_previous=30;geom.yaw_current=50;
    SourceBoxStyle style{};
    style.color={0.5f,0.25f,0.75f};style.line_width=2;
    style.rotate=true;style.filled=true;
    auto box=source_box_preview(true,geom,style,true);
    check(box.draw&&box.rotate&&box.filled&&box.fill_alpha==0.4f&&
          box.rotation_y_degrees==140&&box.translate.x==2&&
          box.translate.y==6&&box.translate.z==7&&
          box.scale.x==0.3&&box.scale.y==1&&box.scale.z==0.3,
          "Backtrack box interpolation, yaw, scale and fill");
    box=source_box_preview(true,geom,style,false);
    check(box.draw&&!box.rotate,"Lag Range omits target yaw rotation");
    box=source_box_preview(false,geom,style,true);
    check(!box.draw,"invisible source box does not render");
    style.color[1]=-0.2f;
    threw=false;
    try{source_box_preview(true,geom,style,true);}
    catch(const std::invalid_argument&){threw=true;}
    check(threw,"untrusted GL style fails closed");
    style.color[1]=0.2f;

    ModuleExecutionBridge b;
    const auto bk=b.add({"Backtrack",0,{{2,0},{6,0},{18,0}},false});
    const auto lg=b.add({"Lag Range",0,{{3,-100},{6,0},{15,0},{18,0}},false});
    const auto tr=b.add({"Timer Range",0,{{0,-100},{1,0},{3,0},{5,0}},false});
    check(!b.bind_backtrack_source_render(bk),"render requires tick owner");
    check(b.bind_backtrack_source_tick(bk)&&b.bind_lag_range_source_tick(lg)&&
          b.bind_timer_range_source_tick(tr),"bind controller owners");
    check(b.bind_backtrack_source_render(bk)&&
          b.bind_lag_range_source_render(lg)&&
          b.bind_lag_range_source_attack(lg)&&
          b.bind_timer_range_source_frame(tr),"three graphics/attack + frame");
    for(auto id:{bk,lg,tr})check(b.request(id,true),"enable");
    check(b.apply(true,true),"apply enable");
    auto t=ev(18);
    BacktrackSourceTickPayload p{};
    p.settings.render_remote_pos=true;
    p.input.in_game=true;p.input.target=true;p.input.distance_sq=9;
    p.input.player_ticks=21;p.input.last_attack_tick=20;
    p.input.target_id=7;p.input.source_reach_eligible=true;
    t.backtrack_source_tick=p;
    LagRangeSourceTickPayload l{};
    l.settings.render_remote_pos=true;
    l.input.in_game=true;l.input.target=true;
    l.input.third_person=true;l.input.target_distance_sq=9;
    l.input.sampled_max_range=11;
    l.input.target_server_position={3,0,0};
    t.lag_range_source_tick=l;
    check(b.dispatch(t),"both event18 controllers");
    // Lag box requires a previous movement packet that advances closer.
    // Test independently that the combined event6 render callback executes.
    auto f=ev(6);
    BacktrackSourceRenderPayload x{};x.observation=geom;x.style=style;
    LagRangeSourceRenderPayload y{};y.observation=geom;y.style=style;
    f.backtrack_source_render=x;f.lag_range_source_render=y;
    check(b.dispatch(f)&&f.backtrack_source_render->executions==1&&
          f.lag_range_source_render->executions==1&&
          f.backtrack_source_render->decision.draw,
          "native-source event6 renders both modules in one event");
    check(!f.lag_range_source_render->decision.draw,
          "lag range box must be gated without closer movement");
    auto bad=f;
    bad.lag_range_source_render->observation.camera.x=12;
    check(!b.dispatch(bad),"mixed render camera snapshots fail closed");

    auto attack=ev(15);
    attack.lag_range_source_attack=LagRangeSourceAttackPayload{true,true,0};
    check(b.dispatch(attack)&&attack.lag_range_source_attack->executions==1,
          "source Lag Range attack event15 accepted");
    t={};t=ev(18);t.lag_range_source_tick=l;
    check(b.dispatch(t),"lag tick after same-tick attack");
    // Timer Range's frame can now derive ray evidence from real copied
    // movement/target/rotation instead of caller asserting intersection.
    auto tick=ev(0);
    TimerRangeSourceTickPayload tt{};
    tt.settings.mode=TimerRangeMode::freeze_after;
    tt.settings.duration_ticks=6;
    tt.settings.min_dist=2;
    tt.input.in_game=true;tt.input.target=true;tt.input.target_distance_sq=9;
    tt.input.player_on_ground=true;tt.input.line_of_sight=true;
    tt.input.forward_input=1;
    tick.timer_range_source_tick=tt;
    check(b.dispatch(tick),"timer tick eligible");
    auto frame=ev(3);
    TimerRangeSourceFramePayload tf{};
    tf.source_snapshot=TimerRangeSourceObservation{};
    tf.source_snapshot->player_eye={0,1.62,0};
    tf.source_snapshot->target_client_pos={0,0,2.8};
    tf.source_snapshot->target_server_pos={0,0,2.8};
    tf.source_snapshot->forward_input=1;
    frame.timer_range_source_frame=tf;
    check(b.dispatch(frame)&&frame.timer_range_source_frame->executions==1&&
          frame.timer_range_source_frame->decision.burst_planned&&
          frame.timer_range_source_frame->decision.write_elapsed_ticks,
          "source Timer Range frame plans burst from derived AABB ray");
    frame.timer_range_source_frame->ray_steps={{true,9}};
    threw=false;try{b.dispatch(frame);}catch(const std::invalid_argument&){threw=true;}
    check(threw,"reject two incompatible timer source evidence inputs");
    b.stop();
    check(!b.dispatch(frame),"source replay stop gating");
    std::cout<<"R333_SOURCE_GEOMETRY_PREDICTION_BRIDGE_PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
