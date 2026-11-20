#include "slinkywinky/recovered_entity_query_policies.hpp"
#include "slinkywinky/menu.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
using namespace slinkywinky;
#define CHECK(c) do {if(!(c))throw std::runtime_error(\
    "R256 line "+std::to_string(__LINE__)+": " #c);}while(false)

static void registration(){
    const auto& catalog=official_module_catalog();
    CHECK(std::string(catalog[7].name)=="Hitboxes");
    CHECK(catalog[7].tab==MenuTab::Combat);
    CHECK(std::string(catalog[48].name)=="Ghost Hand");
    CHECK(catalog[48].tab==MenuTab::Block);
    CHECK(kRecoveredEntityQueryRegistrations[0].category==0);
    CHECK(kRecoveredEntityQueryRegistrations[1].category==3);
    for(const auto& reg:kRecoveredEntityQueryRegistrations){
        CHECK(reg.event==8);
        CHECK(candidate_native_manager_registered(reg.name));
    }
    CHECK(!candidate_native_manager_registered("Chest Stealer"));
}
static void hitboxes(){
    for(const float value:{0.0f,0.25f,1.0f,8.0f,16.0f}){
        auto v=recovered_hitboxes_query(value);
        CHECK(v && v->proposed_active);
        CHECK(v->expansion_per_side==static_cast<double>(value));
    }
    for(const float value:{
        -0.01f,16.01f,std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN()}){
        CHECK(!recovered_hitboxes_query(value));
    }
}
static void ghost_hand(){
    // Exact original source groupings, including gold tools and shears.
    const int tools[]={256,257,258,269,270,271,273,274,275,
                       277,278,279,284,285,286,359};
    const int swords[]={267,268,272,276,283};
    const int buckets[]={325,326,327};
    RecoveredGhostHandSettings cfg{};
    RecoveredGhostHandQuery in{};
    in.held_item_present=true;
    for(const int id:tools){
        in.held_item_id=id;
        CHECK(!recovered_ghost_item_allowed(cfg,in));
        cfg.allow_tools=true;
        CHECK(recovered_ghost_item_allowed(cfg,in));
        cfg.allow_tools=false;
    }
    for(const int id:swords){
        in.held_item_id=id;
        CHECK(!recovered_ghost_item_allowed(cfg,in));
        cfg.allow_swords=true;
        CHECK(recovered_ghost_item_allowed(cfg,in));
        cfg.allow_swords=false;
    }
    for(const int id:buckets){
        in.held_item_id=id;
        CHECK(!recovered_ghost_item_allowed(cfg,in));
        cfg.allow_buckets=true;
        CHECK(recovered_ghost_item_allowed(cfg,in));
        cfg.allow_buckets=false;
    }
    in.held_item_id=259;
    CHECK(!recovered_ghost_item_allowed(cfg,in));
    cfg.allow_flint_steel=true;
    CHECK(recovered_ghost_item_allowed(cfg,in));
    cfg.allow_flint_steel=false;
    in.held_item_id=30;
    CHECK(!recovered_ghost_item_allowed(cfg,in));
    cfg.allow_cobweb=true;
    CHECK(recovered_ghost_item_allowed(cfg,in));
    cfg.allow_cobweb=false;
    in.held_item_id=264;
    CHECK(!recovered_ghost_item_allowed(cfg,in));
    cfg.allow_other=true;
    CHECK(recovered_ghost_item_allowed(cfg,in));
    cfg.allow_other=false;
    in.held_item_present=false;
    CHECK(!recovered_ghost_item_allowed(cfg,in));
    cfg.allow_fists=true;
    CHECK(recovered_ghost_item_allowed(cfg,in));
    cfg.allow_fists=false;

    // Exhaustively verify original bot -> friend/team -> enemy precedence
    // against distinct switches and all 8 combinations of flag bits.
    std::uint64_t permutations=0;
    for(unsigned flags=0;flags<8;++flags)
    for(unsigned toggles=0;toggles<8;++toggles)
    for(bool other_player:{false,true})
    for(bool mouse:{false,true})
    for(bool require_mouse:{false,true}){
        cfg={};
        cfg.allow_swords=true;
        cfg.require_mouse=require_mouse;
        cfg.ignore_non_players=true;
        cfg.ignore_bots=(toggles&1)!=0;
        cfg.ignore_friendlies=(toggles&2)!=0;
        cfg.ignore_enemies=(toggles&4)!=0;
        in.held_item_present=true;
        in.held_item_id=267;
        in.entity_is_other_player=other_player;
        in.player_flags=flags;
        in.mouse_button_held=mouse;
        bool expected=!require_mouse || mouse;
        if(!other_player)expected=expected && cfg.ignore_non_players;
        else if((flags&2u)!=0)expected=expected && cfg.ignore_bots;
        else if((flags&5u)!=0)expected=expected && cfg.ignore_friendlies;
        else expected=expected && cfg.ignore_enemies;
        CHECK(recovered_ghost_should_ignore(cfg,in)==expected);
        ++permutations;
    }
    CHECK(permutations==512);
    std::cout<<"R256_GHOST_HAND_ITEM_CATEGORIES_FLAG_PRECEDENCE_512_CASES_PASS\n";
}
static void lifecycle(){
    RecoveredEntityQueryOwner owner;
    CHECK(!owner.transition(0,true));
    CHECK(owner.transition(100,true));
    CHECK(!owner.hitboxes(8,0.5f));
    CHECK(!owner.ghost_hand(8,{},{}));
    CHECK(owner.select(RecoveredEntityQueryModule::Hitboxes,true));
    CHECK(owner.select(RecoveredEntityQueryModule::GhostHand,true));
    CHECK(owner.hitboxes(8,0.5f)->expansion_per_side==0.5);
    CHECK(!owner.hitboxes(7,0.5f));
    RecoveredGhostHandSettings cfg;
    cfg.allow_fists=true;cfg.ignore_non_players=true;
    const auto valid=owner.ghost_hand(8,cfg,{});
    CHECK(valid && valid->ignore_entity && valid->reach_query_marker);
    CHECK(!owner.ghost_hand(14,cfg,{}));
    auto snap=owner.snapshot(12);
    CHECK(snap && snap->modules.size()==2);
    CHECK(snap->modules[0].canonical_name=="Hitboxes");
    CHECK(snap->modules[1].canonical_name=="Ghost Hand");
    CHECK(make_module_arraylist_display(*snap)->active_names.size()==2);
    CHECK(owner.transition(100,false));
    CHECK(!owner.hitboxes(8,0.5f) && !owner.ghost_hand(8,cfg,{}));
    auto worldless=owner.snapshot(13);
    CHECK(worldless && !worldless->world_ready);
    CHECK(worldless->modules[0].requested_enabled &&
          !worldless->modules[0].active_in_world);
    CHECK(make_module_arraylist_display(*worldless)->active_names.empty());
    CHECK(!owner.transition(100,true));
    CHECK(owner.transition(101,true));
    CHECK(!owner.transition(100,true));
    bool foreign_ok=false;
    std::thread foreign([&]{
        foreign_ok=owner.transition(102,true) ||
                   owner.select(RecoveredEntityQueryModule::Hitboxes,false) ||
                   owner.snapshot(14).has_value() ||
                   owner.hitboxes(8,0.1f).has_value();
    });
    foreign.join();
    CHECK(!foreign_ok && owner.hitboxes(8,0.5f).has_value());
    CHECK(owner.select(RecoveredEntityQueryModule::Hitboxes,false));
    CHECK(!owner.hitboxes(8,0.5f));
    CHECK(owner.retire() && owner.retired());
    CHECK(!owner.transition(102,true) &&
          !owner.select(RecoveredEntityQueryModule::GhostHand,true) &&
          !owner.snapshot(15) &&
          !owner.ghost_hand(8,cfg,{}));
    std::cout<<"R256_EVENT8_OWNER_WORLD_UNLOAD_FEED_PREVIEW_ISOLATION_PASS\n";
}
int main(){
    registration();
    hitboxes();
    ghost_hand();
    lifecycle();
    std::cout<<"R256_TWO_MORE_SOURCE_POLICIES_NO_MINECRAFT_OR_JNI_WRITES_PASS\n";
}
