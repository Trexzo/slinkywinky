#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace slinkywinky::reference_compat;
static void ensure(bool v,const char* s){if(!v)throw std::runtime_error(s);}
static ExecutionFrame ev(unsigned slot){
    ExecutionFrame f{};
    f.event=slot;f.world_present=f.player_present=true;
    return f;
}
int main(){try{
    ModuleExecutionBridge b;
    const auto bk=b.add({"Backtrack",0,{{2,0},{6,0},{18,0}},false});
    const auto lg=b.add({"Lag Range",0,{{3,-100},{6,0},{15,0},{18,0}},false});
    const auto tr=b.add({"Timer Range",0,{{0,-100},{1,0},{3,0},{5,0}},false});
    const auto iw=b.add({"Inv Walk",1,{{5,0},{17,0},{23,0},{25,0}},false});
    ensure(!b.bind_backtrack_source_packet(bk),"must bind tick before packet");
    ensure(!b.bind_lag_range_source_packet(lg),"packet requires tick");
    ensure(!b.bind_timer_range_source_frame(tr),"frame requires tick");
    ensure(!b.bind_inv_walk_source_flush(iw),"flush requires tick");
    ensure(b.bind_backtrack_source_tick(bk)&&b.bind_lag_range_source_tick(lg)&&
           b.bind_timer_range_source_tick(tr)&&b.bind_inv_walk_source_tick(iw),
           "four exact tick subscriptions");
    ensure(b.bind_backtrack_source_packet(bk)&&b.bind_lag_range_source_packet(lg)&&
           b.bind_timer_range_source_packet(tr)&&b.bind_timer_range_source_frame(tr)&&
           b.bind_inv_walk_source_packet(iw)&&b.bind_inv_walk_source_flush(iw),
           "six additional source callback subscriptions");
    ensure(!b.bind_backtrack_source_packet(bk),"duplicate packet bind denied");
    ensure(b.implemented_event_bindings()==10,"10 actual callbacks bound");
    for(auto id:{bk,lg,tr,iw})ensure(b.request(id,true),"request enable");
    ensure(b.apply(true,true),"apply enable");

    auto f=ev(2);
    f.backtrack_source_packet=BacktrackSourcePacketPayload{};
    f.backtrack_source_packet->input.sequence=7;
    f.backtrack_source_packet->input.held_ms=20;
    ensure(b.dispatch(f),"incoming packet before tick");
    ensure(f.backtrack_source_packet->decision.verdict==BacktrackPacketVerdict::unchanged,
          "packet cannot hold before eligible source tick");
    auto tick=ev(18);
    BacktrackSourceTickPayload bt{};
    bt.settings.delay_ms=120;bt.settings.max_range=6;bt.settings.disable_on_hit=true;
    bt.input.in_game=true;bt.input.target=true;bt.input.distance_sq=9;
    bt.input.player_ticks=100;bt.input.last_attack_tick=90;bt.input.target_id=21;
    bt.input.own_id=42;bt.input.source_reach_eligible=true;
    tick.backtrack_source_tick=bt;
    ensure(b.dispatch(tick)&&tick.backtrack_source_tick->active,"backtrack tick arm");
    ensure(b.dispatch(f) &&
           f.backtrack_source_packet->decision.verdict==BacktrackPacketVerdict::hold,
           "e2 incoming packet holds within delay budget");
    f.backtrack_source_packet->input.kind=BacktrackPacketType::target_status;
    f.backtrack_source_packet->input.entity_id=21;
    ensure(b.dispatch(f) &&
           f.backtrack_source_packet->decision.verdict==BacktrackPacketVerdict::pass_feedback,
           "matching target status bypasses buffer");
    f.backtrack_source_packet->input.entity_id=22;
    ensure(b.dispatch(f) &&
           f.backtrack_source_packet->decision.verdict==BacktrackPacketVerdict::hold,
           "other entity status does not bypass");
    f.backtrack_source_packet->input.kind=BacktrackPacketType::entity_velocity;
    f.backtrack_source_packet->input.entity_id=42;
    ensure(b.dispatch(f)&&f.backtrack_source_packet->decision.stopped,
           "local velocity triggers disable-on-hit");
    f.backtrack_source_packet->input.kind=BacktrackPacketType::other;
    ensure(b.dispatch(f)&&f.backtrack_source_packet->decision.verdict==
           BacktrackPacketVerdict::unchanged,"stop no longer holds incoming packets");

    LagRangeSourceTickPayload lt{};
    lt.input.in_game=true;lt.input.target=true;
    lt.input.sampled_max_range=11;lt.input.target_distance_sq=9;
    lt.input.target_server_position={10,0,0};
    tick={};tick=ev(18);tick.lag_range_source_tick=lt;
    ensure(b.dispatch(tick)&&tick.lag_range_source_tick->active,"lag tick arm");
    f=ev(3);f.lag_range_source_packet=LagRangeSourcePacketPayload{};
    f.lag_range_source_packet->input.kind=LagRangePacketType::movement;
    f.lag_range_source_packet->input.movement_has_position=true;
    f.lag_range_source_packet->input.movement_position={5,0,0};
    f.lag_range_source_packet->input.sequence=1;
    f.lag_range_source_packet->input.held_ms=10;
    ensure(b.dispatch(f)&&f.lag_range_source_packet->decision.hold,
          "outbound movement packet held as we approach target");
    f.lag_range_source_packet->input.held_ms=150;
    f.lag_range_source_packet->input.sequence=2;
    ensure(b.dispatch(f)&&!f.lag_range_source_packet->decision.hold &&
          f.lag_range_source_packet->decision.position_recorded,
          "outbound packet released at budget boundary");

    f=ev(1);f.timer_range_source_packet=TimerRangeSourcePacketPayload{};
    ensure(b.dispatch(f)&&!f.timer_range_source_packet->hold_packets,
          "timer source starts with no packet hold");
    auto t=ev(0);
    TimerRangeSourceTickPayload tt{};
    tt.settings.mode=TimerRangeMode::freeze_before;
    tt.settings.duration_legit_ticks=3;tt.settings.cooldown_seconds=0;
    tt.input.in_game=true;tt.input.target=true;tt.input.target_distance_sq=16;
    tt.input.player_on_ground=true;tt.input.line_of_sight=true;
    tt.input.forward_input=1;
    t.timer_range_source_tick=tt;
    ensure(b.dispatch(t),"timer tick eligible");
    auto frame=ev(3);
    TimerRangeSourceFramePayload tf{};
    tf.ray_steps={{true,9}};
    frame.timer_range_source_frame=tf;
    ensure(b.dispatch(frame)&&frame.timer_range_source_frame->decision.burst_planned &&
           frame.timer_range_source_frame->decision.write_speed &&
           frame.timer_range_source_frame->decision.speed==0,
           "timer frame begins freeze-before burst");
    ensure(b.dispatch(f)&&f.timer_range_source_packet->hold_packets,
           "outbound timer packet observes freeze hold");
    ensure(b.request(tr,false)&&b.apply(true,true),"timer disable applied");
    ensure(b.request(tr,true)&&b.apply(true,true),"timer enable applied");
    ensure(b.dispatch(f)&&!f.timer_range_source_packet->hold_packets,
           "timer packet hold does not survive disable");

    auto pkt=ev(23);
    InvWalkSourcePacketPayload ip{};
    ip.kind=InvWalkPacketType::open_inventory;
    pkt.inv_walk_source_packet=ip;
    ensure(b.dispatch(pkt) &&
           pkt.inv_walk_source_packet->decision.action==InvWalkPacketAction::pass,
           "inventory defaults vanilla before settings tick");
    auto it=ev(25);
    InvWalkSourceTickPayload iv{};
    iv.settings.mode=InvWalkMode::silent;
    iv.input.in_game=true;iv.input.screen=InvWalkScreen::inventory;
    iv.input.physically_held={false,false,false,true,false};
    it.inv_walk_source_tick=iv;
    ensure(b.dispatch(it),"inventory enters silent tick");
    ensure(b.dispatch(pkt)&&
           pkt.inv_walk_source_packet->decision.action==InvWalkPacketAction::consume,
           "silent e23 consumes open-inventory packet");
    pkt.inv_walk_source_packet->kind=InvWalkPacketType::click_window;
    ensure(b.dispatch(pkt)&&pkt.inv_walk_source_packet->decision.send_open_inventory &&
           pkt.inv_walk_source_packet->decision.action==
              InvWalkPacketAction::queue_after_server_open,
           "first click queued behind open");
    auto flush=ev(17);
    InvWalkSourceFlushPayload fl{};
    fl.flush_confirmed=true;fl.in_game=true;flush.inv_walk_source_flush=fl;
    ensure(b.dispatch(flush)&&!flush.inv_walk_source_flush->decision.drain_stale_keyboard_events,
           "cannot flush while replaying");
    ensure(b.dispatch(it),"advance inventory replay state");
    ensure(b.dispatch(flush) &&
           flush.inv_walk_source_flush->decision.drain_stale_keyboard_events,
           "confirmed flush clears queued clicks");
    ensure(b.request(iw,false)&&b.apply(true,true),"inventory disable");
    ensure(b.request(iw,true)&&b.apply(true,true),"inventory re-enable");
    pkt.inv_walk_source_packet->kind=InvWalkPacketType::click_window;
    ensure(b.dispatch(pkt)&&pkt.inv_walk_source_packet->decision.action==
           InvWalkPacketAction::pass,"no stale queued refs after disable");

    auto wrong=ev(17);wrong.backtrack_source_packet={};
    ensure(!b.dispatch(wrong),"never admit e2 packet typed on e17");
    wrong=ev(3);wrong.lag_range_source_packet={};wrong.timer_range_source_frame={};
    ensure(!b.dispatch(wrong),"no mixed representations of event3 without identity proof");
    b.stop();
    ensure(!b.dispatch(pkt),"stopped bridge never replays owned packet state");
    std::cout<<"R332_SOURCE_PACKET_FRAME_FLUSH_BRIDGE_PASS\n";return 0;
}catch(const std::exception& x){std::cerr<<x.what()<<'\n';return 1;}}
