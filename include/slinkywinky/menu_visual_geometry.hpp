#pragma once
#include "slinkywinky/menu_render.hpp"
#include <cstddef>

namespace slinkywinky {
// R113: a single render-neutral source of truth for visual and mouse layout.
// Values are logical panel units and therefore independent of physical WGL scale.
constexpr float menu_nav_x=16.0f;
constexpr float menu_nav_y=14.0f;
constexpr float menu_nav_height=42.0f;
constexpr float menu_first_row_y=72.0f;
constexpr float menu_tab_count=7.0f;
constexpr float menu_row_gap=6.0f;
inline float menu_row_body_height(const MenuRenderFrame& frame) noexcept {
    return frame.appearance.compact?43.0f:58.0f;
}
inline float menu_expanded_height(const MenuRenderFrame& frame) noexcept {
    return frame.appearance.compact?34.0f:40.0f;
}
inline float menu_row_top(const MenuRenderFrame& frame,std::size_t index) noexcept {
    float y=menu_first_row_y;
    for(std::size_t i=0;i<index && i<frame.visible.size();++i)
        y+=menu_row_body_height(frame)+menu_row_gap+
            (frame.visible[i].expanded?menu_expanded_height(frame):0.0f);
    return y;
}
inline float menu_row_total_height(const MenuRenderFrame& frame,
                                   std::size_t index) noexcept {
    return menu_row_body_height(frame)+
           (index<frame.visible.size() && frame.visible[index].expanded?
            menu_expanded_height(frame):0.0f);
}
inline float menu_tab_width(float logical_panel_width) noexcept {
    return (logical_panel_width-2*menu_nav_x)/menu_tab_count;
}
} // namespace slinkywinky
