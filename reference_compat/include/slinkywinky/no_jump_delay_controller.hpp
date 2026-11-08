#pragma once
// R179: independent, pure No Jump Delay decision.
// Candidate observation: No Jump Delay / move / native event 17, priority 100.
// Does not set Minecraft fields, inject code or access an external process.

namespace slinkywinky::reference_compat {

struct JumpDelayInput {
    bool in_game = false;
    int observed_jump_ticks = 0;
};

struct JumpDelayDecision {
    bool override_jump_ticks = false;
    int proposed_jump_ticks = 0;
};

// The candidate callback asks the game to set jump ticks to zero on every
// valid in-game player update. The original JVM call is intentionally absent.
// observed_jump_ticks is retained for typed test inputs, not read by policy.
class NoJumpDelayController final {
public:
    JumpDelayDecision evaluate(const JumpDelayInput& input) const noexcept {
        if (!input.in_game) return {};
        return {true, 0};
    }
};

}  // namespace slinkywinky::reference_compat
