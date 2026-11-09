#include "slinkywinky/module_execution_bridge.hpp"

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
using namespace slinkywinky::reference_compat;

static void assert_condition(bool value, const char* reason, int line) {
    if (!value)
        throw std::runtime_error(std::string("R186 line ") +
                                 std::to_string(line) + ": " + reason);
}
#define CHECK(expr) assert_condition(static_cast<bool>(expr), #expr, __LINE__)

static ExecutionFrame packet(unsigned event, PingPacketClass cls) {
    ExecutionFrame frame;
    frame.event = event;
    frame.world_present = true;
    frame.player_present = true;
    frame.ping_fix = PingFixTickPayload{{cls}, {}, 0};
    return frame;
}

int main() {
    PingFixController policy;
    CHECK(policy.on_receive({PingPacketClass::s00_server_keepalive})
              .proposed_packet_state == 3);
    CHECK(!policy.on_receive({PingPacketClass::c00_client_keepalive})
              .proposed_packet_state);
    CHECK(!policy.on_receive({PingPacketClass::other})
              .proposed_packet_state);
    CHECK(policy.on_send({PingPacketClass::c00_client_keepalive})
              .proposed_packet_state == 3);
    CHECK(!policy.on_send({PingPacketClass::s00_server_keepalive})
              .proposed_packet_state);
    CHECK(!policy.on_send({PingPacketClass::other})
              .proposed_packet_state);

    ModuleExecutionBridge bridge;
    const auto ping = bridge.add({"Ping Fix", 4, {{2, -1000}, {3, -1000}}, false});
    const auto prior = bridge.add({"Incoming Observer", 2, {{2, 100}}, true});
    const auto unbound = bridge.add({"Other Packet Handler", 4, {{3, 0}}, true});
    CHECK(ping == 0 && prior == 1 && unbound == 2);

    unsigned prior_calls = 0;
    CHECK(bridge.bind(prior, kIncomingPacketEvent,
        [&](const Delivery& d, ExecutionFrame&) {
            CHECK(d.event == 2 && d.recipient == prior);
            ++prior_calls;
        }));
    CHECK(bridge.bind_ping_fix_from_catalog());
    CHECK(!bridge.bind_ping_fix(ping)); // no partial/double registration
    const auto coverage = bridge.coverage();
    CHECK(coverage.registered_modules == 3);
    CHECK(coverage.native_callback_subscriptions == 4);
    CHECK(coverage.modules_with_executable_bindings == 2);
    CHECK(coverage.executable_callback_bindings == 3);
    CHECK(coverage.unimplemented_callback_subscriptions == 1);
    CHECK(coverage.implemented_per_event[2] == 2);
    CHECK(coverage.implemented_per_event[3] == 1);

    // Before enabling, only the explicitly bound observer executes.
    auto frame = packet(2, PingPacketClass::s00_server_keepalive);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.ping_fix->executions == 0);
    CHECK(!frame.ping_fix->decision.proposed_packet_state);
    CHECK((frame.executed_modules == std::vector<std::size_t>{prior}));
    CHECK(prior_calls == 1);

    CHECK(bridge.request(ping, true));
    CHECK(!bridge.apply(false, false));
    CHECK(!bridge.apply(true, false));
    CHECK(!bridge.enabled(ping));
    CHECK(bridge.apply(true, true));
    CHECK(bridge.enabled(ping));

    frame = packet(2, PingPacketClass::s00_server_keepalive);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.ping_fix->executions == 1);
    CHECK(frame.ping_fix->decision.proposed_packet_state == 3);
    CHECK((frame.executed_modules == std::vector<std::size_t>{prior, ping}));
    CHECK(prior_calls == 2);

    frame = packet(3, PingPacketClass::c00_client_keepalive);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.ping_fix->executions == 1);
    CHECK(frame.ping_fix->decision.proposed_packet_state == 3);
    CHECK((frame.executed_modules == std::vector<std::size_t>{ping}));

    // Only the correct packet class for the current direction is selected.
    frame = packet(2, PingPacketClass::c00_client_keepalive);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.ping_fix->executions == 1);
    CHECK(!frame.ping_fix->decision.proposed_packet_state);
    frame = packet(3, PingPacketClass::s00_server_keepalive);
    CHECK(bridge.dispatch(frame));
    CHECK(!frame.ping_fix->decision.proposed_packet_state);
    frame = packet(2, PingPacketClass::other);
    CHECK(bridge.dispatch(frame));
    CHECK(!frame.ping_fix->decision.proposed_packet_state);
    frame = packet(3, PingPacketClass::other);
    CHECK(bridge.dispatch(frame));
    CHECK(!frame.ping_fix->decision.proposed_packet_state);

    frame = packet(2, PingPacketClass::s00_server_keepalive);
    frame.world_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.world_present = true;
    frame.player_present = false;
    CHECK(!bridge.dispatch(frame));
    frame.player_present = true;
    frame.event = 4;
    CHECK(!bridge.dispatch(frame));
    frame.event = 2;
    frame.no_hit_delay = NoHitDelayTickPayload{};
    CHECK(!bridge.dispatch(frame));
    frame.no_hit_delay.reset();

    CHECK(bridge.request(ping, false));
    CHECK(bridge.apply(true, true));
    frame = packet(2, PingPacketClass::s00_server_keepalive);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.ping_fix->executions == 0);
    CHECK(!frame.ping_fix->decision.proposed_packet_state);
    CHECK((frame.executed_modules == std::vector<std::size_t>{prior}));
    frame = packet(3, PingPacketClass::c00_client_keepalive);
    CHECK(bridge.dispatch(frame));
    CHECK(frame.ping_fix->executions == 0);
    CHECK(frame.executed_modules.empty());

    bridge.stop();
    CHECK(!bridge.dispatch(frame));
    CHECK(!bridge.bind_ping_fix_from_catalog());

    ModuleExecutionBridge wrong_category;
    auto id = wrong_category.add({"Ping Fix", 0, {{2, -1000}, {3, -1000}}, true});
    CHECK(!wrong_category.bind_ping_fix(id));
    ModuleExecutionBridge wrong_receive;
    id = wrong_receive.add({"Ping Fix", 4, {{1, -1000}, {3, -1000}}, true});
    CHECK(!wrong_receive.bind_ping_fix(id));
    ModuleExecutionBridge wrong_send;
    id = wrong_send.add({"Ping Fix", 4, {{2, -1000}, {4, -1000}}, true});
    CHECK(!wrong_send.bind_ping_fix(id));
    ModuleExecutionBridge wrong_priority;
    id = wrong_priority.add({"Ping Fix", 4, {{2, -1000}, {3, -999}}, true});
    CHECK(!wrong_priority.bind_ping_fix(id));
    ModuleExecutionBridge missing_receive;
    id = missing_receive.add({"Ping Fix", 4, {{3, -1000}}, true});
    CHECK(!missing_receive.bind_ping_fix(id));
    CHECK(missing_receive.coverage().executable_callback_bindings == 0);

    std::cout << "R186 Ping Fix PASS: separate incoming/outgoing event callbacks, "
                 "both packet classes, priority -1000, module lifecycle, shutdown\n";
    return 0;
}
