#pragma once
#ifdef _WIN32
#include "slinkywinky/windows_insert_edge.hpp"
#include "slinkywinky/windows_menu_focus_fence.hpp"
#include <cstdint>
#include <thread>

namespace slinkywinky {

// R106: opt-in Insert visibility edge classifier, NOT a modal input hook.
// The source-pinned JNI render owner supplies an independently audited WGL
// window status. No key is read or consumed by this policy class.
class ScopedVisibilityHotkey final {
public:
    explicit ScopedVisibilityHotkey(std::uint64_t epoch) noexcept
        : owner_(std::this_thread::get_id()), epoch_(epoch) {}

    bool sample(GameWindowFocus window, bool insert_down,
                bool other_buttons_released) noexcept {
        if(!same_owner())return false;
        if(!begun_) {
            // The first admission must be genuine foreground with every
            // observed input released; a held Insert never opens the menu.
            if(window==GameWindowFocus::ValidForeground && !insert_down &&
               other_buttons_released)
                begun_=fence_.begin(epoch_,true);
            if(!begun_) {
                edge_.sample(false,insert_down);
                return false;
            }
        }
        const auto status=fence_.observe(window);
        const bool eligible=status==FocusFenceState::ForegroundCandidate;
        // Three successive verified foreground frames are necessary.
        // Focus loss or ambiguous HWND latches the fence for this epoch.
        return edge_.sample(eligible,insert_down);
    }

    bool candidate() const noexcept {
        return same_owner() && begun_ && fence_.foreground_candidate();
    }
    bool can_consume_game_input() const noexcept {return false;}
private:
    bool same_owner() const noexcept {
        return std::this_thread::get_id()==owner_;
    }
    const std::thread::id owner_;
    const std::uint64_t epoch_;
    bool begun_=false;
    MenuFocusFence fence_;
    VisibilityInsertEdge edge_;
};
} // namespace slinkywinky
#endif
