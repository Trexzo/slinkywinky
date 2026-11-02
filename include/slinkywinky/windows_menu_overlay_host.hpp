#pragma once
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/windows_menu_gl11.hpp"
#include <cstdint>
#include <thread>

namespace slinkywinky {
/**
 * R70: bounded, graphics-owner admission bridge for a *future* verified
 * render callback. Contexts are supplied by the caller: no hooks, no JVM,
 * no Minecraft name guesses, no ownership or replacement of WGL resources.
 *
 * A session must be explicitly closed by the original owner before a
 * changed HGLRC/HDC can be accepted; stale tokens never reopen it.
 */
class WglMenuOverlayHost {
public:
    bool open(HGLRC context,HDC dc,unsigned width,unsigned height) noexcept;
    bool resize(unsigned width,unsigned height) noexcept;
    bool draw(const MenuState& menu); // native GL only on admitted owner
    bool close() noexcept;            // allowed after current context loss
    bool active() const noexcept {return session_.attached();}
    std::uint64_t last_sequence() const noexcept {return session_.sequence();}
    WglMenuOverlayHost()=default;
    WglMenuOverlayHost(const WglMenuOverlayHost&)=delete;
    WglMenuOverlayHost& operator=(const WglMenuOverlayHost&)=delete;
private:
    bool current() const noexcept;
    MenuRenderSession session_;
    std::thread::id owner_{};
    HGLRC context_=nullptr;
    HDC dc_=nullptr;
    std::uint64_t last_epoch_=0;
};
} // namespace slinkywinky
#endif
