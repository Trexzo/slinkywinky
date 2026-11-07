#pragma once
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string_view>

namespace slinkywinky {

// R132: shared logical-pixel input geometry for source-identified numeric
// previews. Exact track positions mirror draw_recovered_widget(); width
// and safe clip boundaries are shared by native WGL and hit-testing.
struct MenuNumericTrack {
    float x=0.0f;
    float y=0.0f;
    float width=0.0f;
    float height=18.0f;
};
inline std::optional<MenuNumericTrack> menu_numeric_track_geometry(
        const MenuRenderFrame& frame,float logical_width,
        float logical_height,std::size_t catalog_index,
        std::size_t widget_index) noexcept {
    if(frame.unload_confirmation_preview || frame.dropdown_preview ||
       frame.is_placeholder || !menu_settings_offsets_valid(frame) ||
       widget_index>=recovered_widget_catalog().size() ||
       catalog_index>=official_module_catalog().size() ||
       official_module_catalog()[catalog_index].tab!=frame.tab)
        return std::nullopt;
    const auto& all=recovered_widget_catalog();
    const auto& widget=all[widget_index];
    if(widget.kind!=RecoveredWidgetKind::Slider ||
       std::string_view(widget.module)!=
           official_module_catalog()[catalog_index].name)
        return std::nullopt;
    std::size_t rowIndex=frame.visible.size();
    for(std::size_t i=0;i<frame.visible.size();++i) {
        if(frame.visible[i].catalog_index==catalog_index) {
            rowIndex=i;break;
        }
    }
    if(rowIndex>=frame.visible.size())return std::nullopt;
    const auto& card=frame.visible[rowIndex];
    if(!card.expanded)return std::nullopt;
    std::array<const RecoveredWidget*,5> widgets{};
    if(!recovered_widget_visible_page(card.title.c_str(),
         card.settings_offset,static_cast<int>(frame.aim_preview_mode),
         static_cast<int>(frame.target_preview_mode),widgets))
        return std::nullopt;
    std::size_t slot=widgets.size();
    for(std::size_t i=0;i<widgets.size();++i)
        if(widgets[i]==&widget){slot=i;break;}
    if(slot>=widgets.size())return std::nullopt;
    const auto rect=menu_module_card_rect(frame,logical_width,rowIndex);
    if(rect.y+rect.height>logical_height-menu_panel_padding)
        return std::nullopt;
    float headings=0.0f;
    std::string_view previous=widgets[0]?widgets[0]->group:"";
    for(std::size_t i=1;i<=slot;++i) {
        if(!widgets[i])return std::nullopt;
        const std::string_view current=widgets[i]->group;
        if(current!=previous){headings+=20.0f;previous=current;}
    }
    const float controlY=rect.y+menu_row_body_height(frame)+43.0f+
        27.0f*static_cast<float>(slot)+headings+2.0f;
    MenuNumericTrack track{
        rect.x+rect.width*0.54f,controlY+4.0f,
        std::max(16.0f,rect.width*0.46f-10.0f),18.0f
    };
    if(track.x<rect.x || track.x+track.width>rect.x+rect.width ||
       track.y<rect.y || track.y+track.height>rect.y+rect.height ||
       track.y+track.height>logical_height-menu_panel_padding)
        return std::nullopt;
    return track;
}
inline std::optional<float> menu_slider_value_at_x(
        const RecoveredWidget& widget,
        const MenuNumericTrack& track,float logical_x) noexcept {
    if(widget.kind!=RecoveredWidgetKind::Slider ||
       !std::isfinite(logical_x) ||
       !std::isfinite(track.x) || !std::isfinite(track.width) ||
       track.width<=0.0f ||
       !std::isfinite(widget.min_value) ||
       !std::isfinite(widget.max_value) ||
       !std::isfinite(widget.step) ||
       widget.min_value>=widget.max_value ||
       widget.step<=0.0f)return std::nullopt;
    const double ratio=std::clamp(
        (static_cast<double>(logical_x)-track.x)/track.width,0.0,1.0);
    const double requested=widget.min_value+
        ratio*(static_cast<double>(widget.max_value)-widget.min_value);
    return static_cast<float>(std::clamp(requested,
        static_cast<double>(widget.min_value),
        static_cast<double>(widget.max_value)));
}
} // namespace slinkywinky
