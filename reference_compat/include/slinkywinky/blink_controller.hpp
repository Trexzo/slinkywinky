#pragma once
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace slinkywinky::reference_compat {
enum class BlinkDirection : unsigned { incoming = 0, outgoing = 1, both = 2 };
enum class BlinkPacketKind { other, entity_status, update_health, sound_effect, particles };
enum class BlinkVerdict { unchanged, hold, pass };

struct BlinkSettings {
    float max_duration = 20.0f;
    BlinkDirection direction = BlinkDirection::both;
    bool disable_on_attack = false;
    bool disable_on_block_dig = false;
    bool disable_on_block_interact = false;
};

// A value-only implementation of the candidate Blink callbacks.
// The caller, not this controller, owns any packet queue or module toggles.
class BlinkController {
public:
    void enable(const BlinkSettings& s) {
        if (!std::isfinite(s.max_duration) || s.max_duration < 0.0f ||
            s.max_duration > static_cast<float>(std::numeric_limits<int>::max() / 2) ||
            static_cast<unsigned>(s.direction) > 2u)
            throw std::invalid_argument("invalid Blink settings");
        settings_ = s;
        ticks_left_ = static_cast<int>(s.max_duration);
        last_attacker_ = std::numeric_limits<int>::max(); // source startup sentinel
        active_ = true;
    }
    void disable() noexcept {
        active_ = false;
        ticks_left_ = 0;
        last_attacker_ = std::numeric_limits<int>::max();
    }
    void reset_world() noexcept { disable(); }
    bool enabled() const noexcept { return active_; }
    int ticks_left() const noexcept { return ticks_left_; }
    // Returns a disable proposal; does not silently change the toggle state.
    bool tick(bool in_game, int last_attacker_time) noexcept {
        if (!active_ || !in_game) return false;
        const bool attacked = last_attacker_time > last_attacker_;
        last_attacker_ = last_attacker_time;
        if (ticks_left_ > std::numeric_limits<int>::min()) --ticks_left_;
        return ticks_left_ <= 0 || (attacked && settings_.disable_on_attack);
    }
    bool block_action(bool is_dig, bool ray_hits_block) const noexcept {
        if (!active_ || !ray_hits_block) return false;
        return is_dig ? settings_.disable_on_block_dig
                      : settings_.disable_on_block_interact;
    }
    BlinkVerdict incoming(BlinkPacketKind packet) const noexcept {
        if (!active_ || settings_.direction == BlinkDirection::outgoing)
            return BlinkVerdict::unchanged;
        switch (packet) {
            case BlinkPacketKind::entity_status:
            case BlinkPacketKind::update_health:
            case BlinkPacketKind::sound_effect:
            case BlinkPacketKind::particles: return BlinkVerdict::pass;
            default: return BlinkVerdict::hold;
        }
    }
    BlinkVerdict outgoing() const noexcept {
        if (!active_ || settings_.direction == BlinkDirection::incoming)
            return BlinkVerdict::unchanged;
        return BlinkVerdict::hold;
    }
private:
    BlinkSettings settings_{};
    bool active_ = false;
    int ticks_left_ = 0;
    int last_attacker_ = std::numeric_limits<int>::max();
};
} // namespace slinkywinky::reference_compat
