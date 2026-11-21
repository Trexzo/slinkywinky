#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace slinkywinky::reference_compat;
static void ck(bool ok,const char* message,int line) {
    if(!ok)throw std::runtime_error("R273 Piercing bridge "+std::to_string(line)+": "+message);
}
#define CHECK(x) ck(!!(x),#x,__LINE__)
static ExecutionFrame event8() {
    ExecutionFrame f;f.event=kReachQueryEvent;
    f.world_present=f.player_present=true;
    HitboxesQueryPayload hit;hit.input.expand=0.25f;
    f.hitboxes=hit;
    ReachQueryPayload reach;reach.input.sprinting=true;
    reach.settings.distance=3.2f;f.reach=reach;
    GhostHandQueryPayload ghost;
    ghost.input.game_ready=true;
    ghost.input.module_enabled=true;
    ghost.input.holding_item=true;
    ghost.input.item_id=267;
    ghost.input.target_is_remote_player=true;
    ghost.settings.enemies=true;
    ghost.settings.allow_using_sword=true;
    f.ghost_hand=ghost;
    f.piercing=PiercingQueryPayload{};
    return f;
}
int main() {
    ModuleExecutionBridge b;
    const auto hit=b.add({"Hitboxes",0,{{8,60}},false});
    const auto reach=b.add({"Reach",0,{{8,40}},false});
    const auto ghost=b.add({"Ghost Hand",0,{{8,20}},false});
    const auto piercing=b.add({"Piercing",0,{{8,10}},false});
    CHECK(b.bind_hitboxes_from_catalog() && b.bind_reach(reach) &&
          b.bind_ghost_hand(ghost) && b.bind_piercing_from_catalog());
    CHECK(b.coverage().implemented_per_event[8]==4);
    CHECK(!b.bind_piercing(piercing));
    PiercingSettings settings;settings.enabled=true;settings.enemies=true;
    settings.friendlies=true;settings.bots=true;
    PiercingEntity first,second;first.remote_player=true;
    second.remote_player=true;second.relationship_flags=2;
    CHECK(!b.preview_piercing_rank(settings,true,true,first,second).admitted);
    CHECK(!b.preview_pierce_blocks(settings,true));
    CHECK(b.request(hit,true)&&b.request(reach,true)&&
          b.request(ghost,true)&&b.request(piercing,true)&&
          b.apply(true,true));
    auto f=event8();CHECK(b.dispatch(f));
    CHECK((f.executed_modules==
          std::vector<std::size_t>{hit,reach,ghost,piercing}));
    CHECK(f.hitboxes->executions==1 && f.reach->executions==1 &&
          f.ghost_hand->executions==1 && f.piercing->executions==1 &&
          f.piercing->query_override && f.ghost_hand->query_override);
    // OnEvent8 does not mutate a candidate rank, which is queried
    // separately by the actual ray observer in the original client.
    auto decision=b.preview_piercing_rank(settings,true,true,first,second);
    CHECK(decision.admitted && decision.ranking==1);
    settings.bots=false;second.relationship_flags=0;
    second.health=10;first.health=20;settings.sort_by=1;
    decision=b.preview_piercing_rank(settings,true,true,first,second);
    CHECK(decision.admitted && decision.ranking==0);
    settings.blocks=true;settings.require_weapon=true;
    CHECK(b.preview_pierce_blocks(settings,true) &&
          !b.preview_pierce_blocks(settings,false));
    f=event8();f.hitboxes.reset();f.reach.reset();f.ghost_hand.reset();
    CHECK(b.dispatch(f) && f.piercing->query_override);
    f=event8();f.event=13;
    CHECK(!b.dispatch(f) && f.piercing->executions==0);
    // Disabling only Piercing clears query override and denies rank.
    CHECK(b.request(piercing,false)&&b.apply(true,true));
    f=event8();CHECK(b.dispatch(f));
    CHECK(f.piercing->executions==0 && !f.piercing->query_override &&
          f.reach->executions==1);
    CHECK(!b.preview_piercing_rank(settings,true,true,first,second).admitted);
    // Reject wrong module/category/event metadata and duplicate hooks.
    ModuleExecutionBridge wrong;
    const auto wrongCategory=wrong.add({"Piercing",4,{{8,0}},true});
    const auto wrongName=wrong.add({"Other",0,{{8,0}},true});
    CHECK(!wrong.bind_piercing(wrongCategory) &&
          !wrong.bind_piercing(wrongName));
    ModuleExecutionBridge wrongEvent;
    const auto wrongSlot=wrongEvent.add({"Piercing",0,{{14,0}},true});
    CHECK(!wrongEvent.bind_piercing(wrongSlot));
    ModuleExecutionBridge extra;
    const auto invalidSubs=extra.add({"Piercing",0,{{8,0},{14,0}},true});
    CHECK(!extra.bind_piercing(invalidSubs));
    b.stop();f=event8();CHECK(!b.dispatch(f));
    CHECK(!b.preview_piercing_rank(settings,true,true,first,second).admitted);
    std::cout<<"R273_PIERCING_EVENT8_AND_SEPARATE_RANK_BRIDGE_PASS\n";
}
