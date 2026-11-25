#include "slinkywinky/friends_controller.hpp"
#include "slinkywinky/debug_controller.hpp"
#include "slinkywinky/indicators_controller.hpp"
#include "slinkywinky/mod_overlay_controller.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace slinkywinky::reference_compat;
static void require(bool value,const char* msg){if(!value)throw std::runtime_error(msg);}
int main(){
 try{
    FriendsController f;
    FriendsController::UUID id{1,2},bot{3,4},self{5,6};
    require(f.toggle(true,true,bot,2,8)==FriendsController::Change::none,"bot refusal");
    require(f.toggle(false,true,id,0,8)==FriendsController::Change::none,"disabled click");
    require(f.toggle(true,true,id,0,10)==FriendsController::Change::added,"friend added");
    auto r=f.snapshot(true,true,{{id,1,false},{bot,6,false},{self,4,true}},20);
    require(r[0].flags==5&&r[1].flags==2&&r[2].flags==4,"friend bit only");
    require(f.entries().at(id)==20,"last seen update");
    r=f.disabled(true,false,true,r);
    require(r[0].flags==1&&r[1].flags==2&&r[2].flags==4,"disable only others");
    require(f.toggle(true,true,id,2,30)==FriendsController::Change::removed,"remove even flagged bot");
    f.shutdown();require(f.entries().empty(),"shutdown");
    DebugController d; DebugController::Settings ds{true,true,true};
    d.observe_packet(1,DebugController::Observed::use_item);
    require(d.tick(ds,true,1001,0,false,false).empty(),"ignored event");
    d.observe_packet(3,DebugController::Observed::use_item);
    auto lines=d.tick(ds,true,1002,9,true,true);
    require(lines.size()==2&&lines[0].stamp==2&&lines[0].first_color=='c'&&
            lines[0].second_color=='8'&&lines[0].third_color=='c'&&lines[1].first==1,"hurt and start");
    require(d.tick(ds,true,1003,0,false,false).empty(),"no duplicate state");
    d.observe_packet(0,DebugController::Observed::release_item);
    require(d.tick(ds,true,1004,0,false,false).at(0).first==0,"stop");
    d.observe_click(ds,true);d.observe_click(ds,true);d.observe_click(ds,false);
    lines=d.late_tick(ds,true,1005);
    require(lines.at(0).first==2&&lines[0].second==1&&lines[0].first_color=='2'&&lines[0].second_color=='b',"click summary");
    for(unsigned i=1;i<10;++i)require(d.late_tick(ds,true,i).size()==1,"ten tick window");
    require(d.late_tick(ds,true,10).empty(),"expired window");
    d.reset_session();
    IndicatorsController ind; IndicatorsController::Settings is{true,true,true,true};
    using S=IndicatorsController::Sample;
    std::vector<S> samples{{10,true,{1,0,0},{2,0,0}},
        {10,false,{2,0,0},{1,0,0}},{10,false,{15,0,0},{15,0,0}},
        {12,false,{30,0,0},{31,0,0}},{14,false,{31,0,0},{32,0,0}}};
    for(int i=0;i<12;++i)samples.push_back({12,false,{5,0,0},{6,0,0}});
    ind.collect(is,true,true,{0,0,0},samples);
    auto entries=ind.entries();
    require(entries.size()==10&&entries[0].label=="15m"&&entries[0].color==0xffff0000u&&
        entries[1].color==0xffffff00u&&entries[2].color==0xffffffffu,"filter colors and bound");
    ind.disable(false,false);require(ind.entries().size()==10,"foreign toggle");
    ind.disable(true,false);require(ind.entries().empty(),"own toggle");
    ind.collect(is,true,false,{0,0,0},samples);require(ind.entries().empty(),"missing font");
    ModOverlayController overlay;ModOverlayController::Settings os{{true,false,true,true,true},true,true,true};
    using M=ModOverlayController::Module;
    std::vector<M> modules{{1,0,0,true,true,"NAME","SUFFIX"},{2,1,0,true,true,"MOVE",""},
        {3,2,4,true,true,"HIDDEN",""},{4,4,0,true,false,"UNBOUND",""}};
    overlay.rebuild(os,modules);
    require(overlay.rows().size()==1&&overlay.rows()[0].name=="name"&&overlay.rows()[0].suffix=="suffix","overlay filters");
    overlay.frame(.1f);require(std::abs(overlay.rows()[0].animation-.5f)<.0001f,"fade in");
    overlay.frame(.1f);require(overlay.hud_flags(8)==9,"stable frame flags");
    modules[0].enabled=false;overlay.toggle(os,modules[0]);overlay.frame(.1f);
    require(overlay.rows().size()==1&&overlay.rows()[0].animation>0,"fade out retains");
    overlay.frame(.1f);require(overlay.rows().empty(),"fade out removed");
    overlay.profile_event(0);require(overlay.dirty(),"profile dirty");
    modules.clear();for(unsigned i=0;i<55;++i)modules.push_back({i,0,0,true,true,"LONG NAME",""});
    overlay.rebuild(os,modules);require(overlay.rows().size()==50,"bounded overlay");
    bool rejected=false;try{overlay.frame(std::numeric_limits<float>::quiet_NaN());}
    catch(const std::invalid_argument&){rejected=true;}
    require(rejected&&overlay.rows().size()==50,"nonfinite delta refusal");
    std::cout<<"R312_FOUR_OFFLINE_CONTROLLERS_PASS\n";
    return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
