#pragma once
// R186: independent, offline Ping Fix decision. No network I/O and no
// original DLL, packet implementation, JNI method or gameplay hook.
#include <optional>

namespace slinkywinky::reference_compat {
inline constexpr unsigned kIncomingPacketEvent = 2;
inline constexpr unsigned kOutgoingPacketEvent = 3;

enum class PingPacketClass : unsigned char {
    other,
    s00_server_keepalive,
    c00_client_keepalive
};

struct PingFixInput {
    PingPacketClass packet = PingPacketClass::other;
};

struct PingFixDecision {
    // Candidate state 3 represents a dropped event. This is only
    // a proposed numeric state, NEVER an actual packet discard.
    std::optional<int> proposed_packet_state{};
};

class PingFixController final {
public:
    PingFixDecision on_receive(const PingFixInput& input) const noexcept {
        if (input.packet == PingPacketClass::s00_server_keepalive)
            return {3};
        return {};
    }

    PingFixDecision on_send(const PingFixInput& input) const noexcept {
        if (input.packet == PingPacketClass::c00_client_keepalive)
            return {3};
        return {};
    }
};
} // namespace slinkywinky::reference_compat
