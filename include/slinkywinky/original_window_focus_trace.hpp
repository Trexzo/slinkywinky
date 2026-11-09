#pragma once
#include <array>
#include <cstdint>
#include <limits>

namespace slinkywinky {
// R183: strict, read-only OS focus order. Callers MUST obtain the state from
// audit_game_render_window on the pinned original WGL owner. Fixture callers
// may supply synthetic statuses; those never constitute original-game proof.
enum class OriginalWindowFocusState : int {
    Refused=-1, Background=0, Foreground=1
};
class OriginalWindowFocusTrace final {
public:
    // R185: a rejected WGL paint is not a sampled OS focus state. Break
    // chronology without fabricating an extra foreground/background/refused
    // observation; retain all bounded evidence counters and sample count.
    void break_sequence() noexcept {
        previous_seen_=false;
        previous_foreground_=false;
        had_foreground_=false;
        after_regain_=false;
    }
    // Refused observations are counted but NEVER treated as background or
    // bridged across as a valid foreground-loss/reacquisition sequence.
    bool observe(OriginalWindowFocusState status,bool pointer_admitted,
                 bool exact_source_hit) noexcept {
        if(samples_==std::numeric_limits<std::uint64_t>::max())return false;
        ++samples_;
        const bool foreground=status==OriginalWindowFocusState::Foreground;
        const bool background=status==OriginalWindowFocusState::Background;
        if((!foreground && !background) ||
           (foreground!=pointer_admitted) ||
           (!foreground && exact_source_hit)) {
            ++refused_;
            break_sequence();
            return true;
        }
        if(foreground) {
            ++foreground_;
            if(previous_seen_ && !previous_foreground_ && had_foreground_) {
                ++regains_;
                after_regain_=true;
            }
            had_foreground_=true;
            if(exact_source_hit && after_regain_)++post_hits_;
        } else {
            ++background_;
            if(previous_seen_ && previous_foreground_) {
                ++losses_;
                after_regain_=false;
            }
        }
        previous_seen_=true;
        previous_foreground_=foreground;
        return true;
    }
    std::array<std::uint64_t,6> counters() const noexcept {
        return {foreground_,background_,refused_,losses_,regains_,post_hits_};
    }
    std::uint64_t samples() const noexcept {return samples_;}
private:
    std::uint64_t samples_=0,foreground_=0,background_=0,refused_=0;
    std::uint64_t losses_=0,regains_=0,post_hits_=0;
    bool previous_seen_=false,previous_foreground_=false;
    bool had_foreground_=false,after_regain_=false;
};
} // namespace slinkywinky
