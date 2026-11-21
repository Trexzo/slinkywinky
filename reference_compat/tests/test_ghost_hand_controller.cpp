#include "slinkywinky/ghost_hand_controller.hpp"
#include <cstdio>
#include <cstdlib>
using namespace slinkywinky::reference_compat;
#define CHECK(condition) do{if(!(condition)){\
 std::fprintf(stderr,"R255 Ghost Hand line %d: %s\n",__LINE__,#condition);\
 std::exit(1);}}while(false)
int main() {
    GhostHandSettings config;
    config.enemies=true;config.allow_using_sword=true;
    GhostHandObservation o;
    o.module_enabled=true;o.game_ready=true;o.mouse_pressed=true;
    o.holding_item=true;o.item_id=267;o.target_is_remote_player=true;
    CHECK(GhostHandController::on_event8(true));
    CHECK(!GhostHandController::on_event8(false));
    CHECK(!GhostHandController::sword_excluded(config));
    CHECK(GhostHandController::item_group(30)==GhostHandItem::cobweb);
    CHECK(GhostHandController::item_group(359)==GhostHandItem::tool);
    CHECK(GhostHandController::item_group(259)==GhostHandItem::flint_steel);
    CHECK(GhostHandController::item_group(327)==GhostHandItem::bucket);
    CHECK(GhostHandController::item_group(276)==GhostHandItem::sword);
    CHECK(GhostHandController::item_group(999999)==GhostHandItem::other);
    auto r=GhostHandController::can_interact_through(config,o);
    CHECK(r.admitted && r.can_interact_through &&
          r.item==GhostHandItem::sword &&
          r.target==GhostHandTarget::enemy);
    config.allow_using_sword=false;
    CHECK(GhostHandController::sword_excluded(config));
    CHECK(!GhostHandController::can_interact_through(config,o).can_interact_through);
    config.allow_using_sword=true;
    config.require_mouse_pressed=true;
    o.mouse_pressed=false;
    CHECK(!GhostHandController::can_interact_through(config,o).can_interact_through);
    o.mouse_pressed=true;
    config.require_mouse_pressed=false;
    o.game_ready=false;
    CHECK(!GhostHandController::can_interact_through(config,o).can_interact_through);
    o.game_ready=true;
    o.module_enabled=false;
    CHECK(!GhostHandController::can_interact_through(config,o).can_interact_through);
    o.module_enabled=true;
    // Relationship priority bots over friendlies when both bits are set.
    o.relationship_flags=7;
    CHECK(GhostHandController::target_group(true,7)==GhostHandTarget::bot);
    CHECK(!GhostHandController::can_interact_through(config,o).can_interact_through);
    config.bots=true;
    CHECK(GhostHandController::can_interact_through(config,o).can_interact_through);
    o.relationship_flags=4;
    config.friendlies=true;
    CHECK(GhostHandController::can_interact_through(config,o).can_interact_through);
    o.relationship_flags=1;
    CHECK(GhostHandController::can_interact_through(config,o).can_interact_through);
    o.target_is_remote_player=false;
    CHECK(!GhostHandController::can_interact_through(config,o).can_interact_through);
    config.non_players=true;
    CHECK(GhostHandController::can_interact_through(config,o).can_interact_through);
    // Fists and held items have separate gates, even if item ID is absent.
    o.holding_item=false;
    CHECK(!GhostHandController::can_interact_through(config,o).can_interact_through);
    config.allow_using_fists=true;
    CHECK(GhostHandController::can_interact_through(config,o).can_interact_through);
    o.holding_item=true;o.item_id=-1;
    CHECK(!GhostHandController::can_interact_through(config,o).admitted);
    const int item_ids[]={30,259,325,256,999};
    for(int id:item_ids) {
        o.item_id=id;
        CHECK(!GhostHandController::can_interact_through(config,o).can_interact_through);
    }
    config.allow_using_cobweb=true;
    config.allow_using_fns=true;
    config.allow_using_buckets=true;
    config.allow_using_tools=true;
    config.allow_using_other=true;
    for(int id:item_ids) {
        o.item_id=id;
        CHECK(GhostHandController::can_interact_through(config,o).can_interact_through);
    }
    o.target_is_remote_player=true;o.relationship_flags=7;
    config.bots=false;
    CHECK(!GhostHandController::can_interact_through(config,o).can_interact_through);
    // Event-8 queryOverride remains logically separate from interaction
    // eligibility, including for mouse/item/relationship gate failures.
    CHECK(GhostHandController::on_event8(true));
    std::puts("R255_GHOST_HAND_FILTER_SOURCE_CLASSIFICATION_PASS");
}
