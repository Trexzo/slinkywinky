#pragma once
#include "slinkywinky/menu_hit_test.hpp"
#include <cstdint>
#include <optional>
#include <thread>

namespace slinkywinky {

// R98: input-independent left-button gesture admission for the standalone
// preview only. No WndProc hook or Minecraft mouse ownership is implied.
// Call press/move/release only from one explicit UI owner thread.
//
// A click is committed ONLY on an unambiguous mouse-up in the same painted
// target; the input policy never edits modules or touches a game process.
class MenuMouseGesture final {
public:
    MenuMouseGesture() noexcept;
    bool owner_thread() const noexcept;
    bool pending() const noexcept;
    bool cancel() noexcept;

    bool press(const MenuRenderFrame& frame,MenuPanelMode mode,
               int x,int y,std::uint64_t monotonic_ms,
               bool owner_foreground);
    bool move(const MenuRenderFrame& frame,MenuPanelMode mode,
              int x,int y,bool owner_foreground) noexcept;
    std::optional<MenuHit> release(
        const MenuRenderFrame& frame,MenuPanelMode mode,
        int x,int y,std::uint64_t monotonic_ms,
        bool owner_foreground) noexcept;

private:
    struct Press {
        MenuRenderFrame frame;
        MenuHit hit;
        MenuPanelMode mode;
        std::uint64_t timestamp=0;
    };
    bool equivalent_frame(const MenuRenderFrame& frame,MenuPanelMode mode) const noexcept;
    static bool same_hit(const MenuHit& a,const MenuHit& b) noexcept;

    std::thread::id owner_;
    std::optional<Press> active_;
    std::optional<MenuHit> last_committed_;
    std::uint64_t last_committed_ms_=0;
};

} // namespace slinkywinky
