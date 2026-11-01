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
 * frame epoch, viewport, or frame layout is invalid. State altered during
 * a successful draw is restored before the function returns.
 */
bool draw_menu_gl11(const MenuRenderFrame& frame,
                    HGLRC expected_context,
                    HDC expected_dc) noexcept;

} // namespace slinkywinky
#endif
