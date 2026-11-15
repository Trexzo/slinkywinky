#pragma once
// R184: first-party offline No Use Delay decision. It never writes game
// key states or calls Java/JNI; the original candidate is not executed.
#include <optional>

namespace slinkywinky::reference_compat {
inline constexpr unsigned kNoUseDelayEvent = 18;

struct NoUseDelayInput {
    bool in_game = false;
    bool use_key_down = false;      // effective state sampled before slot check
    bool physical_use_held = false; // physical key held, used on slot restoration
    int selected_slot = 0;
    int use_ticks_left = 0;
    bool allow_desync = false;
};

struct NoUseDelayDecision {
    std::optional<bool> proposed_use_key_pressed{};
    int tracked_remaining_ticks = 0;
    int tracked_slot = 0;
};

// Offline model of the observed candidate decision sequence. In particular,
// a slot change restores the physically held key before the pre-sampled
// effective use-key check; early desync release is optional.
class NoUseDelayController final {
public:
    NoUseDelayDecision evaluate(const NoUseDelayInput& input) noexcept {
        NoUseDelayDecision result;
        if (!input.in_game) {
            remaining_ticks_ = 0;
            return snapshot(result);
        }
        if (input.selected_slot != last_slot_) {
            remaining_ticks_ = 0;
            last_slot_ = input.selected_slot;
            result.proposed_use_key_pressed = input.physical_use_held;
        }
        if (!input.use_key_down) return snapshot(result);
        if (remaining_ticks_ <= 0) {
            remaining_ticks_ = input.use_ticks_left;
            return snapshot(result);
        }
        --remaining_ticks_;
        if (remaining_ticks_ == 0 ||
            (input.use_ticks_left == 0 && input.allow_desync)) {
            result.proposed_use_key_pressed = false;
        }
        return snapshot(result);
    }

    void reset() noexcept {
        remaining_ticks_ = 0;
        last_slot_ = 0;
    }
    int remaining_ticks() const noexcept { return remaining_ticks_; }
    int last_slot() const noexcept { return last_slot_; }

private:
    NoUseDelayDecision snapshot(NoUseDelayDecision value) const noexcept {
        value.tracked_remaining_ticks = remaining_ticks_;
        value.tracked_slot = last_slot_;
        return value;
    }
    int remaining_ticks_ = 0;
    int last_slot_ = 0;
};
} // namespace slinkywinky::reference_compat
