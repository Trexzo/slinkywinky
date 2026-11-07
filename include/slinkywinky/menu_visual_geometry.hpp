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
// The expanded control area follows Slinky's heading + labeled field rows,
// instead of squeezing the label and value into one small preview line.
inline float menu_expanded_height(const MenuRenderFrame& frame) noexcept {
    return frame.appearance.compact?65.0f:84.0f;
}

// R114: source-style two-column read-only profile card grid. The compact
// viewport may use one column if the logical space is insufficient.
constexpr float menu_profile_card_gap=12.0f;
inline unsigned menu_profile_columns(float logical_width) noexcept {
    return logical_width>=580.0f?2u:1u;
}
inline float menu_profile_card_height(const MenuRenderFrame& frame) noexcept {
    return frame.appearance.compact?54.0f:88.0f;
}
struct MenuProfileCardRect {
    float x,y,width,height;
};
inline MenuProfileCardRect menu_profile_card_rect(
    const MenuRenderFrame& frame,float logical_width,
    std::size_t index) noexcept {
    const unsigned columns=menu_profile_columns(logical_width);
    const float width=(logical_width-2.0f*menu_nav_x-
        (columns-1u)*menu_profile_card_gap)/columns;
    const unsigned column=static_cast<unsigned>(index%columns);
    const unsigned row=static_cast<unsigned>(index/columns);
    const float height=menu_profile_card_height(frame);
    return {menu_nav_x+column*(width+menu_profile_card_gap),
            menu_first_row_y+row*(height+menu_profile_card_gap),
            width,height};
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
