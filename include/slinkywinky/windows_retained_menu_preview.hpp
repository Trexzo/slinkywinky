#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>

namespace slinkywinky {
// R99: in-game-render-thread-capable, but INERT menu-state retention.
// No keyboard hook, mouse handler, JNI calls, gameplay actions, packets or
// source-code patching. It retains a first-party model across native frames,
// unlike R91-R94's fresh per-frame diagnostic menu.
class WglRetainedMenuPreview final {
public:
    WglRetainedMenuPreview();
    bool draw(HGLRC context,HDC dc,unsigned width,unsigned height);
    bool apply(MenuNavAction action);
    bool close() noexcept;
    bool owner_thread() const noexcept;
    bool active() const noexcept {return active_;}
    std::uint64_t drawn_frames() const noexcept {return drawn_frames_;}
    std::uint64_t accepted_actions() const noexcept {return accepted_actions_;}
    std::uint64_t refused_actions() const noexcept {return refused_actions_;}
    MenuTab tab() const noexcept {return menu_.tab();}
    std::size_t scroll() const noexcept {return menu_.scroll();}
    std::optional<std::size_t> focused_row() const;
    bool preview_enabled(std::size_t index) const noexcept;
    // R158: only an active, source-pinned WGL/render owner may inspect the
    // actual typed native profile retained by the in-game GUI.
    // Returns a validated version-1 settings document, never gameplay
    // actions, key states, engine pointers, or user account information.
    std::optional<std::string> profile_snapshot(
        HGLRC context,HDC dc,unsigned width,unsigned height) const;
    WglRetainedMenuPreview(const WglRetainedMenuPreview&)=delete;
    WglRetainedMenuPreview& operator=(const WglRetainedMenuPreview&)=delete;
private:
    MenuState menu_;
    MenuProfileBridge profile_;
    MenuNavigation navigation_;
    std::thread::id owner_;
    HGLRC context_=nullptr;
    HDC dc_=nullptr;
    unsigned width_=0,height_=0;
    std::uint64_t drawn_frames_=0,accepted_actions_=0,refused_actions_=0;
    bool active_=false;
};
} // namespace slinkywinky
#endif
