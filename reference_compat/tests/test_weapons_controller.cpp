#include "slinkywinky/weapons_controller.hpp"
#include "slinkywinky/auto_weapon_controller.hpp"
#include <cstdio>
#include <cstdlib>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do{if(!(x)){\
 std::fprintf(stderr,"R260 Weapons line %d: %s\n",__LINE__,#x);\
 std::exit(1);}}while(false)
int main(){
    WeaponsInventory inv;WeaponsSettings cfg;
    auto r=WeaponsController::first_allowed_slot(inv,cfg);
    CHECK(r.admitted && r.first_allowed_slot==-1);
    CHECK(WeaponsController::enchantment_group(16)==1u);
    CHECK(WeaponsController::enchantment_group(19)==2u);
    CHECK(WeaponsController::enchantment_group(20)==4u);
    CHECK(WeaponsController::enchantment_group(17)==0u);
    CHECK(!WeaponsController::event26_requests_clear_extras(false));
    CHECK(WeaponsController::event26_requests_clear_extras(true));
    // A nonempty selected slot setting wins before a later weapon.
    cfg.selected_slots[2]=true;
    inv.slots[5]={true,267,0};
    cfg.swords=true;
    r=WeaponsController::first_allowed_slot(inv,cfg);
    CHECK(r.admitted && r.first_allowed_slot==2);
    cfg.selected_slots[2]=false;
    r=WeaponsController::first_allowed_slot(inv,cfg);
    CHECK(r.first_allowed_slot==5);
    CHECK(WeaponsController::identifier_allowed(267,cfg));
    CHECK(!WeaponsController::identifier_allowed(258,cfg));
    cfg.axes=true;
    CHECK(WeaponsController::identifier_allowed(258,cfg));
    CHECK(WeaponsController::identifier_allowed(271,cfg));
    CHECK(WeaponsController::identifier_allowed(275,cfg));
    CHECK(WeaponsController::identifier_allowed(279,cfg));
    CHECK(WeaponsController::identifier_allowed(286,cfg));
    CHECK(!WeaponsController::identifier_allowed(257,cfg));
    cfg.extra_item_identifiers={280};
    CHECK(WeaponsController::identifier_allowed(280,cfg));
    CHECK(!WeaponsController::identifier_allowed(283,
          WeaponsSettings{}));
    // Source: held empty slot uses fists only, NOT selected-slot flag.
    WeaponItem empty;
    cfg.selected_slots[0]=true;
    CHECK(!WeaponsController::allowed_current(empty,0,cfg));
    cfg.fists=true;
    CHECK(WeaponsController::allowed_current(empty,0,cfg));
    cfg.fists=false;cfg.selected_slots[0]=false;
    // Enchantment groups still allow an otherwise unrecognized item.
    inv.slots[0]={true,1,4};
    cfg.fire_aspect=true;
    CHECK(WeaponsController::allowed_current(inv.slots[0],0,cfg));
    CHECK(WeaponsController::group_allowed(4,cfg));
    CHECK(!WeaponsController::group_allowed(3,cfg));
    cfg.sharpness=true;cfg.knockback=true;
    CHECK(WeaponsController::group_allowed(1,cfg));
    CHECK(WeaponsController::group_allowed(2,cfg));
    // First allowed must use slot order, not item priority.
    inv.slots[1]={true,280,0};
    r=WeaponsController::first_allowed_slot(inv,cfg);
    CHECK(r.first_allowed_slot==0);
    cfg.fire_aspect=false;
    r=WeaponsController::first_allowed_slot(inv,cfg);
    CHECK(r.first_allowed_slot==1);
    // Feed real first-slot model into R259 Auto Weapon delay,
    // without performing an inventory change.
    AutoWeaponInput input;
    input.input_ready=true;input.mouse_pressed=true;
    input.hit_is_entity=true;input.remote_other_player=true;
    input.hostile=true;input.world_token=4;input.player_token=8;
    input.current_slot=5;input.first_allowed_slot=r.first_allowed_slot;
    input.activation_time=1.0f;input.action_owner_granted=true;
    AutoWeaponController autoWeapon;
    auto a=autoWeapon.on_event25(input);
    CHECK(a.admitted && a.waiting && !a.proposed_slot_change);
    a=autoWeapon.on_event25(input);
    CHECK(a.proposed_slot_change && a.proposed_slot==1);
    // Malformed observations fail closed.
    inv.slots[8]={true,-1,0};
    CHECK(!WeaponsController::first_allowed_slot(inv,cfg).admitted);
    inv.slots[8]={true,1,8};
    CHECK(!WeaponsController::first_allowed_slot(inv,cfg).admitted);
    inv.slots[8]={};
    cfg.extra_item_identifiers={-1};
    CHECK(!WeaponsController::first_allowed_slot(inv,cfg).admitted);
    std::puts("R260_SOURCE_WEAPONS_SELECTOR_AUTO_WEAPON_INTEROP_PASS");
}
