#include "slinkywinky/gui_controller.hpp"
#include "slinkywinky/name_tags_controller.hpp"
#include "slinkywinky/pointers_controller.hpp"
#include "slinkywinky/trajectories_controller.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace slinkywinky::reference_compat;
static void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
int main(){
 try{
    GuiController g;
    check(g.input(2,54,true,false,false,54,false).request==GuiController::Request::open,"request open");
    check(!g.open(),"request not transition");
    auto t=g.toggle(true,true,true,true);
    check(t.center_cursor&&t.release_mouse&&t.start_animation&&g.open(),"open lifecycle");
    check(g.input(2,54,true,false,true,54,false).request==GuiController::Request::none,"handler precedence");
    check(!g.input(2,20,true,false,false,54,true).candidate_consumed,"allow keyboard");
    check(g.input(2,5,true,false,false,54,true).candidate_consumed,"low key blocked");
    check(g.input(1,20,true,false,false,54,true).candidate_consumed,"mouse");
    check(g.input(2,0x57,true,false,false,54,true).request==GuiController::Request::close,"F11");
    check(g.hud_flags(8)==11&&g.release_mouse_on_tick(true),"redraw and mouse intent");
    t=g.toggle(true,false,false,true);
    check(t.restore_mouse&&t.reset_popup&&t.reset_drag&&t.save,"close lifecycle");
    g.toggle(true,true,false,false);g.shutdown();
    t=g.toggle(true,false,false,false);check(!t.save&&!t.restore_mouse,"shutdown close");
    check(!g.toggle(true,true,true,true).start_animation,"shutdown open denied");
    auto a=GuiController::appearance(1.25f,true,false,{.2f,.3f,.4f});
    check(a.scale==1.25f&&a.compact&&!a.wide,"appearance");
    NameTagsController n;
    NameTagsController::Settings ns{10,true,false,true,true,true,true};
    using P=NameTagsController::Player;
    std::vector<P> roster{
       {1,0,true,false,{0,0,0},{0,0,0},20,0,"self","Self",{}},
       {2,2,false,false,{1,0,0},{1,0,0},20,0,"bot","Bot",{}},
       {3,1,false,false,{10,0,0},{9,0,0},18,4,"friend","Friend",{true,false,false,false,true}},
       {4,0,false,false,{11,0,0},{11,0,0},20,0,"far","Far",{}},
       {5,0,false,true,{1,0,0},{1,0,0},0,0,"dead","Dead",{}}
    };
    n.collect(ns,true,true,false,{0,0,0},{0,0,0},roster);
    check(n.rows().size()==1,"roster filter");
    const auto& r=n.rows().front();
    check(r.id==3&&r.name_color=='a'&&r.hp==9.f&&r.absorption==2.f&&r.shown_distance==10.f&&r.equipment[4],"tag components");
    ns.display_name=true;
    n.collect(ns,true,true,true,{0,0,0},{0,0,0},roster);
    check(n.rows().size()==2&&n.rows()[0].name=="Self"&&!n.rows()[0].shown_distance,"own third person");
    n.disable(true,false);check(n.rows().empty(),"tag cleanup");
    P sample=roster[2];roster.assign(110,sample);
    n.collect(ns,true,true,false,{0,0,0},{0,0,0},roster);
    check(n.rows().size()==100,"tag cap");
    n.collect(ns,true,false,false,{0,0,0},{0,0,0},roster);
    check(n.rows().empty(),"font loss");
    PointersController::Settings ps{10,30,4,.2f,true,{1,0,0},{0,0,1}};
    auto style=PointersController::style(ps,10,30,.5f);
    check(style.gradient==.5f&&style.radius==6&&style.scale==2&&style.color[0]==.5f&&style.color[2]==.5f,"pointer interpolation");
    check(PointersController::tilt(true,20)==60&&PointersController::tilt(false,20)==0,"pointer tilt");
    ps.far_distance=ps.near_distance;
    check(PointersController::style(ps,20,20,0).gradient==1,"zero gradient fallback");
    TrajectoriesController::Settings ts{true,true,true,true};
    check(!TrajectoriesController::select(ts,true,false,true,0x170,false,0),"third person gate");
    check(!TrajectoriesController::select(ts,true,true,false,0x170,false,0),"silent gate");
    auto l=TrajectoriesController::select(ts,true,false,false,0x170,false,0);
    check(l&&l->power==1.5f&&l->gravity==.03f,"pearl");
    check(!TrajectoriesController::select(ts,true,false,false,0x105,true,72000),"undercharged bow");
    l=TrajectoriesController::select(ts,true,false,false,0x105,true,71980);
    check(l&&l->next_power==3.f&&l->power<3.f&&l->gravity==.05f,"charged bow");
    auto points=TrajectoriesController::free_flight({0,0,0},{1,1,0},.05f,100);
    check(points.size()==100&&points[0].x==1&&points[0].y==1&&std::abs(points[1].y-1.94)<1e-7,"flight");
    bool denied=false;try{TrajectoriesController::free_flight({0,0,0},{1,1,0},.05f,101);}
    catch(const std::invalid_argument&){denied=true;}check(denied,"flight cap");
    std::cout<<"R313_FOUR_VISUAL_MODELS_PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
