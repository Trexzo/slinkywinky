#include "slinkywinky/chest_esp_controller.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"R297 Chest ESP %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main() {
    static_assert(ChestEspController::listens(0) && ChestEspController::listens(6) &&
                  !ChestEspController::listens(18) && !ChestEspController::listens(26),"source events");
    static_assert(ChestEspController::kSourceScale==0.5f,"source scale");
    std::uint32_t alpha=0;
    const float a=ChestEspController::kSourceFilledAlpha;
    std::memcpy(&alpha,&a,sizeof(alpha));
    CHECK(alpha==0x3ecccccdU);
    ChestEspSettings s;
    s.range=5.0f;s.render_chest=true;s.render_ender_chest=true;
    s.render_trapped_chest=true;s.render_beacon=true;
    s.filled=true;s.line_width=2.0f;
    ChestEspController c;
    const ChestEspPosition self{1.0,2.0,3.0};
    std::vector<ChestEspTile> input={
        {{4.0,6.0,3.0},54},    // distance == radius: included
        {{7.0,2.0,3.0},130},   // outside
        {{1.0,2.0,3.0},146},
        {{1.0,2.0,4.0},138},
        {{1.0,2.0,3.0},1},     // non-chest block ID
        {{1.0,2.0,3.0},54}
    };
    c.tick(true,self,s.range,s,input);
    CHECK(c.rows().size()==4u && c.rows()[0].block_id==54 &&
          c.rows()[1].block_id==146 && c.rows()[2].block_id==138 &&
          c.rows()[3].block_id==54);
    auto plan=c.plan({0.0,0.0,0.0},s);
    CHECK(plan.draws.size()==4u && plan.line_width==2.0f);
    CHECK(plan.scale==0.5f && plan.filled_alpha==a);
    CHECK(plan.filled_primitive==168960 && plan.outline_primitive==169008);
    CHECK(plan.draws[0].translation[0]==4.5f &&
          plan.draws[0].translation[1]==6.5f &&
          plan.draws[0].translation[2]==3.5f && plan.draws[0].filled);
    CHECK(std::string(plan.draws[1].color_key)=="trapped_chest");
    CHECK(std::string(plan.draws[2].color_key)=="beacon");
    // Flags filter before distance; encounter-order preserved.
    s.render_chest=false;c.tick(true,self,s.range,s,input);
    CHECK(c.rows().size()==2u && c.rows()[0].block_id==146 && c.rows()[1].block_id==138);
    c.tick(false,self,s.range,s,input);CHECK(c.rows().empty());
    s.render_chest=true;
    std::vector<ChestEspTile> many(110,{{1.0,2.0,3.0},54});
    c.tick(true,self,s.range,s,many);CHECK(c.rows().size()==100u);
    c.unload();CHECK(c.rows().empty() && c.plan(self,s).draws.empty());
    // Original source uses comparison !(distance > (double)(range * range)),
    // so a NaN float range causes the comparison to be false.
    s.range=std::numeric_limits<float>::quiet_NaN();
    c.tick(true,self,s.range,s,input);
    CHECK(c.rows().size()==5u);
    // Negative finite ranges are squared by original Java float operator.
    c.tick(true,self,-5.0f,s,input);
    CHECK(c.rows().size()==4u);
    std::puts("R297_CHEST_ESP_SOURCE_COLLECTION_PLAN_PASS");
}
