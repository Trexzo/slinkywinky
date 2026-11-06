#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "slinkywinky/menu_render.hpp"

namespace slinkywinky {

/**
 * R67: first-party fixed-function OpenGL 1.1 menu overlay.
 *
 * Caller is REQUIRED to own the verified WGL context and call only at a
 * known render boundary, outside any glBegin/glEnd. Does not create, hook,
 * retain, replace or switch a GL context and never touches Minecraft.
 *
 * Returns false before issuing GL commands if the expected context/DC,
 * frame epoch, viewport, or frame layout is invalid. The bounded overlay
 * does not clear the scene; it is clipped to an inset panel, and requires
 * a separate, verified render admission before any future in-game use.
 * State altered during
 * a successful draw is restored before the function returns.
 */
// StandalonePreview reproduces the original full preview canvas.
// BoundedOverlay is an opt-in preparation for future game integration:
// draws only a centered, clipped panel; does not clear or replace the frame.
enum class MenuGL11Mode : unsigned char {
    StandalonePreview,
    BoundedOverlay,
    CompactOverlay
};

bool draw_menu_gl11(const MenuRenderFrame& frame,
                    HGLRC expected_context,
                    HDC expected_dc,
                    MenuGL11Mode mode=MenuGL11Mode::StandalonePreview) noexcept;

} // namespace slinkywinky
#endif
