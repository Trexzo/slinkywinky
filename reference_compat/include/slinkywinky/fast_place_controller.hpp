#pragma once
// R225 first-party Fast Place native policy, reconstructed independently
// from pinned OpenSlinky source semantics. Event 0 / priority -1000,
// block category 3. This model ONLY proposes extra use-key press counts;
// it never posts synthetic input, changes key state, or writes Minecraft.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

namespace slinkywinky::reference_compat {

struct FastPlaceInput {
    bool in_game = false;
    bool use_key_held = false;
    bool block_item_held = false;
    bool use_press_already_pending = false;
    bool target_is_block = false;
    bool adjacent_block_overlaps_player = false;
    bool item_is_ladder = false;
    bool random_bypass_granted = false; // caller-controlled deterministic fixture
    float max_cps = 10.0f;
    float activation_delay_ticks = 0.0f;
};

struct FastPlaceDecision {
    unsigned extra_use_press_count = 0; // PROPOSAL only
    bool admitted = false;
    float timer_after = 0.0f;
    int delay_remaining = 0;
};

class FastPlaceController final {
public:
    void reset() noexcept {
        timer_ = 0.0f;
        was_held_ = false;
        delay_remaining_ = 0;
    }
    FastPlaceDecision evaluate(const FastPlaceInput& input) noexcept {
        FastPlaceDecision out{};
        // R225 intentional bounds: prevent original's unchecked reciprocal
        // from producing infinite / unbounded click loops on bad settings.
        if (!std::isfinite(input.max_cps) ||
            !std::isfinite(input.activation_delay_ticks) ||
            input.max_cps < 1.0f || input.max_cps > 100.0f ||
            input.activation_delay_ticks < 0.0f ||
            input.activation_delay_ticks > 200.0f) return out;
        out.admitted = true;
        auto finish = [&]() noexcept {
            out.timer_after = timer_;
            out.delay_remaining = delay_remaining_;
            return out;
        };
        // The pinned native tick first decrements its signed click timer;
        // if it remains positive it returns without polling the key.
        if (timer_ > 0.0f) {
            timer_ -= 1.0f;
            if (timer_ > 0.0f) return finish();
        }
        if (!input.in_game) return finish();
        const bool previous = was_held_;
        was_held_ = input.use_key_held;
        if (!input.use_key_held) {
            delay_remaining_ = static_cast<int>(input.activation_delay_ticks);
            return finish();
        }
        if (delay_remaining_ > 0) {
            --delay_remaining_;
            return finish();
        }
        if (input.use_press_already_pending || timer_ > 0.0f)
            return finish();

        // First physical/vanilla use press already has its own click.
        // Subsequent scheduled slots may propose an extra press, provided
        // held item is a block and block placement is geometrically valid.
        bool first_press = !previous;
        const float interval = 20.0f / input.max_cps;
        // Max 100 CPS => at most six slots per tick, including first press;
        // bounded loop is an intentional safety deviation from upstream.
        for (unsigned slot = 0; slot < 6; ++slot) {
            timer_ += interval;
            if (!first_press && input.block_item_held &&
                (input.item_is_ladder ||
                 (input.target_is_block &&
                  !input.adjacent_block_overlaps_player) ||
                 input.random_bypass_granted)) {
                ++out.extra_use_press_count;
            }
            first_press = false;
            if (timer_ > 0.0f) return finish();
        }
        // No unbounded actions if the settings/signed timer ever drift.
        out = {};
        reset();
        return out;
    }
    float timer() const noexcept {return timer_;}
    int activation_left() const noexcept {return delay_remaining_;}
    bool was_held() const noexcept {return was_held_;}
private:
    float timer_ = 0.0f;
    bool was_held_ = false;
    int delay_remaining_ = 0;
};
} // namespace slinkywinky::reference_compat
