#pragma once
#ifdef _WIN32
#include "slinkywinky/windows_game_window_gate.hpp"
#include <cstdint>
#include <thread>

namespace slinkywinky {

// R104: strictly read-only *foreground eligibility*, never a key/mouse hook.
// A valid WGL/HWND match is necessary but NOT sufficient to consume events.
// No function in this class can authorize Minecraft input or block gameplay.
enum class FocusFenceState : std::uint8_t {
    Dormant, Background, ForegroundCandidate, FocusLost, Refused
};

class MenuFocusFence final {
public:
    MenuFocusFence() noexcept : owner_(std::this_thread::get_id()) {}

    bool begin(std::uint64_t epoch, bool released_keys_and_buttons) noexcept {
        if(!same_owner() || epoch==0 || epoch<=last_epoch_ ||
           !released_keys_and_buttons)return false;
        last_epoch_=epoch;
        consecutive_=0;
        foreground_samples_=0;
        background_samples_=0;
        denied_samples_=0;
        state_=FocusFenceState::Dormant;
        return true;
    }

    // Must only be called after the independent exact original WGL/HDC/
    // root HWND + viewport audit. The caller does not gain event ownership.
    FocusFenceState observe(GameWindowFocus result) noexcept {
        if(!same_owner() || last_epoch_==0) {
            state_=FocusFenceState::Refused;
            return state_;
        }
        if(state_==FocusFenceState::Refused ||
           state_==FocusFenceState::FocusLost)return state_;
        if(result==GameWindowFocus::Refused) {
            denied_samples_++;
            consecutive_=0;
            state_=FocusFenceState::Refused;
        } else if(result==GameWindowFocus::ValidBackground) {
            background_samples_++;
            consecutive_=0;
            // Once genuine foreground candidacy existed, lost focus is
            // terminal until a *new* epoch and verified released inputs.
            state_=(state_==FocusFenceState::ForegroundCandidate)
                ? FocusFenceState::FocusLost
                : FocusFenceState::Background;
        } else if(result==GameWindowFocus::ValidForeground) {
            foreground_samples_++;
            if(consecutive_<3)consecutive_++;
            state_=(consecutive_==3)
                ? FocusFenceState::ForegroundCandidate
                : FocusFenceState::Dormant;
        } else {
            denied_samples_++;
            state_=FocusFenceState::Refused;
        }
        return state_;
    }

    // A candidate only: the modal routing that prevents Minecraft seeing
    // the same physical events has NOT been installed/certified in R104.
    bool foreground_candidate() const noexcept {
        return same_owner() && state_==FocusFenceState::ForegroundCandidate;
    }
    bool can_consume_game_input() const noexcept { return false; }
    FocusFenceState state() const noexcept {
        return same_owner()?state_:FocusFenceState::Refused;
    }
    unsigned foreground_samples() const noexcept {return foreground_samples_;}
    unsigned background_samples() const noexcept {return background_samples_;}
    unsigned denied_samples() const noexcept {return denied_samples_;}
    std::uint64_t epoch() const noexcept {return last_epoch_;}

private:
    bool same_owner() const noexcept {
        return std::this_thread::get_id()==owner_;
    }
    const std::thread::id owner_;
    std::uint64_t last_epoch_=0;
    unsigned consecutive_=0;
    unsigned foreground_samples_=0;
    unsigned background_samples_=0;
    unsigned denied_samples_=0;
    FocusFenceState state_=FocusFenceState::Dormant;
};

} // namespace slinkywinky
#endif
