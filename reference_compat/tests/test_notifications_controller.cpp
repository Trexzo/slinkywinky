#include "slinkywinky/notifications_controller.hpp"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace slinkywinky::reference_compat;

static void check(bool result, const char* message) {
    if (!result) throw std::runtime_error(message);
}

int main() {
    try {
        NotificationSettings s{false, true, true, true, false, true, false, true};
        NotificationsController n;
        n.module_toggle(s,"A",0,0,false,true);
        n.module_toggle(s,"B",0,0,false,true);
        n.module_toggle(s,"C",4,0,false,false);
        n.module_toggle(s,"hidden",0,0,true,true);
        n.module_toggle(s,"hold",0,2,false,true);
        n.module_toggle(s,"flagged",0,0x10,false,true);
        n.module_toggle(s,"move",1,0,false,true);
        n.module_toggle(s,"block",3,0,false,true);
        n.module_toggle(s,"visual",2,0,false,true);
        check(n.snapshot().empty(),"batch deferred until tick");
        check(n.tick(s,true,0.1f,8)==11,"animation and original flags");
        auto m=n.snapshot();
        check(m.size()==2 && m[0].text=="Enabled 3 mods" &&
              m[1].text=="Disabled C","batch ordering and filters");
        check(n.tick(s,true,0.2f)==1,"stable notification redraw");
        n.reset_session();
        n.profile(0,nullptr);
        std::string named="Demo";
        n.profile(1,&named);
        n.profile(2,&named);
        n.error(42,nullptr);
        n.friend_changed(true);
        n.friend_changed(false);
        m=n.snapshot();
        check(m.size()==5 && m[0].kind==NotificationKind::profile &&
              m[0].text=="Loaded profile Default" &&
              m[1].text=="Saved profile Demo" &&
              m[2].kind==NotificationKind::error &&
              m[2].text=="Error code 42" &&
              m[3].text=="Friend added" && m[4].text=="Friend removed",
              "message kinds and source strings");
        n.special_descriptor_toggle(true,"RSHIFT");
        n.special_descriptor_toggle(true,"RSHIFT");
        check(n.snapshot().size()==6 && n.snapshot().back().text==
              "Press RSHIFT to open GUI","hint once per session");
        n.special_descriptor_toggle(false,"RSHIFT");
        n.special_descriptor_toggle(true,"RSHIFT");
        check(n.snapshot().empty(),"disable clear and no repeated hint");
        for(int i=0;i<11;++i) n.error(i,nullptr);
        m=n.snapshot();
        check(m.size()==10 && m.front().text=="Error code 1" &&
              m.back().text=="Error code 10","bounded ring overwrite");
        check(n.tick(s,true,5.0f)==3 && n.snapshot().size()==10,
              "strict expire threshold and order");
        n.tick(s,true,0.25f);
        check(n.snapshot().size()==10,"expire before aging");
        check(n.tick(s,true,0.0f)==0 && n.snapshot().empty(),
              "remove expired oldest entries");
        s.hide_ingame=true;
        n.module_toggle(s,"A",0,0,false,true);
        n.friend_changed(true);
        check(n.tick(s,false,0.1f,8)==8 && n.snapshot().empty(),
              "hidden GUI clears active and pending");
        check(n.tick(s,true,0.1f)==0,"no stale batch on reopen");
        n.reset_session();
        n.special_descriptor_toggle(true,"K");
        check(n.snapshot().size()==1,"new session hint");
        bool rejected=false;
        try {n.tick(s,true,std::numeric_limits<float>::quiet_NaN());}
        catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && n.snapshot().front().age_seconds==0.0f,
              "invalid delta without mutation");
        rejected=false;
        try {n.profile(3,nullptr);}
        catch(const std::invalid_argument&) {rejected=true;}
        check(rejected && n.snapshot().size()==1,"unknown profile status");
        std::string long_message(200,'x');
        n.error(1,&long_message);
        check(n.snapshot().back().text.size()==100,"bounded message");
        std::cout<<"R310_NOTIFICATIONS_OFFLINE_QUEUE_PASS\n";
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<e.what()<<'\n';
        return 1;
    }
}
