#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include "slinkywinky/windows_game_window_gate.hpp"
#include <cstdint>
#include <optional>

namespace slinkywinky {

// R172: source-admitted, FOREGROUND-ONLY, physically read-only input evidence.
// This is not an event queue, game input consumer, "exclusive owner", or
// permission to invoke WglRetainedMenuPreview::pointer. Never log coordinates.
struct OriginalPointerObservation final {
    int client_x=0,client_y=0;
    bool inside_client=false;
    std::uint32_t button_down_mask=0;
};

// Plain geometry test oracle; both bounds are exclusive at right/bottom.
inline bool point_inside_original_client(int x,int y,
                                         unsigned width,unsigned height) noexcept {
    return x>=0 && y>=0 &&
        static_cast<unsigned>(x)<width &&
        static_cast<unsigned>(y)<height;
}

// This function ONLY reads OS state. It neither calls Peek/GetMessage nor
// changes the cursor, capture, focus, WGL state, or any vanilla LWJGL state.
// The caller's original-Mojang class pin must still be checked by its agent.
inline std::optional<OriginalPointerObservation> audit_original_render_pointer(
        HGLRC context,HDC dc,unsigned width,unsigned height) noexcept {
    if(audit_game_render_window(context,dc,width,height)!=
       GameWindowFocus::ValidForeground)
        return std::nullopt;
    const HWND hwnd=WindowFromDC(dc);
    if(!hwnd || !IsWindow(hwnd))return std::nullopt;
    POINT point{};
    if(!GetCursorPos(&point) || !ScreenToClient(hwnd,&point))
        return std::nullopt;
    // The API is read-only and preserves off-window coordinates for proof of
    // cursor-outside. An absurd cross-monitor offset is refused explicitly.
    if(point.x<-32768 || point.y<-32768 ||
       point.x>32767 || point.y>32767)return std::nullopt;
    // GetKeyState only reads the render thread's last dequeued-message
    // button state. No async pressed-since-last-query bit is consumed.
    std::uint32_t mask=0;
    if((GetKeyState(VK_LBUTTON)&0x8000)!=0)mask|=1u;
    if((GetKeyState(VK_RBUTTON)&0x8000)!=0)mask|=2u;
    if((GetKeyState(VK_MBUTTON)&0x8000)!=0)mask|=4u;
    if((GetKeyState(VK_XBUTTON1)&0x8000)!=0)mask|=8u;
    if((GetKeyState(VK_XBUTTON2)&0x8000)!=0)mask|=16u;
    return OriginalPointerObservation{
        static_cast<int>(point.x),
        static_cast<int>(point.y),
        point_inside_original_client(point.x,point.y,width,height),
        mask
    };
}
} // namespace slinkywinky
#endif
