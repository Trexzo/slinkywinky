#include "slinkywinky/observation.hpp"
#include <cmath>
#include <limits>

namespace slinkywinky {

ObservationLedger::ObservationLedger() noexcept : owner_(std::this_thread::get_id()) {}

bool ObservationLedger::can_accept(const ObservationFrame& f) const noexcept {
    if (owner_ != std::this_thread::get_id() ||
        f.generation == 0 || (f.flags & ~7u) != 0 ||
        ((f.flags & 2u) != 0 && (f.flags & 1u) == 0) ||
        ((f.flags & 4u) != 0 && (f.flags & 3u) != 3u))
        return false;
    for (const double x : f.xyz)
        if (!std::isfinite(x)) return false;
    for (const float x : f.angles)
        if (!std::isfinite(x)) return false;
    if (status_.valid_ticks != 0) {
        if (f.time_ms < status_.time_ms || f.generation < status_.generation ||
            (f.generation == status_.generation &&
             ((f.flags ^ status_.flags) & 3u) != 0))
            return false;
        if (status_.valid_ticks == std::numeric_limits<std::uint64_t>::max() ||
            (f.generation != status_.generation &&
             status_.session_transitions == std::numeric_limits<std::uint64_t>::max()))
            return false;
    }
    if ((f.flags & 4u) != 0) {
        const auto distance = std::hypot(
            f.xyz[3] - f.xyz[0], f.xyz[4] - f.xyz[1], f.xyz[5] - f.xyz[2]);
        if (!std::isfinite(distance)) return false;
    }
    return true;
}

void ObservationLedger::commit(const ObservationFrame& f) noexcept {
    // Caller has already checked can_accept and finished a zero-action engine step.
    if (status_.valid_ticks != 0 && f.generation != status_.generation)
        ++status_.session_transitions;
    ++status_.valid_ticks;
    status_.generation = f.generation;
    status_.time_ms = f.time_ms;
    status_.flags = f.flags;
    status_.eye_y = f.xyz[1];
    status_.yaw = f.angles[0];
    status_.pitch = f.angles[1];
    status_.target_distance = (f.flags & 4u) ?
        std::hypot(f.xyz[3] - f.xyz[0], f.xyz[4] - f.xyz[1], f.xyz[5] - f.xyz[2]) : 0.0;
}

std::optional<ObservationStatus> ObservationLedger::snapshot() const noexcept {
    if (owner_ != std::this_thread::get_id()) return std::nullopt;
    return status_;
}

} // namespace slinkywinky
