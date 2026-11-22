#include "slinkywinky/item_esp_controller.hpp"
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <string>
using namespace slinkywinky::reference_compat;
#define CHECK(x) do {if(!(x)){std::fprintf(stderr,"R298 Item ESP %d: %s\n",__LINE__,#x);std::exit(1);}}while(0)
int main() {
    static_assert(ItemEspController::listens(6)&&ItemEspController::listens(18)&&
                  ItemEspController::listens(26)&&!ItemEspController::listens(0),"events");
    static_assert(ItemEspController::kSourceMergeRadiusSquared==0.0625,"source constant");
    ItemEspController c;
    ItemEspSettings s;s.range=5.0f;s.show_count=true;s.default_color="fallback";
    std::vector<ItemEspPolicy> allowed={{10,"red"},{10,"duplicate"},{-1,"invalid"},{11,"blue"}};
    std::vector<ItemEspEntity> data;
    auto push=[&](int id,double x,int count){ItemEspEntity e;e.item_id=id;e.stack_size=count;
        e.position.x=x;e.previous.x=x-1;data.push_back(e);};
    push(10,0.0,1);push(10,0.2,2);push(10,0.25,3);push(10,0.251,4);
    push(11,0.0,1);push(12,0.0,6);push(10,6.0,1);
    auto width=[](const std::string& s){return static_cast<int>(s.size()*7);};
    c.sample(true,{},s,allowed,data,width);
    CHECK(c.policy_size()==2u && c.rows().size()==5u);
    CHECK(c.rows()[0].quantity==3&&c.rows()[0].label=="3"&&c.rows()[0].width==7);
    CHECK(c.rows()[0].color=="red"&&c.rows()[0].previous.x==-1.0);
    CHECK(c.rows()[1].quantity==0&&c.rows()[1].label.empty());
    CHECK(c.rows()[2].quantity==7&&c.rows()[2].label=="7"&&c.rows()[3].quantity==0);
    CHECK(c.rows()[4].item_id==11&&c.rows()[4].label.empty());
    // Strict merge inequality: entities exactly 0.25 apart do not merge.
    data.clear();push(10,0.0,1);push(10,0.25,2);
    c.sample(true,{},s,allowed,data,width);
    CHECK(c.rows().size()==2u&&c.rows()[0].quantity==1&&c.rows()[1].quantity==2);
    // Same ID group capped at 99, original constructor truncates to 8 bits.
    data.clear();push(10,0.0,256);push(10,0.1,200);push(10,0.12,200);
    c.sample(true,{},s,allowed,data,width);
    CHECK(c.rows().size()==3u&&c.rows()[0].quantity==0);
    CHECK(c.rows()[1].quantity==99&&c.rows()[1].label=="99"&&c.rows()[2].quantity==0);
    s.highlight_all=true;s.show_count=false;
    c.sample(true,{},s,allowed,data,width);
    CHECK(c.rows().size()==3u&&c.rows()[0].quantity==0&&c.rows()[1].quantity==200);
    data.clear();push(12,0.0,1);
    c.sample(true,{},s,allowed,data,width);
    CHECK(c.rows().size()==1u&&c.rows()[0].color=="fallback");
    // Disabled world clears rows but refreshes resolved settings.
    c.sample(false,{},s,allowed,data,width);
    CHECK(c.rows().empty()&&c.policy_size()==2u);
    s.highlight_all=true;
    data.assign(110,ItemEspEntity{});
    for(auto& e:data){e.item_id=10;e.stack_size=1;}
    c.sample(true,{},s,allowed,data,width);CHECK(c.rows().size()==100u);
    c.world_unload();CHECK(c.rows().empty()&&c.policy_size()==2u);
    c.clear_event26();CHECK(c.rows().empty()&&c.policy_size()==0u);
    std::puts("R298_ITEM_ESP_SOURCE_SAMPLE_MERGE_PASS");
}
