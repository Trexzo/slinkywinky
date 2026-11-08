#pragma once
// R183: independently authored, offline-only No Hit Delay decision.
// Does not access JVM state, clear Minecraft fields or synthesize inputs.
namespace slinkywinky::reference_compat {
inline constexpr unsigned kNoHitDelayEvent = 14;

struct NoHitDelayInput {
    bool attack_click = false;
    bool jvm_ready = false;
    int left_click_counter = 0;
    unsigned mode = 0;                  // 0: always clear; 1: selective
    bool target_present = false;
    int target_type_ordinal = 0;       // candidate: 0 denotes miss
    bool player_using_item = false;
};

struct NoHitDelayDecision {
    bool clear_click_counter = false;  // proposed new counter is zero
    bool swing_arm = false;            // proposed arm-swing only; no real input
};

class NoHitDelayController final {
public:
    NoHitDelayDecision evaluate(const NoHitDelayInput& in) const noexcept {
        if (!in.attack_click || !in.jvm_ready || in.left_click_counter <= 0)
            return {};
        if (in.mode == 1 &&
            (!in.target_present || in.target_type_ordinal == 0)) {
            // The candidate's selective mode only swings on a miss,
            // provided the player is not already using an item.
            return {false, !in.player_using_item};
        }
        return {true, false};
    }
};
} // namespace slinkywinky::reference_compat
