#include "slinkywinky/module_execution_bridge.hpp"
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;
static void ck(bool ok, const char* why, int line) {
    if (!ok) throw std::runtime_error("R187 " + std::to_string(line) + ": " + why);
}
#define CHECK(x) ck(static_cast<bool>(x), #x, __LINE__)

static NoItemReleaseInput item(int id, unsigned mode=0, bool stack=true,
                               bool item_present=true) {
    return {NoItemReleasePacketKind::c08_item_use,stack,item_present,id,0,mode};
}
static NoItemReleaseInput digging(int status=5, unsigned mode=0) {
    return {NoItemReleasePacketKind::c07_digging,false,false,-1,status,mode};
}
static ExecutionFrame outbound(NoItemReleaseInput input) {
    ExecutionFrame f;
    f.event=3; f.world_present=true; f.player_present=true;
    f.no_item_release=NoItemReleaseTickPayload{input,{},0};
    return f;
}
static ExecutionFrame shared(NoItemReleaseInput input) {
    ExecutionFrame f=outbound(input);
    const PingPacketClass p = input.packet==NoItemReleasePacketKind::c00_keepalive
                             ? PingPacketClass::c00_client_keepalive
                             : PingPacketClass::other;
    f.ping_fix=PingFixTickPayload{{p},{},0};
    return f;
}
int main() {
    NoItemReleaseController pure;
    auto d=pure.evaluate(item(267,0));
    CHECK(d.release_locked && !d.proposed_packet_state);
    d=pure.evaluate(digging());
    CHECK(d.proposed_packet_state==2 && !d.release_locked);
    CHECK(!pure.evaluate(digging()).proposed_packet_state);
    pure.evaluate(item(267,1));
    CHECK(!pure.release_locked()); // non-sword-only mode rejects swords
    pure.evaluate(item(260,1));
    CHECK(pure.release_locked());
    pure.evaluate(digging(1));
    CHECK(!pure.release_locked()); // any C07 consumes one-shot lock
    CHECK(!pure.evaluate(digging()).proposed_packet_state);
    for(int id : {261,346}) {
        pure.evaluate(item(id,2));
        CHECK(!pure.release_locked());
    }
    pure.evaluate(item(260,0));
    CHECK(!pure.release_locked());
    pure.evaluate(item(260,2));
    CHECK(pure.release_locked());
    pure.evaluate(item(267,2));
    CHECK(pure.release_locked());
    pure.evaluate(item(268,0));
    CHECK(pure.release_locked());
    pure.evaluate(item(283,0));
    CHECK(pure.release_locked());
    pure.evaluate(item(260,1,false,true));
    CHECK(!pure.release_locked());
    pure.evaluate(item(267,0));
    CHECK(pure.release_locked());
    pure.evaluate(item(267,0,true,false));
    CHECK(!pure.release_locked());
    pure.evaluate(item(267,0));
    const NoItemReleaseInput ordinary{};
    d=pure.evaluate(ordinary);
    CHECK(d.release_locked);
    d=pure.evaluate({NoItemReleasePacketKind::c00_keepalive});
    CHECK(d.release_locked);
    d=pure.evaluate(digging(5));
    CHECK(d.proposed_packet_state==2 && !d.release_locked);
    pure.evaluate(item(267));
    pure.reset();
    CHECK(!pure.release_locked());

    ModuleExecutionBridge bridge;
    const auto release=bridge.add({"No Item Release",4,{{3,0}},false});
    const auto ping=bridge.add({"Ping Fix",4,{{2,-1000},{3,-1000}},false});
    const auto unused=bridge.add({"Other Handler",4,{{3,-2000}},true});
    CHECK(release==0 && ping==1 && unused==2);
    CHECK(bridge.bind_no_item_release_from_catalog());
    CHECK(bridge.bind_ping_fix_from_catalog());
    CHECK(!bridge.bind_no_item_release(release));
    const auto counts=bridge.coverage();
    CHECK(counts.registered_modules==3);
    CHECK(counts.native_callback_subscriptions==4);
    CHECK(counts.modules_with_executable_bindings==2);
    CHECK(counts.executable_callback_bindings==3);
    CHECK(counts.unimplemented_callback_subscriptions==1);
    CHECK(counts.implemented_per_event[2]==1 && counts.implemented_per_event[3]==2);

    auto f=shared(item(267));
    CHECK(bridge.dispatch(f));
    CHECK(f.no_item_release->executions==0 && f.ping_fix->executions==0);
    CHECK(bridge.request(release,true));
    CHECK(bridge.request(ping,true));
    CHECK(!bridge.apply(false,false));
    CHECK(!bridge.apply(true,false));
    CHECK(bridge.apply(true,true));
    f=shared(item(267));
    CHECK(bridge.dispatch(f));
    CHECK(f.no_item_release->executions==1 && f.ping_fix->executions==1);
    CHECK(f.no_item_release->decision.release_locked);
    CHECK(!f.no_item_release->decision.proposed_packet_state);
    CHECK(!f.ping_fix->decision.proposed_packet_state);
    CHECK((f.executed_modules==std::vector<std::size_t>{release,ping}));

    // An unrelated outgoing keepalive goes to Ping Fix and must leave
    // No Item Release's one-shot state intact for the later C07.
    NoItemReleaseInput alive;
    alive.packet=NoItemReleasePacketKind::c00_keepalive;
    f=shared(alive);
    CHECK(bridge.dispatch(f));
    CHECK(f.no_item_release->decision.release_locked);
    CHECK(!f.no_item_release->decision.proposed_packet_state);
    CHECK(f.ping_fix->decision.proposed_packet_state==3);
    CHECK((f.executed_modules==std::vector<std::size_t>{release,ping}));

    f=shared(digging());
    CHECK(bridge.dispatch(f));
    CHECK(f.no_item_release->decision.proposed_packet_state==2);
    CHECK(!f.no_item_release->decision.release_locked);
    CHECK(!f.ping_fix->decision.proposed_packet_state);
    CHECK((f.executed_modules==std::vector<std::size_t>{release,ping}));
    f=shared(digging());
    CHECK(bridge.dispatch(f));
    CHECK(!f.no_item_release->decision.proposed_packet_state);

    // Direction agreement: event 3 packets can carry both payloads only
    // when their class labels describe the same outgoing packet.
    f=shared(item(267));
    f.ping_fix->input.packet=PingPacketClass::c00_client_keepalive;
    CHECK(!bridge.dispatch(f));
    f=shared(alive);
    f.ping_fix->input.packet=PingPacketClass::other;
    CHECK(!bridge.dispatch(f));
    f=shared(item(267));
    f.event=2;
    CHECK(!bridge.dispatch(f));
    f.event=3;
    f.world_present=false;
    CHECK(!bridge.dispatch(f));
    f.world_present=true;
    f.player_present=false;
    CHECK(!bridge.dispatch(f));
    f.player_present=true;
    f.jump_delay=JumpDelayTickPayload{};
    CHECK(!bridge.dispatch(f));
    f.jump_delay.reset();

    // Each module may be disabled independently.
    CHECK(bridge.request(release,false));
    CHECK(bridge.apply(true,true));
    f=shared(alive);
    CHECK(bridge.dispatch(f));
    CHECK(f.no_item_release->executions==0);
    CHECK(f.ping_fix->executions==1 &&
          f.ping_fix->decision.proposed_packet_state==3);
    CHECK((f.executed_modules==std::vector<std::size_t>{ping}));
    CHECK(bridge.request(ping,false));
    CHECK(bridge.apply(true,true));
    f=shared(alive);
    CHECK(bridge.dispatch(f));
    CHECK(f.no_item_release->executions==0 && f.ping_fix->executions==0);
    CHECK(f.executed_modules.empty());
    bridge.reset_no_item_release_world_state();
    CHECK(bridge.request(release,true));
    CHECK(bridge.apply(true,true));
    f=outbound(digging());
    CHECK(bridge.dispatch(f));
    CHECK(!f.no_item_release->decision.proposed_packet_state);
    bridge.stop();
    CHECK(!bridge.dispatch(f));
    CHECK(!bridge.bind_no_item_release_from_catalog());

    ModuleExecutionBridge bad_cat;
    auto id=bad_cat.add({"No Item Release",1,{{3,0}},true});
    CHECK(!bad_cat.bind_no_item_release(id));
    ModuleExecutionBridge bad_event;
    id=bad_event.add({"No Item Release",4,{{2,0}},true});
    CHECK(!bad_event.bind_no_item_release(id));
    ModuleExecutionBridge bad_priority;
    id=bad_priority.add({"No Item Release",4,{{3,-1000}},true});
    CHECK(!bad_priority.bind_no_item_release(id));

    std::cout<<"R187 No Item Release PASS: mode classes, one-shot state, "
               "event-3 priority with Ping Fix, packet direction admission\n";
}
