#include "slinkywinky/core.hpp"

#include <cmath>
#include <stdexcept>

namespace slinkywinky {

std::optional<double> ReachController::evaluate(const ReachInput& input,
                                                 const ReachSettings& settings) {
    if (cooldown_ticks_ > 0) --cooldown_ticks_;
    if (settings.require_sprint && !input.sprinting) return std::nullopt;
    if (cooldown_ticks_ == 0 && input.last_attacker_time != previous_attacker_time_) {
        previous_attacker_time_ = input.last_attacker_time;
        cooldown_ticks_ = 9;
    }
    if (!std::isfinite(settings.distance) || !std::isfinite(settings.activation_ticks) ||
        settings.distance < 0.0f) return std::nullopt;
    if (settings.activation_ticks > static_cast<float>(cooldown_ticks_)) {
        return static_cast<double>(settings.distance);
    }
    return std::nullopt;
}

void ReachController::reset() noexcept {
    cooldown_ticks_ = 0;
    previous_attacker_time_ = 0;
}

Random128::Random128(std::array<std::uint32_t, 4> seed) : state_(seed) {
    if ((seed[0] | seed[1] | seed[2] | seed[3]) == 0)
        throw std::invalid_argument("Random128 requires nonzero state");
}

std::uint32_t Random128::next() noexcept {
    const std::uint32_t result = state_[0] + state_[3];
    const std::uint32_t t = state_[1] << 9;
    state_[2] ^= state_[0];
    state_[3] ^= state_[1];
    state_[1] ^= state_[2];
    state_[0] ^= state_[3];
    state_[2] ^= t;
    state_[3] = (state_[3] << 11) | (state_[3] >> 21);
    return result;
}

float Random128::unit_float() noexcept {
    return static_cast<float>(next() >> 8) * 0x1p-24f;
}

bool RotationArbiter::submit(const RotationRequest& request) {
    if (request.source.empty() || !std::isfinite(request.yaw) ||
        !std::isfinite(request.pitch)) return false;
    if (!winner_ || request.priority > winner_->priority) {
        winner_ = request;
        return true;
    }
    return false;
}

KeyEdge InputOwnership::update(bool manual_down, bool synthetic_down) noexcept {
    const bool next = manual_down || synthetic_down;
    if (next == effective_down_) return KeyEdge::none;
    effective_down_ = next;
    return next ? KeyEdge::press : KeyEdge::release;
}

} // namespace slinkywinky
