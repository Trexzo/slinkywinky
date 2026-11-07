#pragma once
#include "slinkywinky/menu_visual_geometry.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace slinkywinky {

// R140: first-party *preview* virtual key identifiers. Numeric Win32
// VK codes 0..255 plus 0x103/0x104 for physical mouse buttons 4/5.
// No registration of Windows hooks, Java instrumentation or gameplay binds.
constexpr std::uint16_t menu_bind_mouse2=0x101u; // middle button (DOM button=1)
constexpr std::uint16_t menu_bind_mouse3=0x102u; // right button (DOM button=2)
constexpr std::uint16_t menu_bind_mouse4=0x103u;
constexpr std::uint16_t menu_bind_mouse5=0x104u;
inline bool menu_valid_bind_code(std::uint16_t code) noexcept {
    if(code==0u)return true; // ESC clears an existing binding
    if((code>=0x30u && code<=0x39u) ||
       (code>=0x41u && code<=0x5Au) ||
       (code>=0x70u && code<=0x7Bu))
        return true;
    switch(code) {
        case 0x08u:case 0x09u:case 0x0Du:case 0x10u:
        case 0x11u:case 0x12u:case 0x20u:
        case 0x21u:case 0x22u:case 0x23u:case 0x24u:
        case 0x25u:case 0x26u:case 0x27u:case 0x28u:
        case 0x2Du:case 0x2Eu:
        case 0xA0u:case 0xA1u:case 0xA2u:case 0xA3u:
        case 0xA4u:case 0xA5u:
        case 0xBAu:case 0xBBu:case 0xBCu:case 0xBDu:
        case 0xBEu:case 0xBFu:case 0xC0u:case 0xDBu:
        case 0xDCu:case 0xDDu:case 0xDEu:
        case menu_bind_mouse2:case menu_bind_mouse3:
        case menu_bind_mouse4:case menu_bind_mouse5:
            return true;
        default:return false;
    }
}
inline std::string menu_bind_code_label(std::uint16_t code) {
    if(code==0u)return "NONE";
    if(code>=0x30u && code<=0x39u)
        return std::string(1u,static_cast<char>(code));
    if(code>=0x41u && code<=0x5Au)
        return std::string(1u,static_cast<char>(code));
    if(code>=0x70u && code<=0x7Bu)
        return "F"+std::to_string(code-0x70u+1u);
    switch(code) {
        case 0x08u:return "BACKSPACE";
        case 0x09u:return "TAB";
        case 0x0Du:return "ENTER";
        case 0x10u:return "SHIFT";
        case 0x11u:return "CONTROL";
        case 0x12u:return "ALT";
        case 0x20u:return "SPACE";
        case 0x21u:return "PREV";
        case 0x22u:return "NEXT";
        case 0x23u:return "END";
        case 0x24u:return "HOME";
        case 0x25u:return "LEFT";
        case 0x26u:return "UP";
        case 0x27u:return "RIGHT";
        case 0x28u:return "DOWN";
        case 0x2Du:return "INS";
        case 0x2Eu:return "DEL";
        case 0xA0u:return "LSHIFT";
        case 0xA1u:return "RSHIFT";
        case 0xA2u:return "LCONTROL";
        case 0xA3u:return "RCONTROL";
        case 0xA4u:return "LALT";
        case 0xA5u:return "RALT";
        case 0xBAu:return "COLON";
        case 0xBBu:return "PLUS";
        case 0xBCu:return "COMMA";
        case 0xBDu:return "MINUS";
        case 0xBEu:return "PERIOD";
        case 0xBFu:return "SLASH";
        case 0xC0u:return "GRAVE";
        case 0xDBu:return "LBRACKET";
        case 0xDCu:return "BACKSLASH";
        case 0xDDu:return "RBRACKET";
        case 0xDEu:return "QUOTE";
        case menu_bind_mouse2:return "MOUSE2";
        case menu_bind_mouse3:return "MOUSE3";
        case menu_bind_mouse4:return "MOUSE4";
        case menu_bind_mouse5:return "MOUSE5";
        default:return "NONE";
    }
}
inline bool menu_bind_supported_module(
        std::size_t catalog_index) noexcept {
    const auto& catalog=official_module_catalog();
    if(catalog_index>=catalog.size())return false;
    const std::string_view name(catalog[catalog_index].name);
    return name!="Weapons" && name!="Friends" && name!="Teams";
}

struct MenuBindChipRect {
    float x=0.0f,y=0.0f,width=0.0f,height=20.0f;
};
struct MenuBindChipLayout {
    MenuBindChipRect key{};
    MenuBindChipRect hold{};
};

// Shared hit/renderer positions. Key label is source-normalized,
// selected from a bounded 67-entry preview array.
inline std::optional<MenuBindChipLayout> menu_bind_chip_geometry(
        const MenuRenderFrame& frame,float logical_width,
        float logical_height,std::size_t row_index,
        std::size_t catalog_index,std::string_view label,
        bool capturing) noexcept {
    if(frame.unload_confirmation_preview || frame.dropdown_preview ||
       frame.is_placeholder || !menu_settings_offsets_valid(frame) ||
       row_index>=frame.visible.size() ||
       frame.visible[row_index].catalog_index!=catalog_index ||
       !menu_bind_supported_module(catalog_index))
        return std::nullopt;
    const auto rect=menu_module_card_rect(frame,logical_width,row_index);
    if(rect.y+rect.height>logical_height-menu_panel_padding)
        return std::nullopt;
    const float titleLimit=std::max(20.0f,rect.width-108.0f);
    const float estimated=std::min(titleLimit,
        static_cast<float>(frame.visible[row_index].title.size())*8.5f);
    const float x=rect.x+menu_card_inner_padding+estimated+5.0f;
    const float y=rect.y+menu_card_inner_padding;
    const float keyW=capturing?108.0f:
        std::max(41.0f,static_cast<float>(label.size())*7.0f+12.0f);
    const float holdW=45.0f;
    const float toggleLeft=rect.x+rect.width-
        menu_card_inner_padding-menu_switch_width;
    if(x+keyW+5.0f+holdW+4.0f>=toggleLeft ||
       y+20.0f>rect.y+menu_row_body_height(frame))
        return std::nullopt;
    return MenuBindChipLayout{
        {x,y,keyW,20.0f},
        {x+keyW+5.0f,y,holdW,20.0f}
    };
}
} // namespace slinkywinky
