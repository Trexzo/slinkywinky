#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>
using namespace slinkywinky::reference_compat;
static void ck(bool yes,const char* what,int line) {
    if(!yes)throw std::runtime_error("R271 event0 "+std::to_string(line)+": "+what);
}
#define CHECK(x) ck(!!(x),#x,__LINE__)
static ExecutionFrame frame(bool both=true,int uuidMode=2) {
    ExecutionFrame f;f.event=kSafeWalkEvent;
    f.world_present=f.player_present=true;
    AntiBotEvent0Payload anti;
    anti.context_ready=true;anti.world_token=11;
    anti.local_player={0,999};
    anti.settings.invalid_uuid_mode=uuidMode;
    AntiBotPlayer friendPlayer;
    friendPlayer.uuid={0x2000u,1};friendPlayer.name=u"\u00a79Friend";
    friendPlayer.ticks_existed=40;
    AntiBotPlayer enemyPlayer;
    enemyPlayer.uuid={0x5000u,2};enemyPlayer.name=u"\u00a7cEnemy";
    enemyPlayer.ticks_existed=40;
    anti.other_players={friendPlayer,enemyPlayer};
    f.anti_bot_event0=anti;
    if(both) {
        TeamsEvent0Payload teams;
        teams.world_ready=true;
        teams.owner.entity.raw_name=u"\u00a79Self";
        teams.other_players={TeamsEntityName{friendPlayer.name,std::nullopt},
                             TeamsEntityName{enemyPlayer.name,std::nullopt}};
        teams.source_world_token=anti.world_token;
        teams.source_owner_uuid=anti.local_player;
        teams.other_player_uuids={friendPlayer.uuid,enemyPlayer.uuid};
        teams.settings.match_self_color=true;
        f.teams_event0=teams;
    }
    return f;
}
int main() {
    ModuleExecutionBridge b;
    const auto bot=b.add({"Anti Bot",4,{{0,40}},false});
    const auto team=b.add({"Teams",4,{{0,30}},false});
    const auto clutch=b.add({"Clutch",3,{{0,0},{13,-100},{18,-1000}},false});
    CHECK(b.bind_anti_bot_from_catalog());
    CHECK(b.bind_teams_from_catalog());
    CHECK(b.bind_clutch_from_catalog());
    CHECK(!b.bind_anti_bot(bot));
    CHECK(b.coverage().implemented_per_event[0]==3);
    CHECK(b.request(bot,true)&&b.request(team,true)&&
          b.request(clutch,true)&&b.apply(true,true));
    auto f=frame();
    CHECK(b.dispatch(f));
    CHECK((f.executed_modules==std::vector<std::size_t>{bot,team,clutch}));
    CHECK(f.anti_bot_event0->executions==1 &&
          f.anti_bot_event0->decision.admitted &&
          f.anti_bot_event0->decision.proposals.size()==2);
    // The recovered Java UUID guard rejects nibble 0x2000 when
    // mode2 is enabled; a different nibble is admitted by that gate.
    CHECK(f.anti_bot_event0->decision.proposals[0].rejected &&
          !f.anti_bot_event0->decision.proposals[1].rejected);
    CHECK(f.teams_event0->executions==1 &&
          f.teams_event0->decisions.size()==2 &&
          f.teams_event0->decisions[0].proposed_team_member &&
          !f.teams_event0->decisions[1].proposed_team_member);
    // Anti Bot=bit2, Teams=bit1, neither proposal changes the
    // underlying RelationshipService or overrides the other bit.
    CHECK(f.anti_bot_event0->decision.proposals[0].relationship_flag==2u &&
          f.teams_event0->decisions[0].relationship_flag==1u);
    f=frame();f.teams_event0->other_players[0].raw_name=u"Imposter";
    CHECK(!b.dispatch(f) && f.anti_bot_event0->executions==0);
    f=frame();f.teams_event0->world_ready=false;
    CHECK(!b.dispatch(f));
    // A display-name comparison alone is insufficient. Identical
    // names with different UUID ordering must also fail closed.
    f=frame();
    f.teams_event0->other_players[0].raw_name=u"Shared";
    f.teams_event0->other_players[1].raw_name=u"Shared";
    f.anti_bot_event0->other_players[0].name=u"Shared";
    f.anti_bot_event0->other_players[1].name=u"Shared";
    std::swap(f.teams_event0->other_player_uuids[0],
              f.teams_event0->other_player_uuids[1]);
    CHECK(!b.dispatch(f) && f.executed_modules.empty());
    f=frame();f.teams_event0->source_world_token=99;
    CHECK(!b.dispatch(f) && f.executed_modules.empty());
    f=frame();f.teams_event0->source_owner_uuid={0,42};
    CHECK(!b.dispatch(f) && f.executed_modules.empty());
    f=frame();f.teams_event0->other_player_uuids.clear();
    CHECK(!b.dispatch(f) && f.executed_modules.empty());
    f=frame();f.event=8;
    CHECK(!b.dispatch(f));
    f=frame(false);f.clutch_click=ClutchClickPayload{};
    f.clutch_click->input.in_game=true;
    CHECK(b.dispatch(f) && f.anti_bot_event0->executions==1 &&
          f.clutch_click->executions==1);
    // Disabling only Anti Bot drops its source history.
    CHECK(b.request(bot,false)&&b.apply(true,true));
    f=frame();
    CHECK(b.dispatch(f) &&
          f.anti_bot_event0->executions==0 &&
          f.teams_event0->decisions.size()==2);
    CHECK(b.request(bot,true)&&b.apply(true,true));
    f=frame(false,1);
    f.anti_bot_event0->other_players[1].uuid.msb=0x2000u;
    f.anti_bot_event0->other_players[1].x=2.0;
    CHECK(b.dispatch(f) &&
          !f.anti_bot_event0->decision.proposals[1].rejected);
    // A later event subscriber throwing must roll back AntiBot
    // movement history even though its callback already ran.
    ModuleExecutionBridge tx;
    const auto tbot=tx.add({"Anti Bot",4,{{0,100}},false});
    const auto other=tx.add({"Thrower",4,{{0,-100}},true});
    CHECK(tx.bind_anti_bot(tbot));
    bool firstThrow=true;
    CHECK(tx.bind(other,0,[&](const Delivery&,ExecutionFrame&) {
        if(firstThrow){firstThrow=false;throw std::runtime_error("synthetic");}
    }));
    CHECK(tx.request(tbot,true)&&tx.apply(true,true));
    f=frame(false,1);
    f.anti_bot_event0->other_players[1].uuid.msb=0x2000u;
    f.anti_bot_event0->other_players[1].x=2.0;
    bool thrown=false;
    try{tx.dispatch(f);}catch(const std::runtime_error&){thrown=true;}
    CHECK(thrown);
    f=frame(false,1);
    f.anti_bot_event0->other_players[1].uuid.msb=0x2000u;
    CHECK(tx.dispatch(f));
    CHECK(f.anti_bot_event0->decision.admitted &&
          f.anti_bot_event0->decision.proposals[1].rejected);
    // Exact metadata negative controls.
    ModuleExecutionBridge wrong;
    const auto cat=wrong.add({"Anti Bot",0,{{0,0}},true});
    const auto name=wrong.add({"Not Anti Bot",4,{{0,0}},true});
    CHECK(!wrong.bind_anti_bot(cat) && !wrong.bind_anti_bot(name));
    ModuleExecutionBridge event;
    const auto badEvent=event.add({"Anti Bot",4,{{8,0}},true});
    CHECK(!event.bind_anti_bot(badEvent));
    ModuleExecutionBridge extra;
    const auto extraSub=extra.add({"Anti Bot",4,{{0,0},{8,0}},true});
    CHECK(!extra.bind_anti_bot(extraSub));
    b.stop();tx.stop();
    f=frame();
    CHECK(!b.dispatch(f)&&!b.bind_anti_bot(bot));
    std::cout<<"R271_ANTIBOT_TEAMS_EVENT0_BIT_SEPARATION_PASS\n";
}
