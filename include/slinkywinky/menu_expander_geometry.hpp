#pragma once
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_visual_copy.hpp"
#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>
namespace slinkywinky {
struct MenuExpanderRect {
    float x=0, y=0, width=0, height=20, text_width=0, plus_x=0;
    bool compact=false;
};
// Portable estimate of first-party system font. Commercial font
// metrics are not authenticated by the candidate reference.
inline float menu_expander_text_width(std::string_view s,float font) noexcept {
    float units=0;
    for(char c:s) {
        if(c==' '||c=='\t')units+=0.32f;
        else if(c=='i'||c=='l'||c=='I'||c=='!'||c=='.'||c==','||c==':')units+=0.29f;
        else if(c=='m'||c=='w'||c=='M'||c=='W')units+=0.78f;
        else if(c=='r'||c=='t'||c=='f'||c=='j')units+=0.40f;
        else if(c>='A'&&c<='Z')units+=0.62f;
        else units+=0.53f;
    }
    return units*font;
}
inline std::optional<MenuExpanderRect> menu_expander_geometry(
        const MenuRenderFrame& frame,float width,std::size_t row) noexcept {
    if(row>=frame.visible.size() || frame.tab>=MenuTab::Profiles)
        return std::nullopt;
    const auto& item=frame.visible[row];
    if(item.catalog_index>=67u || recovered_widget_count(item.title)==0u)
        return std::nullopt;
    const auto card=menu_module_card_rect(frame,width,row);
    const bool compact=frame.appearance.compact;
    const auto label=compact?std::string_view(item.title):
        std::string_view(menu_preview_description(item.catalog_index));
    const float text_width=std::min(
        std::max(0.0f,card.width-44.0f),
        menu_expander_text_width(label,compact?16.0f:14.0f));
    const float x=card.x+8.0f;
    return MenuExpanderRect{x,card.y+(compact?8.0f:40.0f),
        text_width+28.0f,20.0f,text_width,x+text_width+8.0f,compact};
}
inline bool menu_expander_contains(const MenuExpanderRect& a,
                                   float x,float y) noexcept {
    return x>=a.x && x<a.x+a.width && y>=a.y && y<a.y+a.height;
}
}
