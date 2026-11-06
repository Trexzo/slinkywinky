#pragma once
#include "slinkywinky/menu.hpp"
#include <cstddef>
#include <optional>
#include <thread>

namespace slinkywinky {

class MenuProfileBridge;

// R96: commands are first-party *preview model* inputs, not OS keys or
// Minecraft actions. A separately admitted host may map them later.
enum class MenuNavAction : unsigned char {
    NextTab, PreviousTab, NextRow, PreviousRow,
    ExpandFocused, ToggleFocusedPreview, IncreaseSetting, DecreaseSetting
};

// The navigation controller is bound to one menu/profile pair and the
// construction thread. Its only mutations are in inert local configuration.
class MenuNavigation final {
public:
    MenuNavigation(MenuState& menu, MenuProfileBridge& profile) noexcept;
    bool owns(const MenuState& menu,const MenuProfileBridge& profile) const noexcept;
    bool owner_thread() const noexcept;
    bool apply(MenuNavAction action);
    // Focus is reported only for an actual visible row. Never leak an old
    // row selection into a different tab or scrolled page.
    std::optional<std::size_t> visible_focus() const;
private:
    bool switch_tab(int direction);
    bool move_row(int direction);
    bool focused_visible(std::size_t* index) const;
    MenuState& menu_;
    MenuProfileBridge& profile_;
    std::thread::id thread_;
    std::optional<std::size_t> focused_;
};

} // namespace slinkywinky
