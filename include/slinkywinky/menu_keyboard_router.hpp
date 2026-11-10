#pragma once
#include "slinkywinky/menu_navigation.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <thread>

namespace slinkywinky {

// R108: first-party modal keyboard routing *model*. No LWJGL interception,
// Windows hook, Minecraft event consumption or JVM attach is installed here.
// The caller must independently prove exclusive event dispatch before begin.
// R118 maps original LWJGL2 Page Up (201) / Page Down (209) only after
// successful modal admission; this does not install a live game hook.
enum class MenuKeyDelivery : unsigned char {
    GameUnchanged, MenuExclusive, Quarantined
};
struct MenuKeyOutcome {
    MenuKeyDelivery delivery=MenuKeyDelivery::Quarantined;
    bool request_close=false;
    bool action_applied=false;
};

class MenuKeyboardRouter final {
public:
    explicit MenuKeyboardRouter(MenuNavigation& navigation) noexcept;
    bool begin(std::uint64_t epoch,bool exclusive_dispatch_attested,
               bool keys_and_buttons_released) noexcept;
    MenuKeyOutcome event(unsigned lwjgl_key,bool down,bool foreground_verified,
                         bool event_owner_verified,bool menu_visible) noexcept;
    // Called when the GUI actually closes. Previously GUI-owned key releases
    // stay quarantined until drained. It never sends anything to Minecraft.
    bool end() noexcept;
    // A rejected/lost-focus session may return gameplay ownership ONLY after
    // a separate host proves GUI closure, all controls released, and the
    // original vanilla input path restored. No source calls this in-game.
    bool release_after_abort(bool menu_closed,bool controls_released,
                             bool original_input_restored) noexcept;
    // R191: fixture-only check that an external keyboard model operates
    // on the identical retained MenuNavigation, not a dummy menu copy.
    bool fixture_routes(const MenuNavigation& target) const noexcept {
        return same_owner() && &navigation_==&target;
    }
    bool active() const noexcept;
    bool revoked() const noexcept;
    std::uint64_t epoch() const noexcept;
private:
    enum class State : unsigned char {Idle, Active, Draining, Revoked};
    bool same_owner() const noexcept;
    bool all_released() const noexcept;
    static std::optional<MenuNavAction> action_for(unsigned key) noexcept;
    const std::thread::id owner_;
    MenuNavigation& navigation_;
    State state_=State::Idle;
    std::uint64_t last_epoch_=0;
    std::array<bool,256> pressed_{};
};
} // namespace slinkywinky
