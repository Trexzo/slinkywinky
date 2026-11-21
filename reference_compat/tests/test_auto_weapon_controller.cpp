#include "slinkywinky/auto_weapon_controller.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do{if(!(x)){\
 std::fprintf(stderr,"R259 AutoWeapon line %d: %s\n",__LINE__,#x);\
 std::exit(1);}}while(false)
static AutoWeaponInput eligible(float delay=1.5f){
    AutoWeaponInput in;
    in.input_ready=true;in.mouse_pressed=true;
    in.hit_is_entity=true;in.remote_other_player=true;
    in.hostile=true;in.current_is_weapon=false;
    in.world_token=7;in.player_token=9;
    in.current_slot=0;in.first_allowed_slot=3;
    in.activation_time=delay;in.action_owner_granted=true;
    return in;
}
int main(){
    AutoWeaponController c;
    auto in=eligible();
    auto d=c.on_event25(in);
    CHECK(d.admitted && d.eligible && d.waiting &&
          !d.proposed_slot_change && d.selection_delay_ticks==1);
    d=c.on_event25(in);
    CHECK(d.waiting && d.selection_delay_ticks==2);
    d=c.on_event25(in);
    CHECK(d.eligible && !d.waiting && d.claim_required &&
          d.proposed_slot_change && d.proposed_slot==3 &&
          d.selection_delay_ticks==2);
    in.action_owner_granted=false;
    d=c.on_event25(in);
    CHECK(d.claim_required && !d.proposed_slot_change &&
          d.selection_delay_ticks==2);
    // Restoring ownership can propose selection with no extra delay.
    in.action_owner_granted=true;
    d=c.on_event25(in);CHECK(d.proposed_slot_change);
    in.current_is_weapon=true;
    d=c.on_event25(in);CHECK(d.admitted && !d.eligible && c.delay_ticks()==0);
    in=eligible(0.0f);c.reset();
    d=c.on_event25(in);CHECK(d.proposed_slot_change && c.delay_ticks()==0);
    in=eligible(1.5f);c.reset();
    in.first_allowed_slot=-1;
    d=c.on_event25(in);CHECK(!d.eligible && c.delay_ticks()==0);
    in.first_allowed_slot=0;
    d=c.on_event25(in);CHECK(!d.eligible);
    in.first_allowed_slot=3;in.hit_is_entity=false;
    d=c.on_event25(in);CHECK(!d.eligible);
    in.hit_is_entity=true;in.remote_other_player=false;
    d=c.on_event25(in);CHECK(!d.eligible);
    in.remote_other_player=true;in.hostile=false;
    d=c.on_event25(in);CHECK(!d.eligible);
    in.hostile=true;in.require_mouse_pressed=true;in.mouse_pressed=false;
    d=c.on_event25(in);CHECK(!d.eligible);
    in.mouse_pressed=true;in.is_using_item=true;
    d=c.on_event25(in);CHECK(!d.eligible);
    in.is_using_item=false;in.input_ready=false;
    d=c.on_event25(in);CHECK(!d.eligible);
    in.input_ready=true;
    d=c.on_event25(in);CHECK(d.eligible && d.waiting);
    // No delay state survives an unexpected world/player owner swap.
    in.player_token=99;
    d=c.on_event25(in);CHECK(!d.admitted && c.delay_ticks()==0);
    d=c.on_event25(in);CHECK(d.admitted && d.waiting);
    c.reset();
    in=eligible();in.activation_time=std::numeric_limits<float>::quiet_NaN();
    d=c.on_event25(in);CHECK(!d.admitted && c.delay_ticks()==0);
    in=eligible();in.current_slot=9;
    d=c.on_event25(in);CHECK(!d.admitted);
    in=eligible();in.first_allowed_slot=10;
    d=c.on_event25(in);CHECK(!d.admitted);
    in=eligible();in.world_token=0;
    d=c.on_event25(in);CHECK(!d.admitted);
    c.reset();
    std::puts("R259_AUTO_WEAPON_DELAY_OWNER_AND_HOTBAR_PROPOSAL_PASS");
}
