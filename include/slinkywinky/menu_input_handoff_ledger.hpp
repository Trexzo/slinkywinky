#pragma once
#include <array>
#include <cstdint>
#include <thread>

namespace slinkywinky {

// R190: completely offline, first-party *fixture* handoff ledger.
// It cannot read or consume original Minecraft input and does not install a
// keyboard/mouse callback. "MenuFixtureOnly" is not live event ownership.
enum class InputHandoffPhase : unsigned char {
    Idle, Candidate, MenuFixture, Draining, Revoked
};
enum class InputHandoffRoute : unsigned char {
    GameUnchanged, Quarantined, MenuFixtureOnly
};
enum class InputHandoffDevice : unsigned char { Keyboard, Mouse };
struct InputHandoffEvent final {
    InputHandoffDevice device;
    unsigned code;
    bool down;
};

class MenuInputHandoffLedger final {
public:
    MenuInputHandoffLedger() noexcept : owner_(std::this_thread::get_id()) {}

    // First phase only. No event can be routed to the menu yet.
    bool request(std::uint64_t epoch, bool exact_window, bool foreground,
                 bool menu_visible, bool all_controls_released) noexcept {
        if(!same_owner() || phase_!=InputHandoffPhase::Idle ||
           epoch==0 || epoch<=last_epoch_ || !exact_window ||
           !foreground || !menu_visible || !all_controls_released)
            return false;
        last_epoch_=epoch;
        key_down_.fill(false);
        mouse_mask_=0;
        phase_=InputHandoffPhase::Candidate;
        return true;
    }

    // A second, separately supplied (synthetic) host confirmation.
    // Neither boolean is evidence that a real LWJGL queue was intercepted.
    bool confirm_fixture(std::uint64_t epoch,
                         bool original_dispatch_suspended,
                         bool menu_dispatch_exclusive,
                         bool all_controls_released) noexcept {
        if(!same_owner() || phase_!=InputHandoffPhase::Candidate ||
           epoch!=last_epoch_)return false;
        if(!original_dispatch_suspended || !menu_dispatch_exclusive ||
           !all_controls_released) {
            phase_=InputHandoffPhase::Revoked;
            return false;
        }
        phase_=InputHandoffPhase::MenuFixture;
        return true;
    }

    // Only synthetic fixture events may reach this method. Never call from
    // Minecraft's original input handlers. It never forwards or injects.
    InputHandoffRoute simulate(InputHandoffEvent event,
                               bool exact_window, bool foreground,
                               bool menu_visible,
                               bool fixture_dispatch_exclusive) noexcept {
        if(!same_owner())return InputHandoffRoute::Quarantined;
        if(phase_==InputHandoffPhase::Idle)
            return menu_visible?InputHandoffRoute::Quarantined:
                                InputHandoffRoute::GameUnchanged;
        if(phase_==InputHandoffPhase::Candidate ||
           phase_==InputHandoffPhase::Revoked)
            return InputHandoffRoute::Quarantined;
        if(phase_==InputHandoffPhase::Draining) {
            // New Down OR an unknown release cannot be safely reconciled
            // with a previously menu-owned key/button. Revoke and require
            // independent original-dispatch restoration evidence.
            if(event.down || !valid(event))
                phase_=InputHandoffPhase::Revoked;
            else track(event);
            return InputHandoffRoute::Quarantined;
        }
        if(!exact_window || !foreground || !menu_visible ||
           !fixture_dispatch_exclusive || !valid(event)) {
            phase_=InputHandoffPhase::Revoked;
            return InputHandoffRoute::Quarantined;
        }
        track(event);
        return InputHandoffRoute::MenuFixtureOnly;
    }

    // Close never instantly returns formerly menu-owned release edges to
    // vanilla. The fixture stays in draining until restoration is attested.
    bool close(bool menu_closed) noexcept {
        if(!same_owner() || phase_!=InputHandoffPhase::MenuFixture ||
           !menu_closed)return false;
        phase_=InputHandoffPhase::Draining;
        return true;
    }

    bool abort() noexcept {
        if(!same_owner() || phase_==InputHandoffPhase::Idle)return false;
        phase_=InputHandoffPhase::Revoked;
        return true;
    }

    // This does not restore any real original handler. A real host would
    // have to prove and perform that separately, before calling here.
    bool restore(bool menu_closed, bool all_controls_released,
                 bool original_dispatch_restored) noexcept {
        if(!same_owner() ||
           (phase_!=InputHandoffPhase::Draining &&
            phase_!=InputHandoffPhase::Revoked) ||
           !menu_closed || !all_controls_released ||
           !original_dispatch_restored ||
           (phase_==InputHandoffPhase::Draining &&
            !tracked_released()))return false;
        key_down_.fill(false);
        mouse_mask_=0;
        phase_=InputHandoffPhase::Idle;
        return true;
    }

    InputHandoffPhase phase() const noexcept {
        return same_owner()?phase_:InputHandoffPhase::Revoked;
    }
    std::uint64_t epoch() const noexcept {
        return same_owner()?last_epoch_:0;
    }
    bool tracked_released() const noexcept {
        if(!same_owner())return false;
        if(mouse_mask_!=0)return false;
        for(bool down:key_down_)if(down)return false;
        return true;
    }

    // R192: read-only held-edge query for paired synthetic event ordering.
    // A duplicate mouse-down or orphan mouse-up cannot be routed into the
    // retained native GUI merely because the model is in MenuFixture.
    bool tracked_down(InputHandoffDevice device,unsigned code) const noexcept {
        if(!same_owner())return false;
        if(device==InputHandoffDevice::Keyboard)
            return code>0 && code<key_down_.size() && key_down_[code];
        if(device==InputHandoffDevice::Mouse)
            return code<5u && (mouse_mask_ & (1u<<code))!=0u;
        return false;
    }

    // Permanent guard: this offline ledger can NEVER license live routing.
    bool may_deliver_live_minecraft_event() const noexcept {return false;}
    bool may_suppress_vanilla_event() const noexcept {return false;}

private:
    bool same_owner() const noexcept {
        return owner_==std::this_thread::get_id();
    }
    static bool valid(InputHandoffEvent event) noexcept {
        return event.device==InputHandoffDevice::Keyboard?
            event.code>0 && event.code<256:
            (event.device==InputHandoffDevice::Mouse && event.code<5);
    }
    void track(InputHandoffEvent event) noexcept {
        if(event.device==InputHandoffDevice::Keyboard)
            key_down_[event.code]=event.down;
        else if(event.down)
            mouse_mask_=static_cast<unsigned char>(
                mouse_mask_ | (1u<<event.code));
        else
            mouse_mask_=static_cast<unsigned char>(
                mouse_mask_ & ~(1u<<event.code));
    }

    const std::thread::id owner_;
    InputHandoffPhase phase_=InputHandoffPhase::Idle;
    std::uint64_t last_epoch_=0;
    std::array<bool,256> key_down_{};
    unsigned char mouse_mask_=0;
};
} // namespace slinkywinky
