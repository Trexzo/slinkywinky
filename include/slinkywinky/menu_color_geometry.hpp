#pragma once
#include "slinkywinky/menu_setting_layout.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace slinkywinky {

// R136: the candidate template's <input type=color> has no authenticated
// OS dialog implementation. A Windows native color chooser is used only
// by the separate demo; these helpers never make an OS or game call.
inline int menu_hex_digit(char c) noexcept {
    if(c>='0' && c<='9')return c-'0';
    if(c>='a' && c<='f')return c-'a'+10;
    if(c>='A' && c<='F')return c-'A'+10;
    return -1;
}
inline std::optional<std::uint32_t> menu_parse_color_hex(
        std::string_view value) noexcept {
    if(value.size()!=7u || value.front()!='#')return std::nullopt;
    std::uint32_t rgb=0u;
    for(std::size_t i=1;i<value.size();++i) {
        const int digit=menu_hex_digit(value[i]);
        if(digit<0)return std::nullopt;
        rgb=(rgb<<4u)|static_cast<std::uint32_t>(digit);
    }
    return rgb;
}
inline std::optional<std::uint32_t> menu_source_widget_color(
        std::size_t widget_index) noexcept {
    const auto& all=recovered_widget_catalog();
    if(widget_index>=all.size() ||
       all[widget_index].kind!=RecoveredWidgetKind::Color)
        return std::nullopt;
    return menu_parse_color_hex(all[widget_index].auxiliary);
}

struct MenuColorSwatchRect {
    float x=0.0f;
    float y=0.0f;
    float width=42.0f;
    float height=20.0f;
};

// Mirrors the currently drawn GL11 42x22 right-aligned swatch. The
// module must be on the visible page, expanded, and entirely unclipped.
inline std::optional<MenuColorSwatchRect> menu_color_swatch_geometry(
        const MenuRenderFrame& frame,float logical_width,
        float logical_height,std::size_t catalog_index,
        std::size_t widget_index) noexcept {
    const auto& catalog=official_module_catalog();
    const auto& all=recovered_widget_catalog();
    if(frame.unload_confirmation_preview || frame.dropdown_preview ||
       frame.is_placeholder || !menu_settings_offsets_valid(frame) ||
       catalog_index>=catalog.size() || widget_index>=all.size() ||
       catalog[catalog_index].tab!=frame.tab ||
       all[widget_index].kind!=RecoveredWidgetKind::Color ||
       std::string_view(all[widget_index].module)!=catalog[catalog_index].name ||
       !menu_source_widget_color(widget_index))
        return std::nullopt;
    std::size_t visible_index=frame.visible.size();
    for(std::size_t i=0;i<frame.visible.size();++i)
        if(frame.visible[i].catalog_index==catalog_index) {
            visible_index=i; break;
        }
    if(visible_index>=frame.visible.size() ||
       !frame.visible[visible_index].expanded)return std::nullopt;
    const auto& item=frame.visible[visible_index];
    std::array<const RecoveredWidget*,5> page{};
    if(!recovered_widget_visible_page(
        item.title.c_str(),item.settings_offset,
        static_cast<int>(frame.aim_preview_mode),
        static_cast<int>(frame.target_preview_mode),page))
        return std::nullopt;
    std::size_t position=page.size();
    for(std::size_t i=0;i<page.size();++i)
        if(page[i]==&all[widget_index]) {position=i;break;}
    if(position>=page.size())return std::nullopt;
    const auto rect=menu_module_card_rect(frame,logical_width,visible_index);
    if(rect.y+rect.height>logical_height-menu_panel_padding)
        return std::nullopt;
    float headings=0.0f;
    std::string_view previous=page[0]?page[0]->group:"";
    for(std::size_t i=1;i<=position;++i) {
        if(!page[i])return std::nullopt;
        const std::string_view now=page[i]->group;
        if(now!=previous) {headings+=20.0f;previous=now;}
    }
    const float controlY=rect.y+menu_row_body_height(frame)+43.0f+
        27.0f*static_cast<float>(position)+headings+2.0f;
    const float controlX=menu_setting_column(rect.x,rect.width).x;
    const float controlW=menu_setting_column(rect.x,rect.width).width;
    MenuColorSwatchRect swatch{
        controlX+controlW-40.0f,controlY,40.0f,20.0f
    };
    if(swatch.x<rect.x || swatch.x+swatch.width>rect.x+rect.width ||
       swatch.y<rect.y || swatch.y+swatch.height>rect.y+rect.height ||
       swatch.y+swatch.height>logical_height-menu_panel_padding)
        return std::nullopt;
    return swatch;
}
} // namespace slinkywinky
