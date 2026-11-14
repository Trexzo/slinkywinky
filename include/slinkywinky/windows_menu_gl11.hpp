#pragma once
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/module_arraylist.hpp"
#include <optional>

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
// R96 separate value-only focus argument, leaving original frame ABI intact.
bool draw_menu_gl11(const MenuRenderFrame& frame,
                    HGLRC expected_context,HDC expected_dc,
                    MenuGL11Mode mode,
                    std::optional<std::size_t> focused_catalog_index) noexcept;
// R200: additional value-only optional dialog state; no class ABI change.
bool draw_menu_gl11(const MenuRenderFrame& frame,
                    HGLRC expected_context,HDC expected_dc,
                    MenuGL11Mode mode,
                    std::optional<std::size_t> focused_catalog_index,
                    const MenuColorPopupPreview* color_popup) noexcept;

// R207 first-party enabled-module ArrayList renderer. This is an opt-in
// owner-thread HUD draw of a verified runtime-snapshot *copy*; the JNI
// live game callback is NOT connected yet. No direct MenuState preview.
bool draw_module_arraylist_gl11(const ModuleArrayListSnapshot& snapshot,
    HGLRC expected_context,HDC expected_dc,
    unsigned viewport_width,unsigned viewport_height,
    std::uint32_t accent_rgb=0xF35D12u) noexcept;

} // namespace slinkywinky
#endif
