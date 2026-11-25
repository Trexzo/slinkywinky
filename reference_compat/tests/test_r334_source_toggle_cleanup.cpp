#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
using namespace slinkywinky::reference_compat;
static void need(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
static ExecutionFrame f(unsigned e){
    ExecutionFrame x{};x.event=e;x.world_present=x.player_present=true;return x;
}
int main(){try{
    ModuleExecutionBridge b;
    const auto tr=b.add({"Timer Range",0,{{0,-100},{1,0},{3,0},{5,0}},false});
    const auto iw=b.add({"Inv Walk",1,{{5,0},{17,0},{23,0},{25,0}},false});
    need(!b.bind_timer_range_source_toggle(tr),"timer toggle requires tick binding");
    need(!b.bind_inv_walk_source_toggle(iw),"inventory toggle requires tick binding");
    need(b.bind_timer_range_source_tick(tr)&&b.bind_inv_walk_source_tick(iw),
         "bind exact native tick owners");
    need(b.bind_timer_range_source_toggle(tr)&&b.bind_inv_walk_source_toggle(iw),
         "register exact e5 source callbacks");
    need(!b.bind_timer_range_source_toggle(tr),"duplicate e5 binding refused");
    need(b.implemented_event_bindings()==4,"four callback entries bound");
    need(b.request(tr,true)&&b.request(iw,true)&&b.apply(true,true),
         "source owners enabled");
    need(!b.take_timer_range_source_cleanup() &&
         !b.take_inv_walk_source_cleanup(),
         "enabling does not trigger disable cleanup");

    auto tick=f(0);
    TimerRangeSourceTickPayload p{};
    p.settings.mode=TimerRangeMode::freeze_before;
    p.settings.duration_legit_ticks=3;
    p.settings.cooldown_seconds=0;
    p.input.in_game=true;p.input.target=true;p.input.target_distance_sq=9;
    p.input.player_on_ground=true;p.input.line_of_sight=true;
    p.input.forward_input=1;
    tick.timer_range_source_tick=p;
    need(b.dispatch(tick),"timer tick eligible");
    auto frame=f(3);
    TimerRangeSourceFramePayload tp{};
    tp.ray_steps={{true,9}};
    frame.timer_range_source_frame=tp;
    need(b.bind_timer_range_source_frame(tr),"bind source timer frame");
    need(b.dispatch(frame)&&frame.timer_range_source_frame->decision.write_speed &&
         frame.timer_range_source_frame->decision.speed==0,
         "source timer is now frozen");

    auto inv_tick=f(25);
    InvWalkSourceTickPayload iv{};
    iv.settings.mode=InvWalkMode::silent;
    iv.input.in_game=true;iv.input.screen=InvWalkScreen::inventory;
    iv.input.physically_held={false,false,false,true,false};
    inv_tick.inv_walk_source_tick=iv;
    need(b.dispatch(inv_tick),"inventory silent tick");
    auto pkt=f(23);pkt.inv_walk_source_packet=InvWalkSourcePacketPayload{};
    need(b.bind_inv_walk_source_packet(iw),"bind inventory packet");
    pkt.inv_walk_source_packet->kind=InvWalkPacketType::open_inventory;
    need(b.dispatch(pkt)&&pkt.inv_walk_source_packet->decision.action==
         InvWalkPacketAction::consume,"inventory status held");
    pkt.inv_walk_source_packet->kind=InvWalkPacketType::click_window;
    need(b.dispatch(pkt)&&pkt.inv_walk_source_packet->decision.send_open_inventory,
         "queued first click awaiting confirm");

    // Artificial event5 frames must NOT invoke the real apply() lifecycle.
    auto forged=f(5);forged.notification_target=tr;
    forged.notification_new_state=false;
    need(b.dispatch(forged),"synthetic e5 is observation only");
    need(!b.take_timer_range_source_cleanup() &&
         !b.take_inv_walk_source_cleanup(),
         "synthetic e5 does not clear live controller state");

    need(b.request(tr,false)&&b.request(iw,false),
         "two module disables queued in one source batch");
    need(b.apply(true,true),"source native disable notifications replayed");
    auto tr_receipt=b.take_timer_range_source_cleanup();
    auto inv_receipt=b.take_inv_walk_source_cleanup();
    need(tr_receipt&&tr_receipt->write_speed&&tr_receipt->speed==1,
         "timer source e5 restore captured from real disable notification");
    need(inv_receipt&&inv_receipt->queued_clicks_before_reset==1&&
         inv_receipt->requires_external_packet_reconciliation,
         "inventory source e5 retained packet ownership reconciliation receipt");
    need(!b.take_timer_range_source_cleanup() &&
         !b.take_inv_walk_source_cleanup(),
         "cleanup receipts single-consumer");
    need(b.request(tr,true)&&b.request(iw,true)&&b.apply(true,true),
         "both original source owners re-enabled");
    auto timer_packet=f(1);
    timer_packet.timer_range_source_packet=TimerRangeSourcePacketPayload{};
    need(b.bind_timer_range_source_packet(tr)&&b.dispatch(timer_packet)&&
         !timer_packet.timer_range_source_packet->hold_packets,
         "freeze packet holding cleared after disable");
    need(b.dispatch(pkt)&&pkt.inv_walk_source_packet->decision.action==
         InvWalkPacketAction::pass,
         "prior inventory click queue cannot leak into new world");

    // Stop also supplies restoration for a currently frozen source module.
    need(b.dispatch(tick)&&b.dispatch(frame),"source timer rearmed");
    b.stop();
    tr_receipt=b.take_timer_range_source_cleanup();
    need(tr_receipt&&tr_receipt->write_speed&&tr_receipt->speed==1,
         "forced bridge stop exports the final timer reset receipt");
    need(!b.dispatch(tick),"no replay after stop");
    std::cout<<"R334_SOURCE_NATIVE_TOGGLE_CLEANUP_PASS\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
