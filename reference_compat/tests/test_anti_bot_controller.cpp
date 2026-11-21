#include "slinkywinky/anti_bot_controller.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do {if(!(x)){\
 std::fprintf(stderr,"R270 AntiBot %d: %s\n",__LINE__,#x);\
 std::exit(1);}} while(0)
static AntiBotPlayer player(std::uint64_t msb=0x2000u) {
    AntiBotPlayer p;
    p.uuid={msb,22};p.name=u"Normal_01";p.ticks_existed=40;
    return p;
}
static AntiBotFrameResult run(AntiBotController& c,
                              AntiBotSettings s,AntiBotPlayer p,
                              std::uint64_t world=1) {
    return c.on_event0(world,true,{0,99},s,{p});
}
int main() {
    CHECK(!AntiBotController::valid_name(u""));
    CHECK(!AntiBotController::valid_name(u"\u00a7a"));
    CHECK(!AntiBotController::valid_name(u"-!~"));
    CHECK(AntiBotController::valid_name(u"\u00a7cABC"));
    CHECK(AntiBotController::valid_name(u"_"));
    CHECK(AntiBotController::valid_name(u"77"));
    CHECK(!AntiBotController::valid_name(u"\u00C1"));
    AntiBotController c;
    AntiBotSettings cfg;cfg.always_stationary=true;
    auto p=player();
    auto r=run(c,cfg,p);
    CHECK(r.admitted && r.proposals.size()==1 &&
          r.proposals[0].rejected && c.history_entries()==1);
    p.x=0.6;
    r=run(c,cfg,p);
    CHECK(r.proposals[0].rejected && r.proposals[0].travelled_squared>0.35);
    p.x=0.9;p.previous_x=0;
    r=run(c,cfg,p);
    CHECK(!r.proposals[0].rejected && r.proposals[0].travelled_squared>1.0);
    // Exact reconstructed Java truth table: the nibble == 0x2000
    // causes rejection in modes 1/2 (unless movement permits mode1).
    // A different nibble is NOT rejected by this particular gate.
    for(int mode=0;mode<=2;++mode) {
        for(int version=0;version<2;++version) {
            for(int moved=0;moved<2;++moved) {
                c.reset();
                cfg={};cfg.invalid_uuid_mode=mode;
                p=player(version==0?0x2000u:0x5000u);
                p.x=moved==0?0.0:2.0;
                const bool expected_reject=(mode!=0 && version==0 &&
                                             (mode==2 || moved==0));
                const auto result=run(c,cfg,p);
                CHECK(result.admitted && result.proposals.size()==1);
                CHECK(result.proposals[0].rejected==expected_reject);
            }
        }
    }
    c.reset();
    cfg={};cfg.sleeping=true;p=player();p.sleeping=true;
    CHECK(run(c,cfg,p).proposals[0].rejected);
    cfg.sleeping=false;
    CHECK(!run(c,cfg,p).proposals[0].rejected);
    c.reset();cfg={};cfg.always_invis=true;
    p=player();p.invisible_to_owner=true;p.has_armor=false;
    p.ticks_existed=21;
    r=run(c,cfg,p);
    CHECK(r.proposals[0].rejected && !r.proposals[0].armor_seen);
    p.has_armor=true;
    r=run(c,cfg,p);
    CHECK(!r.proposals[0].rejected && r.proposals[0].armor_seen);
    p.has_armor=false;
    r=run(c,cfg,p);
    CHECK(!r.proposals[0].rejected); // sticky armor_seen
    c.reset();cfg={};cfg.min_age=12.5f;
    p=player();p.ticks_existed=12;
    CHECK(run(c,cfg,p).proposals[0].rejected);
    p.ticks_existed=13;
    CHECK(run(c,cfg,p).proposals[0].age_accepted);
    p.ticks_existed=1;
    CHECK(!run(c,cfg,p).proposals[0].rejected); // sticky age_accepted
    c.reset();cfg={};cfg.invalid_username=true;
    p=player();p.name=u"\u00a7a!";
    CHECK(run(c,cfg,p).proposals[0].rejected);
    p.name=u"ABC";
    CHECK(run(c,cfg,p).proposals[0].name_accepted);
    p.name=u"!";
    CHECK(!run(c,cfg,p).proposals[0].rejected);
    // New world must not inherit old age/name/armor acceptance.
    p=player();p.ticks_existed=1;p.name=u"!";
    cfg.min_age=12.5f;
    r=run(c,cfg,p,2);
    CHECK(r.world_changed && r.proposals[0].rejected &&
          !r.proposals[0].age_accepted);
    CHECK(c.current_world()==2);
    // A malformed second player does not partly write the first.
    AntiBotPlayer bad=player();bad.uuid={0x2000,23};
    bad.x=std::numeric_limits<double>::quiet_NaN();
    const auto prev_count=c.history_entries();
    auto invalid=c.on_event0(2,true,{0,99},cfg,{p,bad});
    CHECK(!invalid.admitted && c.history_entries()==prev_count);
    cfg.invalid_uuid_mode=3;
    CHECK(!run(c,cfg,p).admitted);
    CHECK(c.on_disable_clear_mask()==2u &&
          c.history_entries()==0 && c.current_world()==0);
    CHECK(c.on_unload_clear_mask()==2u);
    // Owner entity is skipped, not labeled as a bot.
    cfg={};
    auto same=player();same.uuid={0,99};
    r=c.on_event0(3,true,{0,99},cfg,{same});
    CHECK(r.admitted && r.proposals.empty() && c.history_entries()==0);
    std::puts("R270_ANTIBOT_SOURCE_HISTORY_AND_GATING_PASS");
}
