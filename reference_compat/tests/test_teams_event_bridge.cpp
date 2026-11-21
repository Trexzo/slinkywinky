#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void check(bool ok,const char* expr,int line){
    if(!ok)throw std::runtime_error("R267 Teams event0 "+std::to_string(line)+": "+expr);
}
#define CHECK(x) check(!!(x),#x,__LINE__)
static ExecutionFrame query(bool ready=true) {
    ExecutionFrame f;
    f.event=kSafeWalkEvent;
    f.world_present=f.player_present=true;
    TeamsEvent0Payload p;
    p.world_ready=ready;
    p.owner.entity.raw_name=u"\u00A79Self";
    p.other_players.push_back(TeamsEntityName{u"\u00A79Friend",std::nullopt});
    p.other_players.push_back(TeamsEntityName{u"\u00A7cEnemy",std::nullopt});
    p.settings.match_self_color=true;
    f.teams_event0=p;
    return f;
}
int main(){
    ModuleExecutionBridge b;
    const auto teams=b.add({"Teams",4,{{0,50}},false});
    const auto safe=b.add({"Safe Walk",1,{{0,0}},false});
    const auto clutch=b.add({"Clutch",3,{{0,0},{13,-100},{18,-1000}},false});
    const auto block=b.add({"Block In",3,
        {{0,0},{5,0},{13,-2000},{15,-1000},{16,-500}},false});
    CHECK(b.bind_teams_from_catalog() && b.bind_safe_walk_from_catalog() &&
          b.bind_clutch_from_catalog() && b.bind_block_in_from_catalog());
    CHECK(!b.bind_teams(teams));
    CHECK(b.coverage().implemented_per_event[0]==4);
    CHECK(b.request(teams,true) && b.request(safe,true) &&
          b.request(clutch,true) && b.request(block,true) &&
          b.apply(true,true));
    auto f=query();
    CHECK(b.dispatch(f));
    CHECK(f.teams_event0->executions==1 &&
          f.teams_event0->decisions.size()==2);
    CHECK(f.teams_event0->decisions[0].proposed_team_member &&
          !f.teams_event0->decisions[1].proposed_team_member);
    CHECK((f.executed_modules==
          std::vector<std::size_t>{teams,safe,clutch,block}));
    // Same event0 can carry an unrelated movement policy: inputs do not
    // need to be invented for Teams itself, and no physical key is set.
    f=query();
    SafeWalkTickPayload sw;
    sw.input.in_game=true;
    f.safe_walk=sw;
    CHECK(b.dispatch(f));
    CHECK(f.teams_event0->decisions.size()==2);
    // Regression: Clutch + Block In empty + Teams used to dereference a
    // missing fast_mine_early input in the combined event0 guard.
    f=query();
    ClutchClickPayload cc;
    cc.input.in_game=true;
    f.clutch_click=cc;
    f.block_in_empty=BlockInEmptyPayload{};
    CHECK(b.dispatch(f) && f.teams_event0->decisions.size()==2 &&
          f.clutch_click->executions==1 &&
          f.block_in_empty->executions==1);
    // Real conflicting in_game evidence is rejected before callbacks.
    f=query();
    SafeWalkTickPayload conflicting;
    conflicting.input.in_game=false;
    f.safe_walk=conflicting;
    f.clutch_click=cc;
    CHECK(!b.dispatch(f) && f.teams_event0->executions==0);
    // A not-ready world still delivers the event but no team decisions.
    f=query(false);
    CHECK(b.dispatch(f) && f.teams_event0->executions==1 &&
          f.teams_event0->decisions.empty());
    f=query();
    f.teams_event0->settings.match_self_color=false;
    f.teams_event0->settings.named_colors[12]=true;
    CHECK(b.dispatch(f) &&
          !f.teams_event0->decisions[0].proposed_team_member &&
          f.teams_event0->decisions[1].proposed_team_member);
    f=query();f.event=8;
    CHECK(!b.dispatch(f) && f.teams_event0->executions==0);
    CHECK(b.request(teams,false) && b.apply(true,true));
    f=query();
    CHECK(b.dispatch(f) && f.teams_event0->executions==0 &&
          f.teams_event0->decisions.empty());
    // Metadata checks: category, name, event and extra slots.
    ModuleExecutionBridge wrong;
    auto badName=wrong.add({"Other",4,{{0,0}},true});
    auto badCategory=wrong.add({"Teams",0,{{0,0}},true});
    CHECK(!wrong.bind_teams(badName) && !wrong.bind_teams(badCategory));
    ModuleExecutionBridge wrongEvent;
    const auto badEvent=wrongEvent.add({"Teams",4,{{8,0}},true});
    const auto extra=wrongEvent.add({"Teams",4,{{0,0},{5,0}},true});
    CHECK(!wrongEvent.bind_teams(badEvent) &&
          !wrongEvent.bind_teams(extra));
    b.stop();
    f=query();
    CHECK(!b.dispatch(f) && !b.bind_teams(teams));
    std::cout<<"R267_OFFLINE_TEAMS_EVENT0_BRIDGE_PASS\n";
}
