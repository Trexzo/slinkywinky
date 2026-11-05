#pragma once
#include "slinkywinky/combat.hpp"
#include "slinkywinky/settings.hpp"
#include <array>
#include <vector>

namespace slinkywinky {
struct HostBatch {
    CombatResult decision;
    std::vector<Command> commands;
    std::vector<PacketNotice> released_packet_notices;
    bool emergency_release = false;
    bool valid = false;
};

// Glue layer for an *explicitly authorized* host, not an injector. The host
// must apply decisions on the exact Minecraft client thread. No game API calls.
class HostSession {
public:
    explicit HostSession(std::array<std::uint32_t,4> seed);
    bool attach();
    bool start_world();
    HostBatch tick(TickSnapshot observation, const ClientSettings& settings);
    bool queue_packet_notice(std::uint32_t id, std::uint64_t received_ms);
    HostBatch leave_world(bool physical_left, bool physical_right);
    HostBatch shutdown(bool physical_left, bool physical_right);
    RuntimeMailbox& mailbox() noexcept { return mailbox_; }
private:
    RuntimeMailbox mailbox_{512};
    CombatCoordinator coordinator_;
    PacketHoldQueue held_{128};
    HostBatch release_state(bool physical_left, bool physical_right);
};
} // namespace slinkywinky
