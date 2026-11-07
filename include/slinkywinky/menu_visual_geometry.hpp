#pragma once
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <cstddef>

namespace slinkywinky {
// R115: dimensions extracted from the provided candidate reference's
// recovered ClickGUI render notes. These operate in logical GUI pixels.
// The layout is a noninteractive first-party rendering translation.
constexpr float menu_nav_x=8.0f;
constexpr float menu_nav_y=8.0f;
constexpr float menu_nav_height=42.0f;
constexpr float menu_nav_radius=20.0f;
constexpr float menu_nav_selected_inset=6.0f;
constexpr float menu_first_row_y=62.0f;
constexpr float menu_tab_count=7.0f;
constexpr float menu_row_gap=12.0f;
constexpr float menu_panel_padding=8.0f;
constexpr float menu_card_radius=20.0f;
constexpr float menu_card_inner_padding=8.0f;
constexpr float menu_widget_height=20.0f;
constexpr float menu_switch_width=40.0f;
constexpr float menu_switch_height=20.0f;
constexpr float menu_expand_size=20.0f;
constexpr float menu_column_target=430.0f;

inline float menu_tab_width(float width) noexcept {
    return (width-2.0f*menu_nav_x)/menu_tab_count;
}
inline unsigned menu_module_columns(float width) noexcept {
    const float available=width-2.0f*menu_panel_padding;
    const float raw=(available+menu_row_gap)/
                      (menu_column_target+menu_row_gap);
    return static_cast<unsigned>(std::max(1.0f,std::min(4.0f,raw)));
}
inline float menu_row_body_height(const MenuRenderFrame& frame) noexcept {
    return frame.appearance.compact?36.0f:68.0f;
}
inline float menu_expanded_height(const MenuRenderFrame& frame,
                                  std::size_t index) noexcept {
    if(index>=frame.visible.size())return 0.0f;
    // A bounded, read-only viewport into 378 candidate GUI controls.
    // Settings beyond five remain in the typed catalog for the next
    // scrolling/interaction pass; never paint them outside the card.
    const std::size_t count=recovered_widget_count(
        frame.visible[index].title);
    const float visible=static_cast<float>(std::min<std::size_t>(count,5u));
    return 52.0f+visible*27.0f+(count>5?22.0f:12.0f);
}
inline float menu_row_total_height(const MenuRenderFrame& frame,
                                   std::size_t index) noexcept {
    return menu_row_body_height(frame)+
        (index<frame.visible.size() && frame.visible[index].expanded?
         menu_expanded_height(frame,index):0.0f);
}
struct MenuModuleCardRect {
    float x,y,width,height;
};
inline MenuModuleCardRect menu_module_card_rect(
        const MenuRenderFrame& frame,float logical_width,
        std::size_t index) noexcept {
    const unsigned columns=menu_module_columns(logical_width);
    const float usable=logical_width-2.0f*menu_panel_padding;
    const float cell=(usable-(columns-1u)*menu_row_gap)/columns;
    const unsigned column=static_cast<unsigned>(index%columns);
    float y=menu_first_row_y;
    for(std::size_t i=column;i<index && i<frame.visible.size();i+=columns)
        y+=menu_row_total_height(frame,i)+menu_row_gap;
    return {menu_panel_padding+column*(cell+menu_row_gap),
            y,cell,menu_row_total_height(frame,index)};
}
inline float menu_row_top(const MenuRenderFrame& frame,std::size_t index,
                          float logical_width=676.0f) noexcept {
    return menu_module_card_rect(frame,logical_width,index).y;
}
inline unsigned menu_profile_columns(float width) noexcept {
    return menu_module_columns(width);
}
inline float menu_profile_card_height(const MenuRenderFrame& frame) noexcept {
    return frame.appearance.compact?36.0f:68.0f;
}
struct MenuProfileCardRect {
    float x,y,width,height;
};
inline MenuProfileCardRect menu_profile_card_rect(
    const MenuRenderFrame& frame,float logical_width,
    std::size_t index) noexcept {
    const auto columns=menu_profile_columns(logical_width);
    const float usable=logical_width-2.0f*menu_panel_padding;
    const float card=(usable-(columns-1u)*menu_row_gap)/columns;
    const unsigned column=static_cast<unsigned>(index%columns);
    const unsigned row=static_cast<unsigned>(index/columns);
    const float h=menu_profile_card_height(frame);
    return {menu_panel_padding+column*(card+menu_row_gap),
            menu_first_row_y+row*(h+menu_row_gap),card,h};
}
} // namespace slinkywinky
