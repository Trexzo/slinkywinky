#include "slinkywinky/combat.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace slinkywinky;
#define CHECK(c) do { if(!(c)) throw std::runtime_error("FAIL line "+std::to_string(__LINE__)+": " #c); } while(false)
int main(){
    CombatCoordinator c({1,2,3,4});
    CombatSettings settings;
    TickSnapshot s;
    s.world_valid=true; s.generation=1; s.holding_weapon=true; s.mouse_left=true;
    s.elapsed_ms=100; settings.auto_clicker_enabled=true;
    auto a=c.tick(s,settings); CHECK(a.attempt_attack); CHECK(a.left_held); CHECK(a.left_edge==KeyEdge::press);
    s.elapsed_ms=110; a=c.tick(s,settings); CHECK(!a.attempt_attack); CHECK(a.left_edge==KeyEdge::none);
    s.elapsed_ms=130; a=c.tick(s,settings); CHECK(a.left_held); // manual hold remains
    s.gui_open=true; a=c.tick(s,settings); CHECK(!a.attempt_attack); CHECK(a.left_held); // manual remains
    s.mouse_left=false; a=c.tick(s,settings); CHECK(a.left_edge==KeyEdge::release);
    s.gui_open=false;
    s.target=Target{5,{2,0,0},true,true}; settings.aim_enabled=true;settings.aim_range=4;
    s.elapsed_ms=160; a=c.tick(s,settings);
    CHECK(a.rotation.has_value());
    CHECK(a.rotation->priority==100);
    CHECK(std::abs(a.rotation->yaw+13.5f)<0.01f); // target yaw -90 * 0.15
    s.target->visible=false; s.elapsed_ms=180; a=c.tick(s,settings); CHECK(!a.rotation.has_value());
    s.target->visible=true; settings.auto_block_enabled=true; settings.block_requires_damage=false;
    s.elapsed_ms=200; a=c.tick(s,settings); CHECK(a.right_edge==KeyEdge::press);
    s.elapsed_ms=220; settings.auto_block_enabled=false; a=c.tick(s,settings); CHECK(a.right_edge==KeyEdge::release);
    settings.hit_select_enabled=true; settings.hit_select_min_hurt=3;
    s.elapsed_ms=400; a=c.tick(s,settings); CHECK(!a.attempt_attack);
    settings.hit_select_enabled=false; s.elapsed_ms=410;
    s.mouse_left=false; a=c.tick(s,settings);
    settings.require_mouse_down=false;
    s.elapsed_ms=500; a=c.tick(s,settings); CHECK(a.attempt_attack);
    s.generation=2; s.elapsed_ms=520; a=c.tick(s,settings); CHECK(!a.attempt_attack);
    s.elapsed_ms=550; a=c.tick(s,settings); CHECK(a.attempt_attack);
    s.world_valid=false; a=c.tick(s,settings); CHECK(a.left_edge==KeyEdge::release);
    a=c.disable(false,false); CHECK(!a.left_held && !a.right_held);
    VelocitySettings v;v.enabled=true;v.horizontal=.4f;v.vertical=.8f;
    const auto out=decide_velocity({10,5,-20},v,.5f);
    CHECK(out.has_value() && out->x==4 && out->y==4 && out->z==-8);
    CHECK(!decide_velocity({10,5,3},VelocitySettings{},.5f));
    v.chance_percent=20;CHECK(!decide_velocity({1,1,1},v,.9f));
    std::cout<<"PASS combat click, GUI gating, aiming, blocking, hit-selection, reset, velocity\n";
}
