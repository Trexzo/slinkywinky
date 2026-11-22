#include "slinkywinky/player_esp_controller.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"R299 Player ESP %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main() {
    static_assert(PlayerEspController::listens(6)&&
                  PlayerEspController::listens(7)&&PlayerEspController::listens(18)&&
                  !PlayerEspController::listens(0),"source callback events");
    static_assert(PlayerEspController::relationship_mask(false)==2u&&
                  PlayerEspController::relationship_mask(true)==7u,"source flags");
    static_assert(PlayerEspController::kSourceHealthMax==1.0f,"source max");
    CHECK(PlayerEspController::java_float_compare(-0.0f,0.0f)<0);
    CHECK(PlayerEspController::java_float_compare(
      std::numeric_limits<float>::quiet_NaN(),
      std::numeric_limits<float>::infinity())>0);
    PlayerEspController c;
    PlayerEspSettings s;s.range=10.0f;s.color_mode=1;s.override_target_color=true;
    PlayerEspEntity near;near.position.x=2.0;near.previous.x=1.0;
    near.health=10.0f;near.max_health=20.0f;near.absorption=5.0f;
    near.hurt_time=3;
    PlayerEspEntity far=near;far.position.x=7.0;far.is_target=true;
    PlayerEspEntity other=near;other.position.x=5.0;other.relationship_flags=1u;
    PlayerEspEntity hidden=near;hidden.position.x=3.0;hidden.relationship_flags=2u;
    PlayerEspEntity dead=near;dead.dead=true;
    PlayerEspEntity own=near;own.is_self=true;
    c.update(true,{},s,{near,far,other,hidden,dead,own});
    CHECK(c.rows().size()==3u);
    CHECK(c.rows()[0].position.x==7.0&&c.rows()[0].target&&c.rows()[0].color=="target");
    CHECK(c.rows()[1].position.x==5.0&&c.rows()[1].color=="friendly");
    CHECK(c.rows()[2].position.x==2.0&&c.rows()[2].color=="enemy");
    CHECK(c.rows()[2].health==0.6f&&c.rows()[2].absorption==0.5f&&
          c.rows()[2].hurt==3&&c.rows()[2].previous.x==1.0);
    s.hide_friendlies=true;c.update(true,{},s,{near,far,other,hidden});
    CHECK(c.rows().size()==2u);
    // Java update() does not clear collected rows if the context is unready.
    c.update(false,{},s,{});CHECK(c.rows().size()==2u);
    c.world_unload();CHECK(c.rows().empty());
    CHECK(c.screen_mode(s,true)&&!c.world_mode(s,true));
    s.mode=1;CHECK(c.world_mode(s,true)&&!c.world_mode(s,false)&&
                   !c.screen_mode(s,true));
    std::vector<PlayerEspEntity> many;
    s.hide_friendlies=false;s.range=1000.0f;
    for(int i=1;i<=105;++i){PlayerEspEntity p;p.position.x=i;many.push_back(p);}
    c.update(true,{},s,many);
    CHECK(c.rows().size()==100u&&c.rows().front().position.x==100.0&&
          c.rows().back().position.x==1.0); // cap before descending sort
    PlayerEspEntity zero;zero.max_health=0.0f;zero.health=0.0f;
    c.update(true,{},s,{zero});
    CHECK(c.rows().size()==1u&&c.rows()[0].health==0.0f&&
          c.rows()[0].absorption==0.0f);
    // Original comparison and Float.compare deliberately retain NaN rows.
    PlayerEspEntity nan;nan.position.x=std::numeric_limits<double>::quiet_NaN();
    c.update(true,{},s,{near,nan});
    CHECK(c.rows().size()==2u&&std::isnan(c.rows()[0].distance));
    std::puts("R299_PLAYER_ESP_SOURCE_ROWS_SORT_HEALTH_PASS");
}
