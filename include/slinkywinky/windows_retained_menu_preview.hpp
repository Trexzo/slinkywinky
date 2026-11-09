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
#include "slinkywinky/menu_source_hover_target.hpp"
#include "slinkywinky/windows_menu_overlay_host.hpp"
#include "slinkywinky/original_window_focus_trace.hpp"
#include <array>
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
    // R175: actual original-foreground window passive physical cursor to
    // source-validated *visual* hover. NEVER presses, releases or gameplay.
    bool observe_hover(HGLRC context,HDC dc,unsigned width,unsigned height,
                       int x,int y,bool foreground_verified);
    std::optional<MenuNumericHover> observed_numeric_hover() const noexcept;
    std::optional<std::size_t> observed_bind_hover() const noexcept;
    // R176: counters ONLY. Exact WGL/owner/viewport receipt is the sole
    // source of original-game runtime hover evidence; no raw coordinates,
    // input buttons, game entities, profile data or click permissions.
    std::optional<std::array<std::uint64_t,5>> hover_evidence(
        HGLRC context,HDC dc,unsigned width,unsigned height) const noexcept;
    // R179: one narrowly source-named target, verified against the same
    // native hit_test_menu as the retained GL11 UI. Geometry is read-only,
    // never an event/click input or arbitrary gameplay target.
    std::optional<MenuSourceHoverTarget> source_hover_target(
        HGLRC context,HDC dc,unsigned width,unsigned height);
    // [actual_draws, exact_source_slider_visible_draws,
    //  foreground samples whose cursor hit that exact source slider].
    std::optional<std::array<std::uint64_t,3>> source_hover_evidence(
        HGLRC context,HDC dc,unsigned width,unsigned height) const noexcept;
    // R182: only aggregate render-owner focus-loss/reacquisition sequence
    // counters, plus exact source hits AFTER a verified reacquisition.
    // These never authorize clicks or modify original game input.
    std::optional<std::array<std::uint64_t,3>> focus_cycle_evidence(
        HGLRC context,HDC dc,unsigned width,unsigned height) const noexcept;
    // R183: companion receipt requires a tri-state OS window audit on the
    // same original WGL owner. Invoked after ONE accepted passive hover.
    bool observe_original_window_focus(HGLRC context,HDC dc,
        unsigned width,unsigned height,OriginalWindowFocusState state,
        bool pointer_admitted) noexcept;
    // [OS foreground, OS background, refused, losses, regains, post hits].
    std::optional<std::array<std::uint64_t,6>>
    original_window_focus_evidence(HGLRC context,HDC dc,
        unsigned width,unsigned height) const noexcept;
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
    // R169: read-only isolated GUI mode/popup receipts for owner-thread
    // test acceptance. No Minecraft UI-input or gameplay authority.
    std::optional<unsigned> aim_preview_mode() const noexcept;
    bool preview_dropdown_open() const noexcept;
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
    // R180: immutable snapshot of the WGL frame actually painted.
    // Never expose hover geometry from navigation that has not been drawn.
    std::optional<MenuRenderFrame> last_drawn_frame_;
    std::thread::id owner_;
    HGLRC context_=nullptr;
    HDC dc_=nullptr;
    unsigned width_=0,height_=0;
    std::uint64_t drawn_frames_=0,accepted_actions_=0,refused_actions_=0;
    std::uint64_t hover_samples_=0,hover_foreground_=0,
                  hover_numeric_=0,hover_bind_=0;
    std::uint64_t source_visible_draws_=0,source_exact_hover_hits_=0;
    std::uint64_t foreground_losses_=0,foreground_regains_=0,
                  post_regain_exact_source_hits_=0;
    bool focus_sample_seen_=false,had_foreground_=false,
         previous_foreground_=false,post_regain_epoch_=false;
    // Only invoked after the passive observation has been accepted.
    void note_focus_sample(bool foreground,bool exact_source_hit) noexcept;
    OriginalWindowFocusTrace exact_window_focus_;
    std::uint64_t strict_hover_samples_=0,strict_source_hits_=0;
    bool active_=false;
};
} // namespace slinkywinky
#endif
