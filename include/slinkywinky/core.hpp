#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace slinkywinky {

// Values are candidate-reference hypotheses, not official provenance proof.
struct ReferenceProfile {
    float reach_distance = 3.05f;
    float reach_activation_ticks = 3.0f;
    float click_target_cps = 12.0f;
    float aim_horizontal_speed = 15.0f;
    float aim_randomization = 0.5f;
    float backtrack_delay_ms = 100.0f;
};

struct ReachInput {
    bool sprinting = false;
    int last_attacker_time = 0;
};

struct ReachSettings {
    float distance = 3.05f;
    float activation_ticks = 3.0f;
    bool require_sprint = false;
};

// Pure decision model: does not touch the JVM, Minecraft or a process.
class ReachController {
public:
    std::optional<double> evaluate(const ReachInput& input, const ReachSettings& settings);
    void reset() noexcept;
    int cooldown_ticks() const noexcept { return cooldown_ticks_; }
private:
    int cooldown_ticks_ = 0;
    int previous_attacker_time_ = 0;
};

// Deterministic xoshiro128+ reference sequence; explicit seed required.
class Random128 {
public:
    explicit Random128(std::array<std::uint32_t, 4> seed);
    std::uint32_t next() noexcept;
    float unit_float() noexcept;
private:
    std::array<std::uint32_t, 4> state_;
};

struct RotationRequest {
    std::string source;
    int priority = 0;
    float yaw = 0;
    float pitch = 0;
};

// One rotation owner per frame; equal priorities keep earliest submitter.
class RotationArbiter {
public:
    void start_frame() noexcept { winner_.reset(); }
    bool submit(const RotationRequest& request);
    const std::optional<RotationRequest>& winner() const noexcept { return winner_; }
private:
    std::optional<RotationRequest> winner_;
};

enum class KeyEdge { none, press, release };

// Manual input and synthetic ownership are kept separately to avoid sticky keys.
class InputOwnership {
public:
    KeyEdge update(bool manual_down, bool synthetic_down) noexcept;
    KeyEdge disable_synthetic(bool manual_down) noexcept { return update(manual_down, false); }
    bool effective_down() const noexcept { return effective_down_; }
private:
    bool effective_down_ = false;
};

} // namespace slinkywinky
