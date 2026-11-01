#include "slinkywinky/hud_snapshot.hpp"
#include <cmath>
#include <limits>

namespace slinkywinky {
HudSnapshotMailbox::HudSnapshotMailbox() noexcept : owner_(std::this_thread::get_id()) {}

bool HudSnapshotMailbox::publish(const ObservationStatus& s) noexcept {
    if (owner_ != std::this_thread::get_id() ||
        s.valid_ticks == 0 || s.generation == 0 ||
        (s.flags & ~7u) != 0 ||
        ((s.flags & 2u) && !(s.flags & 1u)) ||
        ((s.flags & 4u) && (s.flags & 3u) != 3u) ||
        !std::isfinite(s.target_distance) || s.target_distance < 0 ||
        !std::isfinite(s.eye_y) || !std::isfinite(s.yaw) ||
        !std::isfinite(s.pitch))
        return false;

    std::lock_guard<std::mutex> lock(mutex_);
    if (revoked_) return false;
    std::uint64_t sequence=1;
    if (latest_) {
        const auto& previous=*latest_;
        if (previous.sequence == std::numeric_limits<std::uint64_t>::max() ||
            s.valid_ticks <= previous.status.valid_ticks ||
            s.time_ms < previous.status.time_ms ||
            s.generation < previous.status.generation)
            return false;
        sequence=previous.sequence+1;
    }
    latest_=PublishedObservation{s,sequence};
    return true;
}

std::optional<PublishedObservation> HudSnapshotMailbox::read() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if (revoked_) return std::nullopt;
    return latest_;
}

bool HudSnapshotMailbox::revoke() noexcept {
    if (owner_ != std::this_thread::get_id()) return false;
    std::lock_guard<std::mutex> lock(mutex_);
    if (revoked_) return false;
    revoked_=true;
    latest_.reset();
    return true;
}
} // namespace slinkywinky
