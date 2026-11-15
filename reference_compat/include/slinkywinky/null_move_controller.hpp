#pragma once
// R180: independently authored offline Null Move movement-key policy.
// No JVM, input hooks, game state mutation, native module code or binaries.
#include <array>
#include <optional>

namespace slinkywinky::reference_compat {
enum class MoveKey : unsigned { forward = 0, left = 1, back = 2, right = 3 };
inline constexpr unsigned kMovementKeys = 4;

struct NullMoveInput {
    bool in_game = false;
    int ticks_existed = 0;
    // A key's effective game-pressed state entering this particular tick.
    std::array<bool, kMovementKeys> game_pressed{};
    // Whether the corresponding physical key is still being held.
    std::array<bool, kMovementKeys> physically_held{};
};

struct NullMoveDecision {
    std::array<bool, kMovementKeys> resulting_game_pressed{};
    std::array<std::optional<bool>, kMovementKeys> overrides{};
};

// Models the sequential original candidate key handling order:
// forward, left, back, right. Newly-pressed effective keys take precedence
// over their opposing direction; when released, previously held opposite
// keys may be restored according to physical key state.
// This is a TEST-ONLY proposed key state. It never writes a key binding.
class NullMoveController final {
public:
    NullMoveDecision evaluate(const NullMoveInput& input) noexcept {
        NullMoveDecision result;
        result.resulting_game_pressed = input.game_pressed;
        if (!input.in_game) return result;
        constexpr std::array<unsigned, kMovementKeys> opposite{2, 3, 0, 1};
        for (unsigned key = 0; key < kMovementKeys; ++key) {
            const unsigned other = opposite[key];
            const bool down = result.resulting_game_pressed[key];
            const bool opposite_down = result.resulting_game_pressed[other];
            if (down) {
                if (last_held_tick_[key] == 0 && opposite_down) {
                    result.resulting_game_pressed[other] = false;
                    result.overrides[other] = false;
                }
                // Source candidate uses 0 as the sentinel for "not held".
                // On tick 0 it also stores 0, matching that behavior.
                last_held_tick_[key] = input.ticks_existed;
            } else if (last_held_tick_[key] != 0) {
                last_held_tick_[key] = 0;
                if (input.physically_held[other]) {
                    result.resulting_game_pressed[other] = true;
                    result.overrides[other] = true;
                }
            } else {
                last_held_tick_[key] = 0;
            }
        }
        return result;
    }

    void reset() noexcept { last_held_tick_.fill(0); }
    const std::array<int, kMovementKeys>& last_held_tick() const noexcept {
        return last_held_tick_;
    }

private:
    std::array<int, kMovementKeys> last_held_tick_{};
};
} // namespace slinkywinky::reference_compat
