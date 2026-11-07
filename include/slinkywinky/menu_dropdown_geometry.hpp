#pragma once
#include "slinkywinky/menu_visual_geometry.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

namespace slinkywinky {

// R127: pure geometry/labels for three isolated GUI preview dropdowns.
// 28-pixel option rows and rounded borders are grounded in the recovered
// clickgui.tpl.html reference. No Minecraft input, JNI, packets or globals.
struct MenuDropdownPopupRect {
    float x=0, y=0, width=0, row_height=28.0f;
    unsigned option_count=0;
    unsigned selected=0;
};
inline unsigned menu_dropdown_option_count(MenuDropdownKind kind) noexcept {
    switch(kind) {
        case MenuDropdownKind::AimMode:return 4u;
        case MenuDropdownKind::TargetMode:return 2u;
        case MenuDropdownKind::PiercingSort:return 2u;
        default:return 0u;
    }
}
inline std::string_view menu_dropdown_option_label(
        MenuDropdownKind kind,unsigned ordinal) noexcept {
    constexpr std::array<std::string_view,4> aim{{
        "Regular","Linear","Lock-on","Silent"
    }};
    constexpr std::array<std::string_view,2> target{{"Single","Switch"}};
    constexpr std::array<std::string_view,2> piercing{{"Hurt time","Health"}};
    switch(kind) {
        case MenuDropdownKind::AimMode:
            return ordinal<aim.size()?aim[ordinal]:std::string_view{};
        case MenuDropdownKind::TargetMode:
            return ordinal<target.size()?target[ordinal]:std::string_view{};
        case MenuDropdownKind::PiercingSort:
            return ordinal<piercing.size()?piercing[ordinal]:std::string_view{};
        default:return {};
    }
}
inline std::optional<MenuDropdownPopupRect> menu_dropdown_popup_geometry(
        const MenuRenderFrame& frame,float logical_width,
        float logical_height) noexcept {
    if(!frame.dropdown_preview || frame.tab!=MenuTab::Combat ||
       frame.is_placeholder || logical_width<320.0f ||
       logical_height<240.0f)return std::nullopt;
    const auto popup=*frame.dropdown_preview;
    const unsigned options=menu_dropdown_option_count(popup.kind);
    if(!options)return std::nullopt;
    const std::size_t expected_index=
        popup.kind==MenuDropdownKind::PiercingSort?11u:0u;
    if(popup.catalog_index!=expected_index)return std::nullopt;
    std::size_t visible_index=frame.visible.size();
    for(std::size_t i=0;i<frame.visible.size();++i)
        if(frame.visible[i].catalog_index==expected_index)
            visible_index=i;
    if(visible_index==frame.visible.size())return std::nullopt;
    const auto& card=frame.visible[visible_index];
    if(!card.expanded || card.settings_offset!=popup.page_offset ||
       popup.control_index<popup.page_offset ||
       popup.control_index-popup.page_offset>=5u)return std::nullopt;
    std::array<const RecoveredWidget*,5> widgets{};
    const auto count=recovered_widget_visible_page(
        card.title.c_str(),popup.page_offset,
        static_cast<int>(frame.aim_preview_mode),
        static_cast<int>(frame.target_preview_mode),widgets);
    const std::size_t position=popup.control_index-popup.page_offset;
    if(!count || position>=widgets.size() || !widgets[position] ||
       widgets[position]->kind!=RecoveredWidgetKind::Dropdown)
        return std::nullopt;
    const auto& widget=*widgets[position];
    if(popup.kind==MenuDropdownKind::AimMode &&
       (popup.control_index!=0u ||
        std::string_view(widget.group)!="Aim" ||
        std::string_view(widget.label)!="Mode"))return std::nullopt;
    if(popup.kind==MenuDropdownKind::TargetMode &&
       (std::string_view(widget.group)!="Target" ||
        std::string_view(widget.label)!="Mode"))return std::nullopt;
    if(popup.kind==MenuDropdownKind::PiercingSort &&
       (popup.control_index!=0u ||
        std::string_view(widget.group)!="Always hit through" ||
        std::string_view(widget.label)!="Sort targets by"))
        return std::nullopt;
    const auto rect=menu_module_card_rect(
        frame,logical_width,visible_index);
    if(rect.y+rect.height>logical_height-menu_panel_padding)
        return std::nullopt;
    float headings=0.0f;
    std::string_view last=widgets[0]?std::string_view(widgets[0]->group):
                                     std::string_view{};
    for(std::size_t i=1;i<=position;++i) {
        if(!widgets[i])return std::nullopt;
        const std::string_view group=widgets[i]->group;
        if(group!=last){headings+=20.0f;last=group;}
    }
    const float controlY=rect.y+menu_row_body_height(frame)+
        43.0f+27.0f*static_cast<float>(position)+headings+2.0f;
    MenuDropdownPopupRect result;
    result.x=rect.x+rect.width*0.54f;
    result.width=std::max(16.0f,rect.width*0.46f-10.0f);
    result.option_count=options;
    result.selected=popup.kind==MenuDropdownKind::AimMode?
        frame.aim_preview_mode:
        (popup.kind==MenuDropdownKind::TargetMode?
         frame.target_preview_mode:frame.piercing_preview.sort);
    if(result.selected>=options)return std::nullopt;
    const float height=result.row_height*static_cast<float>(options);
    result.y=controlY;
    // Open upward when the reference-style option list would be clipped.
    // If no full, safe placement exists, the popup does not receive input.
    if(result.y+height>logical_height-menu_panel_padding)
        result.y=controlY-height+22.0f;
    if(result.y<menu_first_row_y ||
       result.y+height>logical_height-menu_panel_padding ||
       result.x<menu_panel_padding ||
       result.x+result.width>logical_width-menu_panel_padding)
        return std::nullopt;
    return result;
}
} // namespace slinkywinky
