#include "slinkywinky/block_esp_controller.hpp"
#include <iostream>
#include <stdexcept>
using namespace slinkywinky::reference_compat;
static void check(bool b,const char* msg){if(!b)throw std::runtime_error(msg);}
int main(){
 try{
    BlockEspController c;
    check(!c.update(1,{0,0},{8,24}),"disabled admission");
    check(c.enable(1),"enable");
    check(c.tick(1).resupply_scene,"initial supply");
    check(!c.enable(1),"duplicate enable");
    check(c.update(1,{0,0},{8,24}),"initial resource");
    check(c.update(1,{0,0},{16,48})&&c.resource_count()==1,"replace");
    check(!c.update(2,{1,0},{8,24}),"foreign owner");
    c.settings_changed();
    auto t=c.tick(1);
    check(t.rebuild_existing&&!t.resupply_scene,"settings rebuild");
    check(c.resource_count()==1&&!c.tick(1).rebuild_existing,"coalesced");
    c.invalidate_scene();c.settings_changed();t=c.tick(1);
    check(t.resupply_scene&&!t.rebuild_existing&&c.resource_count()==0,"invalidation");
    check(c.update(1,{0,0},{8,24})&&c.remove(1,{0,0}),"remove");
    check(c.update(1,{1,0},{8,24}),"resource before world loss");
    c.tick(0);check(c.resource_count()==0&&!c.update(1,{0,0},{8,24}),"world loss");
    c.enable(2);c.tick(2);
    bool rejected=false;
    try{c.update(2,{0,0},{8,3});}catch(const std::invalid_argument&){rejected=true;}
    check(rejected&&c.resource_count()==0,"invalid resource");
    for(int i=0;i<4096;++i)check(c.update(2,{i,0},{8,24}),"bounded insert");
    check(!c.update(2,{4096,0},{8,24}),"capacity");
    check(c.update(2,{0,0},{8,24}),"replacement at capacity");
    c.clear();check(c.resource_count()==0,"shutdown cleanup");
    std::cout<<"R315_RESOURCE_LIFECYCLE_PASS\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
