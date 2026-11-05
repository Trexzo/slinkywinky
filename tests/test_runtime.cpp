#include "slinkywinky/runtime.hpp"
#include <algorithm>
#include <atomic>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
using namespace slinkywinky;
#define CHECK(c) do { if(!(c)) throw std::runtime_error("FAIL line "+std::to_string(__LINE__)+": " #c); } while(false)
int main(){
    RuntimeMailbox m(16);
    CHECK(!m.begin_world()); CHECK(m.attach_owner()); CHECK(m.begin_world());
    const auto generation=m.generation();
    std::thread bad_owner([&]{ CHECK(!m.begin_world()); CHECK(m.drain().empty()); CHECK(!m.shutdown()); });
    bad_owner.join();
    constexpr int producers=4;
    std::atomic<int> admitted{0};
    std::vector<std::thread> workers;
    for(int t=0;t<producers;t++) workers.emplace_back([&]{
        for(int i=0;i<40;i++) if(m.submit(CommandKind::set_attack,static_cast<float>(i))) admitted++;
    });
    for(auto& w:workers) w.join();
    CHECK(admitted==16);
    auto commands=m.drain();
    CHECK(commands.size()==16);
    for(std::size_t i=1;i<commands.size();i++) CHECK(commands[i].sequence>commands[i-1].sequence);
    for(const auto& cmd:commands) CHECK(cmd.generation==generation);
    CHECK(m.submit(CommandKind::set_block));
    CHECK(m.world_changed()); CHECK(m.queued()==0);
    CHECK(!m.submit(CommandKind::set_block)); CHECK(m.begin_world());
    CHECK(m.generation()==generation+2);
    CHECK(m.submit(CommandKind::release_all));
    CHECK(m.shutdown()); CHECK(m.queued()==0);
    CHECK(!m.submit(CommandKind::set_attack));
    CHECK(m.attach_owner()); CHECK(m.begin_world()); CHECK(m.drain().empty());
    CHECK(m.shutdown());

    PacketHoldQueue p(3);
    CHECK(p.enqueue(1,10,42)); CHECK(p.enqueue(2,20,42)); CHECK(p.enqueue(3,30,42));
    CHECK(!p.enqueue(4,40,42));
    CHECK(p.expire(29,20,42).empty());
    auto due=p.expire(30,20,42); CHECK(due.size()==1 && due[0].identifier==1);
    CHECK(!p.enqueue(5,19,42));
    auto rest=p.release_all(42); CHECK(rest.size()==2);
    CHECK(p.size()==0);
    CHECK(p.enqueue(6,100,7));
    CHECK(p.expire(1000,0,8).empty()); CHECK(p.size()==0);
    std::cout<<"PASS runtime mailbox thread admission, bounded FIFO, generations, packet release\n";
}
