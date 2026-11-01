#pragma once
#include "slinkywinky/observation.hpp"
#include <cstdint>
#include <mutex>
#include <optional>
#include <thread>

namespace slinkywinky {

struct PublishedObservation {
    ObservationStatus status;
    std::uint64_t sequence = 0;
};

// In-process, read-only, fixed-capacity client-thread -> render-thread handoff.
// The owning game thread publishes a copy only AFTER the C++ engine accepts a
// zero-action frame. Other threads can read coherent copies; no JVM pointers or
// render hooks are stored here. The owner must revoke the mailbox on shutdown.
class HudSnapshotMailbox {
public:
    HudSnapshotMailbox() noexcept;
    HudSnapshotMailbox(const HudSnapshotMailbox&) = delete;
    HudSnapshotMailbox& operator=(const HudSnapshotMailbox&) = delete;

    bool publish(const ObservationStatus& status) noexcept;
    std::optional<PublishedObservation> read() const noexcept;
    bool revoke() noexcept;
private:
    const std::thread::id owner_;
    mutable std::mutex mutex_;
    std::optional<PublishedObservation> latest_;
    bool revoked_ = false;
};

} // namespace slinkywinky
