#pragma once
#include "slinkywinky/menu_settings_vertical.hpp"
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

// R137: accepts exactly one well-formed printable Unicode scalar per
// insertion. Controls, overlong UTF-8, surrogates and invalid bytes are
// rejected; max rendered input is a bounded 128 UTF-8 bytes.
inline bool menu_one_printable_utf8(std::string_view input) noexcept {
    if(input.empty() || input.size()>4u)return false;
    const auto first=static_cast<unsigned char>(input[0]);
    std::uint32_t point=0u;
    std::size_t length=0u;
    if(first>=0x20u && first<=0x7Eu) {
        point=first;length=1u;
    } else if(first>=0xC2u && first<=0xDFu) {
        point=first&0x1Fu;length=2u;
    } else if(first>=0xE0u && first<=0xEFu) {
        point=first&0x0Fu;length=3u;
    } else if(first>=0xF0u && first<=0xF4u) {
        point=first&0x07u;length=4u;
    } else return false;
    if(input.size()!=length)return false;
    for(std::size_t i=1;i<length;++i) {
        const auto c=static_cast<unsigned char>(input[i]);
        if((c&0xC0u)!=0x80u)return false;
        point=(point<<6u)|(c&0x3Fu);
    }
    if((length==2u && point<0x80u) ||
       (length==3u && point<0x800u) ||
       (length==4u && point<0x10000u) ||
       point>0x10FFFFu ||
       (point>=0xD800u && point<=0xDFFFu) ||
       (point>=0x7Fu && point<=0x9Fu))
        return false;
    return true;
}
struct MenuTextFieldRect {
    float x=0.0f;
    float y=0.0f;
    float width=0.0f;
    float height=20.0f;
};
inline std::optional<MenuTextFieldRect> menu_text_field_geometry(
        const MenuRenderFrame& frame,float logical_width,
        float logical_height,std::size_t catalog_index,
        std::size_t widget_index) noexcept {
    const auto& catalog=official_module_catalog();
    const auto& all=recovered_widget_catalog();
    if(frame.unload_confirmation_preview || frame.dropdown_preview ||
       frame.is_placeholder || !menu_settings_offsets_valid(frame) ||
       catalog_index>=catalog.size() || widget_index>=all.size() ||
       catalog[catalog_index].tab!=frame.tab ||
       all[widget_index].kind!=RecoveredWidgetKind::Text ||
       std::string_view(all[widget_index].module)!=catalog[catalog_index].name)
        return std::nullopt;
    std::size_t visible_index=frame.visible.size();
    for(std::size_t i=0;i<frame.visible.size();++i)
        if(frame.visible[i].catalog_index==catalog_index) {
            visible_index=i;break;
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
        if(page[i]==&all[widget_index]){position=i;break;}
    if(position>=page.size())return std::nullopt;
    const auto rect=menu_module_card_rect(frame,logical_width,visible_index);
    if(rect.y+rect.height>logical_height-menu_panel_padding)
        return std::nullopt;
    const float controlY=rect.y+menu_row_body_height(frame)+
        menu_settings_control_offset(page,position);
    const float controlX=menu_setting_column(rect.x,rect.width).x;
    const float controlW=menu_setting_column(rect.x,rect.width).width;
    MenuTextFieldRect field{controlX,controlY,controlW,20.0f};
    if(field.x<rect.x || field.x+field.width>rect.x+rect.width ||
       field.y<rect.y || field.y+field.height>rect.y+rect.height ||
       field.y+field.height>logical_height-menu_panel_padding)
        return std::nullopt;
    return field;
}
} // namespace slinkywinky
