#include "slinkywinky/roster_readonly_event0.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
using namespace slinkywinky::reference_compat;
static void require(bool ok,const char* expr,int line){
    if(!ok)throw std::runtime_error(
        "R281 roster "+std::to_string(line)+": "+expr);
}
#define CHECK(x) require(!!(x),#x,__LINE__)
static RosterSourceSnapshot snapshot() {
    RosterSourceSnapshot s;
    s.world_token=5;s.owner_token=6;s.generation=1;s.tick=10;
    s.owner_uuid={0,99};
    s.owner_team.entity.raw_name=u"\u00a79Local";
    s.complete_team_color_evidence=true; // explicit synthetic fixture
    RosterSourcePlayer a,b;
    a.bot.uuid={0x2000u,1};a.bot.name=u"\u00a79Same";
    a.bot.ticks_existed=22;
    a.teams.raw_name=a.bot.name;
    b.bot.uuid={0x5000u,2};b.bot.name=u"\u00a7cOther";
    b.bot.ticks_existed=22;
    b.teams.raw_name=b.bot.name;
    s.remote={a,b};
    return s;
}
int main() {
    RosterReadOnlyEvent0 host;
    AntiBotSettings anti;anti.invalid_uuid_mode=2;
    TeamsSettings teams;teams.match_self_color=true;
    auto s=snapshot();
    CHECK(!host.observe(s,anti,teams)); // disabled unless explicitly opted in
    CHECK(host.set_enabled(true,true));
    CHECK(host.observe(s,anti,teams).has_value());
    auto result=host.observe(s,anti,teams);
    CHECK(result && result->teams_evaluated && result->antibot_evaluated);
    CHECK(result->remote.size()==2);
    CHECK(result->remote[0].uuid==s.remote[0].bot.uuid &&
          result->remote[0].team_member &&
          result->remote[0].antibot_rejected);
    CHECK(result->remote[1].uuid==s.remote[1].bot.uuid &&
          !result->remote[1].team_member &&
          !result->remote[1].antibot_rejected);
    s.complete_team_color_evidence=false;
    CHECK(!host.observe(s,anti,teams)); // no invented scoreboard metadata
    CHECK(host.set_enabled(true,false));
    result=host.observe(s,anti,teams);
    CHECK(result && result->antibot_evaluated && !result->teams_evaluated &&
          result->remote.size()==2);
    CHECK(host.set_enabled(true,true));
    s.complete_team_color_evidence=true;
    s.remote[1].bot.name=s.remote[0].bot.name;
    s.remote[1].teams.raw_name=s.remote[1].bot.name;
    s.remote[1].bot.uuid=s.remote[0].bot.uuid;
    CHECK(!host.observe(s,anti,teams)); // duplicate UUID, same name
    s=snapshot();s.tick=9;
    CHECK(!host.observe(s,anti,teams)); // tick rollback
    s=snapshot();s.owner_token=7;
    CHECK(!host.observe(s,anti,teams)); // owner change requires new generation
    s.generation=2;s.tick=0;
    CHECK(host.observe(s,anti,teams)); // new owner scope admitted
    s.world_token=8;s.owner_token=9;s.generation=3;
    CHECK(host.observe(s,anti,teams));
    s=snapshot();s.world_token=8;s.owner_token=9;s.generation=3;s.tick=1;
    s.remote[0].bot.x=std::numeric_limits<double>::quiet_NaN();
    CHECK(!host.observe(s,anti,teams));
    s.remote[0].bot.x=2;
    CHECK(host.observe(s,anti,teams)); // invalid sample didn't commit
    s.remote[0].teams.raw_name=u"Forged Name";
    CHECK(!host.observe(s,anti,teams));
    s.remote[0].teams.raw_name=s.remote[0].bot.name;
    s.complete_team_color_evidence=false;
    CHECK(!host.observe(s,anti,teams));
    CHECK(host.set_enabled(false,true));
    s.complete_team_color_evidence=true;
    result=host.observe(s,anti,teams);
    CHECK(result && result->teams_evaluated && !result->antibot_evaluated);
    bool wrongThreadAccepted=false;
    std::thread worker([&] {
        wrongThreadAccepted=host.observe(s,anti,teams).has_value() ||
                            host.set_enabled(true,true);
    });
    worker.join();
    CHECK(!wrongThreadAccepted);
    CHECK(host.close());
    CHECK(!host.close());
    CHECK(!host.observe(s,anti,teams));
    std::cout<<"R281_READONLY_OWNER_UUID_EVENT0_PROPOSALS_PASS\n";
}
