#include "slinkywinky/session.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
using namespace slinkywinky;
#define CHECK(x) do {if(!(x))throw std::runtime_error("FAIL line "+std::to_string(__LINE__)+" " #x);}while(false)
int main(){
    HostSession session({5,7,11,13});
    TickSnapshot snap;
    ClientSettings settings;
    settings.combat.auto_clicker_enabled=true;
    settings.combat.require_mouse_down=false;
    settings.combat.aim_enabled=true;
    settings.combat.aim_range=5;
    settings.backtrack_enabled=true;
    settings.backtrack_ms=100;
    CHECK(session.attach());CHECK(session.start_world());
    snap.world_valid=true;snap.generation=session.mailbox().generation();
    snap.holding_weapon=true;snap.elapsed_ms=100;
    snap.target=Target{1,{2,0,0},true,true};
    CHECK(session.queue_packet_notice(50,90));
    auto batch=session.tick(snap,settings);
    CHECK(batch.valid);CHECK(batch.decision.attempt_attack);
    CHECK(batch.released_packet_notices.empty());
    CHECK(!batch.commands.empty());
    snap.elapsed_ms=191;
    batch=session.tick(snap,settings);
    CHECK(batch.valid && batch.released_packet_notices.size()==1);
    CHECK(batch.released_packet_notices[0].identifier==50);
    CHECK(session.queue_packet_notice(60,192));
    settings.backtrack_enabled=false;
    snap.elapsed_ms=200;
    batch=session.tick(snap,settings);
    CHECK(batch.valid && batch.released_packet_notices.size()==1);
    CHECK(session.queue_packet_notice(61,201));
    batch=session.leave_world(false,false);
    CHECK(batch.valid && batch.emergency_release);
    CHECK(batch.commands.size()==1 && batch.commands[0].kind==CommandKind::release_all);
    CHECK(!session.queue_packet_notice(63,202));
    CHECK(session.start_world());
    snap.generation=session.mailbox().generation();snap.elapsed_ms=1000;
    batch=session.tick(snap,settings);
    CHECK(batch.valid && batch.released_packet_notices.empty());
    snap.generation--;
    batch=session.tick(snap,settings);
    CHECK(!batch.valid && batch.emergency_release);
    snap.generation=session.mailbox().generation();
    int thread_failed=-100;
    std::thread worker([&]{thread_failed=session.leave_world(false,false).valid?1:0;});
    worker.join();CHECK(thread_failed==0);
    batch=session.shutdown(false,false);
    CHECK(batch.valid && session.mailbox().phase()==RuntimePhase::stopped);
    CHECK(!session.start_world());
    std::cout<<"PASS host session tick, combat dispatch, packet expiry, world fencing, shutdown\n";
}
