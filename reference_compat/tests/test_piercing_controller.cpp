#include "slinkywinky/piercing_controller.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do{if(!(x)){\
 std::fprintf(stderr,"R272 Piercing line %d: %s\n",__LINE__,#x);\
 std::exit(1);}}while(false)
int main(){
    PiercingSettings s;
    PiercingEntity enemy1,enemy2;
    enemy1.remote_player=enemy2.remote_player=true;
    CHECK(!PiercingController::on_event8(false));
    CHECK(PiercingController::on_event8(true));
    CHECK(!PiercingController::pierce_blocks(s,false));
    CHECK(PiercingController::rank(s,true,true,enemy1,enemy2).ranking==2);
    s.enabled=true;s.blocks=true;
    CHECK(PiercingController::pierce_blocks(s,false));
    s.require_weapon=true;
    CHECK(!PiercingController::pierce_blocks(s,false));
    CHECK(PiercingController::pierce_blocks(s,true));
    CHECK(PiercingController::rank(s,false,true,enemy1,enemy2).ranking==2);
    s.require_weapon=false;s.require_inside_bb=true;
    CHECK(PiercingController::rank(s,true,false,enemy1,enemy2).ranking==2);
    s.require_inside_bb=false;
    PiercingEntity nonplayer;
    CHECK(PiercingController::rank(s,true,false,enemy1,nonplayer).ranking==2);
    s.non_players=true;
    CHECK(PiercingController::rank(s,true,false,enemy1,nonplayer).ranking==1);
    CHECK(PiercingController::rank(s,true,false,nonplayer,enemy2).ranking==0);
    s.friendlies=true;enemy2.relationship_flags=1u;
    CHECK(PiercingController::rank(s,true,true,nonplayer,enemy2).ranking==1);
    enemy2.relationship_flags=4u;
    CHECK(PiercingController::rank(s,true,true,nonplayer,enemy2).ranking==1);
    enemy2.relationship_flags=2u;
    CHECK(PiercingController::rank(s,true,true,nonplayer,enemy2).ranking==0);
    s.bots=true;
    CHECK(PiercingController::rank(s,true,true,nonplayer,enemy2).ranking==1);
    enemy2.relationship_flags=0;
    CHECK(PiercingController::rank(s,true,true,enemy1,enemy2).ranking==2);
    s.enemies=true;s.sort_by=1;
    enemy1.health=10.0f;enemy2.health=15.0f;
    CHECK(PiercingController::rank(s,true,true,enemy1,enemy2).ranking==1);
    enemy1.health=19.0f;enemy2.health=15.0f;
    CHECK(PiercingController::rank(s,true,true,enemy1,enemy2).ranking==0);
    enemy1.health=std::numeric_limits<float>::quiet_NaN();
    CHECK(PiercingController::rank(s,true,true,enemy1,enemy2).ranking==0);
    enemy1.health=20.0f;enemy2.health=20.0f;
    s.sort_by=0;enemy1.hurt_time=4;enemy2.hurt_time=8;
    CHECK(PiercingController::rank(s,true,true,enemy1,enemy2).ranking==1);
    enemy1.hurt_time=9;
    CHECK(PiercingController::rank(s,true,true,enemy1,enemy2).ranking==0);
    enemy2.hurt_time=9;enemy1.ticks_existed=1;
    enemy2.ticks_existed=1;
    CHECK(PiercingController::rank(s,true,true,enemy1,enemy2).ranking==1);
    CHECK(PiercingController::mix_bits(0)==0u);
    CHECK(PiercingController::mix_bits(1)!=PiercingController::mix_bits(2));
    // Signed Java counter addition must wrap, never overflow in C++.
    enemy1.ticks_existed=std::numeric_limits<int>::max();
    enemy1.last_attacker_time=1;
    enemy2.ticks_existed=3;enemy2.last_attacker_time=0;
    const auto out=PiercingController::rank(s,true,true,enemy1,enemy2);
    CHECK(out.admitted && (out.ranking==0 || out.ranking==1));
    s.sort_by=3;
    CHECK(!PiercingController::rank(s,true,true,enemy1,enemy2).admitted);
    s.sort_by=0;enemy1.health=std::numeric_limits<float>::infinity();
    CHECK(!PiercingController::rank(s,true,true,enemy1,enemy2).admitted);
    std::puts("R272_PIERCING_SOURCE_RANK_AND_RELATIONSHIPS_PASS");
}
