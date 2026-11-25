#include "slinkywinky/auto_pot_controller.hpp"
#include "slinkywinky/auto_soup_controller.hpp"
#include "slinkywinky/auto_rod_controller.hpp"
#include "slinkywinky/refill_controller.hpp"
#include "slinkywinky/inv_manager_controller.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace slinkywinky::reference_compat;
static void check(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
int main(){
 try{
    HotbarSample h{};
    h[0]={373,5,true};h[1]={373,0x4001,true};h[2]={373,0x4005,true};
    h[3]={282,0,true};h[4]={413,0,true};h[5]={346,0,true};
    h[6]={346,0,true};h[7]={344,0,true};
    check(AutoPotController::first_slot(h,false)==2,"healing splash");
    check(AutoPotController::first_slot(h,true)==1,"nonhealing splash");
    check(AutoPotController::first_slot({},true)==9,"missing potion sentinel");
    auto c=AutoPotController::count({false,true,true},true,4,20);
    check(c.initial==2&&c.extra==0,"double batch count");
    c=AutoPotController::count({false,true,false},true,4,20);
    check(c.initial==1&&c.extra==1,"single extra count");
    check(AutoPotController::count({false,true,true},true,20,20).initial==0,"prevent waste");
    check(AutoPotController::count({false,false,false},true,20,20).extra==5,"unrestricted extra");
    check(AutoPotController::count({false,true,false},false,0,20).initial==0,"no world");
    auto threshold=AutoSoupController::health(10,14,10);
    check(threshold.first&&threshold.second,"inclusive double threshold");
    check(!AutoSoupController::health(15,14,10).first,"above threshold");
    std::array<bool,9> trace{};
    auto s=AutoSoupController::classify(h,3,false,false,{},trace);
    check(s.slot==3&&s.leaves_bowl,"current stew");
    s=AutoSoupController::classify(h,3,true,false,{},trace);
    check(s.slot==4&&s.leaves_bowl,"busy skip");
    trace[0]=true;
    check(AutoSoupController::classify(h,3,false,false,{},trace).slot==4,"explicit continuation");
    trace[0]=false;
    s=AutoSoupController::classify(h,8,false,true,{346},trace);
    check(s.slot==5&&!s.leaves_bowl,"custom item");
    check(AutoSoupController::classify({},0,false,false,{},trace).slot==-1,"no soup");
    auto rod=AutoRodController::classify(h,true);
    check(rod.slot==7&&!rod.fishing_rod,"throwable preferred");
    rod=AutoRodController::classify(h,false);
    check(rod.slot==5&&rod.fishing_rod,"first rod");
    h[7]={};rod=AutoRodController::classify(h,true);
    check(rod.slot==6&&rod.fishing_rod,"last rod fallback");
    MainInventorySample main{};
    main[0]=h[0];main[1]=h[1];main[2]=h[2];main[3]=h[3];main[4]=h[4];
    auto types=RefillController::classify(main,{true,4,6});
    check(types[0]==1&&types[1]==5&&types[2]==2&&types[3]==7&&types[4]==1&&types[26]==0,"27 slot classes");
    types=RefillController::classify(main,{false,4,6});
    check(types[1]==1&&types[2]==1,"potion setting");
    check(RefillController::hotbar_has_space(h),"empty hotbar");
    h.fill({1,0,true});check(!RefillController::hotbar_has_space(h),"full hotbar");
    check(InvManagerController::paired_kind(22)==25&&InvManagerController::paired_kind(17)==7&&InvManagerController::paired_kind(7)==17,"paired mapping");
    check(InvManagerController::rank(true,true,7,1,65535)==65536u,"direct outranks paired");
    check(InvManagerController::rank(true,true,7,0,12)==12,"paired fallback");
    check(InvManagerController::rank(true,true,8,0,12)==0,"unpaired");
    check(InvManagerController::rank(true,false,7,12,100)==12,"no pairing");
    check(InvManagerController::rank(false,true,7,1,100)==0,"missing stack");
    bool rejected=false;
    try{AutoPotController::count({false,true,false},true,std::numeric_limits<float>::quiet_NaN(),20);}
    catch(const std::invalid_argument&){rejected=true;}check(rejected,"bad health");
    rejected=false;
    try{RefillController::classify(main,{true,255,0});}
    catch(const std::invalid_argument&){rejected=true;}check(rejected,"invalid class offset");
    std::cout<<"R314_FIVE_INVENTORY_MODELS_PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
