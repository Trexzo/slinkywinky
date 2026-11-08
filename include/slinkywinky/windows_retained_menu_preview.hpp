#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <thread>

namespace slinkywinky {
// R99: in-game-render-thread-capable, inert menu-state retention.
// R159: the opt-in pointer transaction is a first-party MODEL only;
// there is still NO original Minecraft mouse hook, game input consumption,
// movement, combat, packets, or source-class modification. The model is
// retained across native WGL frames instead of reset each draw.
// First-party pointer events may only be delivered by a separately
// proven, exclusive GUI-input owner. No live original-game hook invokes
// this interface. Cancellation is always allowed for an active owner.
enum class RetainedPointerPhase : unsigned char {
    Down, Move, Up, Cancel
};
class WglRetainedMenuPreview final {
public:
    WglRetainedMenuPreview();
    bool draw(HGLRC context,HDC dc,unsigned width,unsigned height);
    bool apply(MenuNavAction action);
    // R159: admitted WGL owner + exact viewport, physical client pixels,
    // monotonic timestamp, explicitly attested event exclusivity and
    // foreground. This is NOT yet connected to LWJGL/Windows game input.
    bool pointer(RetainedPointerPhase phase,HGLRC context,HDC dc,
                 unsigned width,unsigned height,int x,int y,
                 std::uint64_t monotonic_ms,bool exclusive_dispatch_attested,
                 bool foreground_verified);
    bool close() noexcept;
    bool owner_thread() const noexcept;
    bool active() const noexcept {return active_;}
    std::uint64_t drawn_frames() const noexcept {return drawn_frames_;}
    std::uint64_t accepted_actions() const noexcept {return accepted_actions_;}
    std::uint64_t refused_actions() const noexcept {return refused_actions_;}
    MenuTab tab() const noexcept {return menu_.tab();}
    std::size_t scroll() const noexcept {return menu_.scroll();}
    std::optional<std::size_t> focused_row() const;
    // R168: strict source-page read-only receipt. This is local GUI
    // configuration evidence, not a live Minecraft input ownership gate.
    std::optional<std::size_t> settings_page(std::size_t index) const noexcept;
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
    MenuMouseGesture pointer_gesture_;
    MenuRenderSession pointer_frames_;
    std::thread::id owner_;
    HGLRC context_=nullptr;
    HDC dc_=nullptr;
    unsigned width_=0,height_=0;
    std::uint64_t drawn_frames_=0,accepted_actions_=0,refused_actions_=0;
    bool active_=false;
};
} // namespace slinkywinky
#endif
