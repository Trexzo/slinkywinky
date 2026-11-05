#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <thread>

namespace slinkywinky {

struct ObservationFrame {
    std::uint64_t time_ms = 0;
    std::uint64_t generation = 0;
    std::uint32_t flags = 0; // world = 1, player = 2, ray target = 4
    std::array<double, 6> xyz{}; // eye x/y/z, ray target x/y/z
    std::array<float, 2> angles{}; // yaw, pitch
};

struct ObservationStatus {
    std::uint64_t valid_ticks = 0;
    std::uint64_t session_transitions = 0;
    std::uint64_t generation = 0;
    std::uint64_t time_ms = 0;
    std::uint32_t flags = 0;
    double target_distance = 0.0; // only ray-selected distance; not reach eligibility
    double eye_y = 0.0;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

// Read-only per-session state for future native HUD and diagnostics.
// No Minecraft pointers, input controls, packets, rendering or gameplay decisions.
class ObservationLedger {
public:
    ObservationLedger() noexcept;
    ObservationLedger(const ObservationLedger&) = delete;
    ObservationLedger& operator=(const ObservationLedger&) = delete;

    bool can_accept(const ObservationFrame& frame) const noexcept;
    // Must be called only for a frame already accepted and successfully processed
    // by the native zero-action engine. Never call after a rejected frame.
    void commit(const ObservationFrame& frame) noexcept;
    std::optional<ObservationStatus> snapshot() const noexcept;
private:
    const std::thread::id owner_;
    ObservationStatus status_{};
};

} // namespace slinkywinky
