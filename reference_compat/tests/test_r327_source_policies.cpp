#include "slinkywinky/blink_controller.hpp"
#include "slinkywinky/fake_lag_controller.hpp"
#include "slinkywinky/sprint_reset_controller.hpp"
#include "slinkywinky/knockback_delay_controller.hpp"
#include <cassert>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace slinkywinky::reference_compat;
static void check(bool ok, const char* msg) {
    if (!ok) throw std::runtime_error(msg);
}
int main() {
    try {
        BlinkController blink;
        BlinkSettings b{};
        b.max_duration=2; b.disable_on_attack=true;
        b.disable_on_block_dig=true; b.direction=BlinkDirection::both;
        blink.enable(b);
        check(blink.outgoing()==BlinkVerdict::hold,"blink outgoing");
        check(blink.incoming(BlinkPacketKind::other)==BlinkVerdict::hold,"blink incoming");
        check(blink.incoming(BlinkPacketKind::update_health)==BlinkVerdict::pass,"health bypass");
        check(blink.incoming(BlinkPacketKind::particles)==BlinkVerdict::pass,"particles bypass");
        check(blink.block_action(true,true) && !blink.block_action(false,true),"dig guard");
        check(!blink.tick(false,0) && blink.ticks_left()==2,"no in-game countdown");
        check(!blink.tick(true,1),"first countdown");
        check(blink.tick(true,1),"second countdown");
        blink.disable(); check(blink.outgoing()==BlinkVerdict::unchanged,"blink disable");
        b.direction=BlinkDirection::incoming;b.max_duration=5;blink.enable(b);
        check(blink.outgoing()==BlinkVerdict::unchanged,"incoming-only");
        check(blink.incoming(BlinkPacketKind::sound_effect)==BlinkVerdict::pass,"sound bypass");
        blink.reset_world();check(!blink.enabled(),"world cleanup");

        FakeLagController lag;
        FakeLagSettings ls{}; ls.inbound_ms=100;ls.outbound_ms=200;
        ls.require_attack=true;ls.require_weapon=true;
        lag.configure(ls);
        lag.tick({true,true,true,1});
        check(lag.inbound_budget()==0,"attack-gated baseline");
        lag.tick({true,true,true,2});
        check(lag.cooldown()==20 && lag.inbound_budget()==0,"attack deferred");
        lag.tick({true,true,true,2});
        check(lag.cooldown()==19 && lag.inbound_budget()==100,"attack window");
        auto packet=lag.packet(true,40);
        check(packet.verdict==LagVerdict::hold && packet.residual_budget_ms==0,"lag holds");
        packet=lag.packet(false,201);
        check(packet.verdict==LagVerdict::unchanged && packet.residual_budget_ms==1,"lag release");
        ls.realtime_damage=true;lag.configure(ls);
        check(lag.packet(true,0,true).verdict==LagVerdict::pass,"damage bypass");
        lag.tick({false,false,true,2});
        check(lag.inbound_budget()==0 && lag.cooldown()==0,"lost world resets budget");
        ls.mode=FakeLagMode::pulse;ls.require_attack=false;ls.require_weapon=false;
        lag.configure(ls);lag.reset_world();
        lag.tick({true,true,true,2});
        check(!lag.inbound_window(),"pulse starts closed");
        lag.tick({true,true,true,2});
        lag.tick({true,true,true,2});
        check(lag.inbound_window(),"pulse final tick opens");

        SprintResetController reset;
        SprintResetSettings rs{};rs.mode=2;rs.delay=1;rs.duration=2;
        rs.require_weapon=true;
        reset.configure(rs);
        SprintResetFrame sf{};sf.in_game=true;sf.seconds=1;sf.player_ticks=10;
        sf.last_attacker_time=10;sf.weapon=true;sf.physically_pressed=true;
        check(!reset.step(sf).change_key && reset.stage()==SprintResetStage::waiting,"hit schedules wait");
        sf.player_ticks=11; sf.seconds=1.05;
        auto key=reset.step(sf);
        check(key.change_key && !key.pressed && key.change_sprinting &&
              !key.sprinting,"sprint key released");
        key=reset.step(sf);
        check(key.change_key && !key.pressed,"repeat key hold");
        key=reset.step(sf);
        check(key.change_key && key.pressed && key.sprinting,"restore physical sprint");
        sf.in_game=false;
        check(!reset.step(sf).change_key,"already-restored world close");
        rs.mode=1;rs.delay=0;rs.duration=3;reset.configure(rs);
        sf.in_game=true;sf.player_ticks=50;sf.last_attacker_time=50;sf.seconds=3;
        check(!reset.step(sf).change_key,"second reset begins");
        sf.player_ticks=51;key=reset.step(sf);
        check(key.change_key && key.pressed && !key.change_sprinting,"sneak press");
        sf.in_game=false;key=reset.step(sf);
        check(key.change_key && key.pressed && !key.change_sprinting,"screen loss restores physical key");

        KnockbackDelayController kb;
        KnockbackDelaySettings ks{};ks.max_delay_ms=100;ks.range=5;
        ks.require_weapon=true;
        kb.configure(ks);
        KnockbackDelayFrame kf{};kf.player=true;kf.own_entity_id=77;
        kf.has_combat_target=true;kf.combat_distance_sq=20;
        kf.weapon=true;kf.random_chance_passed=true;
        kb.late_tick(kf);
        check(kb.allowed(),"knockback conditions");
        KnockbackDelayPacket kp{};kp.is_velocity=true;
        kp.velocity_entity_id=77;kp.transaction_id=42;
        check(kb.packet(kp)==KnockbackDelayVerdict::hold,"knockback holds own velocity");
        kp.is_velocity=false;kp.held_ms=50;kp.hurt_feedback=true;
        check(kb.packet(kp)==KnockbackDelayVerdict::hold_feedback,"feedback code");
        kp.held_ms=101;
        check(kb.packet(kp)==KnockbackDelayVerdict::unchanged && !kb.holding(),
              "knockback expiry");
        kp.is_velocity=true;kp.held_ms=0;kp.hurt_feedback=false;
        check(kb.packet(kp)==KnockbackDelayVerdict::unchanged,"expiry clears admission");
        kb.late_tick(kf);check(kb.packet(kp)==KnockbackDelayVerdict::hold,"readmit");
        kp.transaction_id=43;
        check(kb.packet(kp)==KnockbackDelayVerdict::unchanged && !kb.holding(),
              "cross-batch denial");
        kb.reset_world();check(!kb.allowed() && !kb.holding(),"world cleanup");
        ks.require_air=true;kb.configure(ks);kf.on_ground=false;kb.late_tick(kf);
        check(kb.packet({true,false,0,0,9,false})==KnockbackDelayVerdict::hold,
              "air admission first packet");
        check(kb.packet({true,false,0,10,9,false})==KnockbackDelayVerdict::unchanged,
              "air budget edge");
        bool rejected=false;
        try { ls.inbound_ms=std::numeric_limits<float>::quiet_NaN();lag.configure(ls); }
        catch (const std::invalid_argument&) {rejected=true;}
        check(rejected,"bad lag setting rejected");
        rejected=false;
        try { b.max_duration=-1;blink.enable(b); }
        catch (const std::invalid_argument&) {rejected=true;}
        check(rejected,"bad blink setting rejected");
        std::cout<<"R327_FOUR_SOURCE_POLICY_CONTROLLERS_PASS\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
