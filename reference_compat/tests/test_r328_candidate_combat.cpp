#include "slinkywinky/criticals_controller.hpp"
#include "slinkywinky/knockback_displace_controller.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace slinkywinky::reference_compat;
static void check(bool v,const char* why){if(!v)throw std::runtime_error(why);}
int main(){
 try{
   CriticalsController c;
   CriticalsSettings cs{};cs.mode=CriticalsMode::packet;
   cs.max_packet_delay_ms=100;cs.require_mouse=true;cs.require_weapon=true;
   c.configure(cs);
   CriticalsTickFrame tf{};tf.in_game=true;tf.mouse=true;tf.weapon=true;
   tf.player_ticks=100;tf.last_attacker_tick=99;tf.sampled_chance_roll=true;
   c.tick(tf);check(c.packet_armed() && c.roll_cooldown()==9,"packet mode chance");
   CriticalsPacketFrame pf{};pf.player_packet=true;pf.positional=true;
   pf.sequence=1;pf.y=50;pf.on_ground=true;
   check(!c.packet(pf),"ground movement");
   pf.sequence=2;pf.y=49.8;pf.on_ground=false;
   check(c.packet(pf) && c.packet_hold_budget()==20,"falling incremental hold");
   pf.sequence=3;pf.y=49.6;
   check(c.packet(pf) && c.packet_hold_budget()==40,"incremental budget");
   pf.sequence=3;pf.y=40;pf.held_ms=45;
   check(!c.packet(pf) && c.packet_hold_budget()==40,"duplicate sequence denied");
   pf.sequence=4;pf.y=49.2;pf.held_ms=50;check(c.packet(pf),"budget third");
   tf.mouse=false;c.tick(tf);
   check(!c.packet_armed(),"mouse requirement");
   pf.sequence=5;pf.y=49;pf.held_ms=1;
   check(!c.packet(pf) && c.packet_hold_budget()==0,"disabled packet holds clear");

   c.reset_world();
   cs.mode=CriticalsMode::timer;cs.timer_speed=0.5f;cs.boost_ticks=3;
   c.configure(cs);
   tf.mouse=true;tf.on_ground=false;tf.motion_y=-0.1;
   tf.entity_under_crosshair=true;tf.hurt_time=0;
   c.tick(tf);check(c.timer_armed() && !c.packet_armed(),"timer armed");
   auto speed=c.frame(0.2);
   check(speed.update_speed && speed.speed==0.5f && 
         std::fabs(c.boost_debt()-2.0)<1e-6,"timer accumulates");
   speed=c.frame(0.2);check(c.boost_debt()==3.0,"timer clamp");
   tf.on_ground=true;c.tick(tf);
   speed=c.frame(0.05);check(speed.update_speed && speed.speed==1.0f &&
       speed.add_elapsed_ticks==3,"restore and catch up");
   check(!c.disable().update_speed,"disable idempotent");
   c.reset_world();
   tf.on_ground=false;tf.sampled_chance_roll=false;
   c.tick(tf);speed=c.frame(0.05);
   check(!speed.update_speed,"chance roll rejects");
   bool rejected=false;
   try {cs.timer_speed=std::numeric_limits<float>::quiet_NaN();c.configure(cs);}
   catch(const std::invalid_argument&){rejected=true;}
   check(rejected,"invalid timer rejected");

   KnockbackDisplaceController d;
   KnockbackDisplaceSettings ds{};
   ds.direction=3;ds.hide_rotation=true;ds.angle_degrees=90;
   ds.require_weapon=true;ds.use_last_strafe=true;
   d.configure(ds);
   KnockbackDisplaceAttack a{};
   a.phase_pre_attack=true;a.selected_enemy_player=true;
   a.accepted_weapon=true;a.sprinting=true;a.rotation_priority_available=true;
   a.yaw=45;a.pitch=10;a.sampled_angle_reduction=5;
   a.sampled_pitch_magnitude=12;a.sampled_pitch_down=true;
   check(!d.attack(a).cancel_attack,"no hit during initial reset gate");
   for(int i=0;i<10;++i)d.late_tick(true,1.0f,true);
   auto p=d.attack(a);
   check(p.cancel_attack && p.apply_rotation && p.hide_packets &&
     p.yaw_offset==85 && p.pitch_offset==12 && d.packet_hold(),
     "displace after cooldown with sampled angle");
   check(d.attack(a).cancel_attack,"double attack blocked");
   d.post_attack(true);check(d.reset_ticks()==0,"post hit reset");
   auto late=d.late_tick(true,0,true);
   check(late.restore_rotation && late.replay_one_click &&
       late.target_yaw==45 && late.target_pitch==10 && late.packet_hold,
       "late tick key replay and restored facing");
   check(d.saved_strafe_sign()==1,"last strafe retained");
   check(!d.attack(a).cancel_attack,"post displacement gate");
   for(int i=0;i<10;++i)d.late_tick(true,-1,true);
   p=d.attack(a);
   check(p.apply_rotation && p.yaw_offset==-85,"reversed strafe direction");
   d.reset_world();check(!d.packet_hold() && !d.displacing(),"world release");
   ds.direction=2;d.configure(ds);
   for(int i=0;i<10;++i)d.late_tick(true,0,true);
   a.sampled_random_direction_positive=false;
   check(d.attack(a).yaw_offset==-85,"explicit random direction sample");
   rejected=false;
   try {ds.angle_degrees=std::numeric_limits<float>::infinity();d.configure(ds);}
   catch(const std::invalid_argument&){rejected=true;}
   check(rejected,"invalid angle rejected");

   std::cout<<"R328_TWO_CANDIDATE_COMBAT_CONTROLLERS_PASS\n";
   return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
