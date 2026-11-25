#include "slinkywinky/backtrack_controller.hpp"
#include "slinkywinky/lag_range_controller.hpp"
#include "slinkywinky/timer_range_controller.hpp"
#include "slinkywinky/inv_walk_controller.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace slinkywinky::reference_compat;
static void require(bool b,const char* why){if(!b)throw std::runtime_error(why);}
int main(){
try{
    BacktrackController back;
    BacktrackSettings bs{};
    bs.delay_ms=100;bs.cooldown_ticks=3;bs.min_range=1;bs.max_range=5;
    bs.max_target_hurt=5;bs.require_weapon=true;bs.disable_on_hit=true;
    bs.render_remote_pos=true;
    back.configure(bs);
    BacktrackSample f{};
    f.in_game=true;f.weapon=true;f.target=true;f.player_ticks=100;f.last_attack_tick=85;
    f.target_hurt=3;f.distance_sq=16;f.target_id=7;f.own_id=2;f.source_reach_eligible=true;
    back.tick(f);require(back.active()&&back.render_box(),"backtrack starts");
    BacktrackPacketSample bp{};
    bp.held_ms=100;bp.sequence=1;
    auto d=back.packet(bp);
    require(d.verdict==BacktrackPacketVerdict::hold&&d.sample_held_movement,
            "inclusive packet budget");
    require(!back.packet(bp).sample_held_movement,"no duplicate tracking");
    bp.held_ms=101;
    d=back.packet(bp);
    require(d.sample_released_movement,"release tracking");
    require(!back.packet(bp).sample_released_movement,"single release tracking");
    bp.kind=BacktrackPacketType::target_status;
    require(back.packet(bp).verdict==BacktrackPacketVerdict::pass_feedback,
            "target hurt feedback passes");
    bp.kind=BacktrackPacketType::teleport;
    require(back.packet(bp).stopped&&!back.active(),"teleport stops");
    back.tick(f);
    require(back.active(),"can reactivate without cooldown after teleport");
    f.real_position_minus_shown_distance=-0.051;
    back.tick(f);require(!back.active()&&back.cooldown()==3,"closer stop cooldown");
    f.real_position_minus_shown_distance=0;
    back.tick(f);require(!back.active()&&back.cooldown()==2,"cooldown two");
    back.tick(f);require(!back.active()&&back.cooldown()==1,"cooldown one");
    back.tick(f);require(back.active(),"final cooldown tick rechecks");
    f.target_id=10;back.tick(f);
    require(!back.active()&&back.cooldown()==3,"target change flush");
    back.reset_world();require(!back.active()&&back.cooldown()==0,"world teardown");
    bool failed=false;
    bs.max_range=-1;try{back.configure(bs);}catch(const std::invalid_argument&){failed=true;}
    require(failed,"reject invalid range");

    LagRangeController lag;
    LagRangeSettings ls{};ls.delay_ms=100;ls.flush_splash_potion=true;
    ls.flush_sprint_reset=true;ls.require_weapon=true;ls.render_remote_pos=true;
    lag.configure(ls);
    LagRangeTickSample lf{};
    lf.in_game=true;lf.weapon=true;lf.target=true;lf.sampled_max_range=12;
    lf.target_distance_sq=49;lf.sprinting=true;lf.target_server_position={10,0,0};
    lf.third_person=true;
    lag.tick(lf);require(lag.active(),"lag range starts");
    LagRangePacketSample lp{};
    lp.sequence=1;lp.kind=LagRangePacketType::movement;
    lp.movement_has_position=true;lp.movement_position={2,0,0};
    require(lag.packet(lp).hold,"movement towards target held");
    require(lag.moving_closer(),"last packet moves closer");
    lp.sequence=2;lp.held_ms=100;
    require(!lag.packet(lp).hold,"strict budget");
    lf.sprinting=false;lag.tick(lf);
    lp.kind=LagRangePacketType::stop_sprinting;lp.sequence=3;
    require(lag.packet(lp).released_on_sprint,"sprint release");
    lp.kind=LagRangePacketType::movement;lp.sequence=4;lp.movement_position={4,0,0};
    lp.held_ms=100;lag.packet(lp);
    lp.kind=LagRangePacketType::other;lp.sequence=5;lp.held_ms=0;
    require(lag.packet(lp).hold,"movement clears release latch");
    lag.attack_phase(true,true);
    lf.target_hurt=1;lag.tick(lf);
    require(lag.awaiting_attack_release(),"recent attack gates hold");
    lp.kind=LagRangePacketType::attack;lp.sequence=6;
    require(lag.packet(lp).released_on_attack,"attack releases");
    lf.splash_potion=true;lag.tick(lf);
    require(!lag.active()&&lag.pause_ticks()==15,"splash pause");
    lf.splash_potion=false;
    for(int i=0;i<15;++i)lag.tick(lf);
    require(!lag.active()&&lag.pause_ticks()==0,"pause drains");
    lag.tick(lf);require(lag.active(),"splash pause ended");
    lag.reset_world();require(!lag.active(),"lag world reset");
    failed=false;lf.sampled_max_range=20;
    try{lag.tick(lf);}catch(const std::invalid_argument&){failed=true;}
    require(failed,"range walk outside source bounds denied");

    TimerRangeController tr;
    TimerRangeSettings ts{};
    ts.mode=TimerRangeMode::freeze_before;
    ts.duration_legit_ticks=4;ts.cooldown_seconds=0.1f;ts.min_dist=2;
    tr.configure(ts);
    TimerRangeTick tf{};
    tf.in_game=true;tf.target=true;tf.player_on_ground=true;tf.target_distance_sq=9;
    tf.forward_input=1;tf.line_of_sight=true;
    require(!tr.tick(tf).burst_planned,"tick only arms ray plan");
    std::vector<TimerRangeRayStep> steps{{false,0},{true,5},{true,3}};
    auto tp=tr.frame(0.01f,0.0f,steps,true);
    require(tp.burst_planned&&tp.write_speed&&tp.speed==0.0f&&tp.hold_packets&&
            tr.burst_ticks()==2,"freeze before burst mode");
    require(tr.packet_hold(),"freeze holds packets");
    tp=tr.frame(0.01f,0,{},true);
    require(tp.write_speed&&tp.speed==0,"timer still frozen");
    tp=tr.frame(0.2f,0,{},true);
    require(tp.write_speed&&tp.speed==1&&tp.write_elapsed_ticks&&
            tp.elapsed_ticks==2&&!tr.packet_hold(),"unfreeze and catch-up");
    ts.mode=TimerRangeMode::freeze_after;ts.block_while_frozen=true;
    ts.cooldown_seconds=0;
    tr.configure(ts);tr.reset_world();
    tf.sword_held=true;
    tr.tick(tf);
    tp=tr.frame(0.01f,0,steps,true);
    require(tp.burst_planned&&tp.write_elapsed_ticks&&tp.elapsed_ticks==2&&
            !tp.write_speed,"freeze after starts by stepping ticks");
    tr.tick(tf);tp=tr.tick(tf);
    require(tp.write_speed&&tp.speed==0&&tp.write_use_key&&tp.use_key&&tr.frozen(),
            "freeze after attack window");
    tp=tr.disable();
    require(tp.write_speed&&tp.speed==1&&tp.write_use_key&&!tp.use_key&&!tr.packet_hold(),
            "timer restoration on disable");
    failed=false;ts.duration_ticks=std::numeric_limits<float>::infinity();
    try{tr.configure(ts);}catch(const std::invalid_argument&){failed=true;}
    require(failed,"invalid timer duration rejected");

    InvWalkController inv;
    InvWalkSettings is{};is.mode=InvWalkMode::silent;
    is.stop_on_click=InvWalkStop::always;is.stop_duration_ticks=3;
    inv.configure(is);
    require(inv.packet(InvWalkPacketType::open_inventory).action==InvWalkPacketAction::consume,
            "silent open is held");
    require(inv.phase()==InvWalkPhase::server_closed,"server closed state");
    auto ip=inv.packet(InvWalkPacketType::click_window);
    require(ip.send_open_inventory&&ip.action==InvWalkPacketAction::queue_after_server_open&&
            inv.queued()==1,"first click reopens on server then queues");
    for(int i=0;i<31;++i)
        require(inv.packet(InvWalkPacketType::click_window).action==InvWalkPacketAction::queue,
                "bounded queued clicks");
    require(inv.packet(InvWalkPacketType::click_window).action==
            InvWalkPacketAction::overflow_rejected && inv.queued()==32,
            "32 packet cap fail closed");
    InvWalkInput ii{};ii.in_game=true;ii.screen=InvWalkScreen::inventory;
    ii.physically_held={false,false,false,true,false};
    require(!inv.inventory_tick(true,true).drain_stale_keyboard_events,
            "replay guard delays flush");
    auto it=inv.tick(ii); // tick restores key ownership and clears replay phase
    require(it.update_keys&&!it.pressed[3]&&it.cancel_sprint,
            "moving keys suspended during queued clicks");
    require(inv.inventory_tick(true,true).drain_stale_keyboard_events&&
            inv.queued()==0,"flush after replay settled");
    for(int i=0;i<2;++i){
        it=inv.tick(ii);
        require(it.update_keys&&!it.pressed[3],"recent click suppresses keys");
    }
    it=inv.tick(ii);
    require(it.update_keys&&it.pressed[3]&&it.close_on_server,
            "keys restored after configured stop duration");
    ii.screen=InvWalkScreen::none;
    it=inv.tick(ii);
    require(it.update_keys&&it.pressed[3],"closed screen restores key once");
    require(!inv.tick(ii).update_keys,"no redundant restore");
    auto restore=inv.disable(ii.physically_held);
    require(restore.update_keys&&restore.pressed[3]&&inv.queued()==0,
            "disable restores keys and releases state");
    failed=false;is.stop_duration_ticks=-1;
    try{inv.configure(is);}catch(const std::invalid_argument&){failed=true;}
    require(failed,"bad inventory stop duration");

    std::cout<<"R329_FOUR_SOURCE_OFFLINE_CONTROLLER_TESTS_PASS\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
