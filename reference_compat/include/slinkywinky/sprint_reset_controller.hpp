#pragma once
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace slinkywinky::reference_compat {
enum class SprintResetStage { idle, waiting, key_changed };
struct SprintResetSettings {
    unsigned mode = 0; // 0 forward, 1 sneak, 2 sprint
    float delay = 0;
    float duration = 1;
    bool randomize = false;
    bool wait_for_damage = false;
    bool require_weapon = false;
};
struct SprintResetFrame {
    bool in_game = false;
    double seconds = 0;
    int player_ticks = 0;
    int last_attacker_time = 0;
    int hurt_time = 0;
    bool weapon = false;
    bool physically_pressed = false;
    // Input randomness, not invented RNG. Used only when randomize is enabled.
    float extra_delay = 0, duration_reduction = 0, duration_extension = 0;
};
struct SprintResetProposal {
    bool change_key = false;
    bool pressed = false;
    bool change_sprinting = false;
    bool sprinting = false;
    unsigned key_mode = 0;
};

class SprintResetController {
public:
    void configure(SprintResetSettings s) {
        if (s.mode > 2u || !std::isfinite(s.delay) || !std::isfinite(s.duration) ||
            s.delay < 0 || s.duration < 0 || s.delay > 10000 || s.duration > 10000)
            throw std::invalid_argument("invalid Sprint Reset settings");
        settings_ = s;
    }
    void reset_world() noexcept {
        stage_ = SprintResetStage::idle;
        countdown_ = 0;
        hit_seen_ = false;
        ticks_since_hurt_ = 0;
        last_hit_seconds_ = -1e12;
    }
    SprintResetProposal step(const SprintResetFrame& frame) {
        if (!std::isfinite(frame.seconds) || (settings_.randomize &&
            (!std::isfinite(frame.extra_delay) || !std::isfinite(frame.duration_reduction) ||
             !std::isfinite(frame.duration_extension))))
            throw std::invalid_argument("invalid Sprint Reset frame");
        if (!frame.in_game) {
            const bool restore = stage_ == SprintResetStage::key_changed;
            reset_world();
            return restore ? key_proposal(false, frame.physically_pressed)
                           : SprintResetProposal{};
        }
        if (frame.seconds - last_hit_seconds_ >= 0.5 &&
            frame.player_ticks == frame.last_attacker_time) {
            last_hit_seconds_ = frame.seconds;
            hit_seen_ = true;
        }
        if (ticks_since_hurt_ < std::numeric_limits<int>::max())
            ++ticks_since_hurt_;
        if (frame.hurt_time == 10) ticks_since_hurt_ = 0;
        if (countdown_ > 0) {
            countdown_ -= 1;
            if (countdown_ > 0) {
                if (stage_ == SprintResetStage::key_changed)
                    return key_proposal(true, frame.physically_pressed);
                return {};
            }
        }
        if (stage_ == SprintResetStage::key_changed) {
            stage_ = SprintResetStage::idle;
            return key_proposal(false, frame.physically_pressed);
        }
        if (stage_ == SprintResetStage::waiting) {
            stage_ = SprintResetStage::key_changed;
            hit_seen_ = false;
            const float reduction = settings_.randomize ?
                std::clamp(frame.duration_reduction, 0.0f, 1.0f) : 0.0f;
            float duration = std::max(0.0f, settings_.duration - reduction);
            if (settings_.randomize)
                duration += std::clamp(frame.duration_extension, 0.0f, 2.0f);
            countdown_ += duration;
            return key_proposal(true, frame.physically_pressed);
        }
        if (hit_seen_ && (!settings_.wait_for_damage || ticks_since_hurt_ == 0) &&
            (!settings_.require_weapon || frame.weapon)) {
            stage_ = SprintResetStage::waiting;
            countdown_ += settings_.delay +
                (settings_.randomize ? std::clamp(frame.extra_delay, 0.0f, 2.0f) : 0.0f);
        }
        return {};
    }
    SprintResetStage stage() const noexcept { return stage_; }
    float countdown() const noexcept { return countdown_; }
    bool pending_hit() const noexcept { return hit_seen_; }
private:
    SprintResetProposal key_proposal(bool active, bool physical) const noexcept {
        return {true, active ? settings_.mode == 1u : physical,
                settings_.mode == 2u, !active, settings_.mode};
    }
    SprintResetSettings settings_{};
    SprintResetStage stage_ = SprintResetStage::idle;
    float countdown_ = 0;
    bool hit_seen_ = false;
    int ticks_since_hurt_ = 0;
    double last_hit_seconds_ = -1e12;
};
} // namespace slinkywinky::reference_compat
