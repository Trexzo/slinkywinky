#pragma once
#include "slinkywinky/menu_numeric_geometry.hpp"
#include "slinkywinky/menu_numeric_formats.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>

namespace slinkywinky {

// Source HTML #tip: height 20px, padding 0 5px, radius 5px,
// position:fixed, pointer-events:none; center follows slider value.
struct MenuNumericTooltip {
    float x=0.0f;
    float y=0.0f;
    float width=0.0f;
    float height=20.0f;
    std::string label;
};

inline std::optional<MenuNumericTooltip> menu_numeric_tooltip_geometry(
        const MenuRenderFrame& frame,float width,
        float height) {
    if(!frame.numeric_hover || frame.unload_confirmation_preview ||
       frame.dropdown_preview || frame.profile_action_preview ||
       frame.is_placeholder || width<120.0f || height<100.0f)
        return std::nullopt;
    const auto id=*frame.numeric_hover;
    const auto& all=recovered_widget_catalog();
    if(id.widget_index>=all.size())return std::nullopt;
    const auto& w=all[id.widget_index];
    if(w.kind!=RecoveredWidgetKind::Slider &&
       w.kind!=RecoveredWidgetKind::Range)return std::nullopt;
    const auto track=menu_numeric_track_geometry(
        frame,width,height,id.catalog_index,id.widget_index);
    if(!track)return std::nullopt;
    if(!std::isfinite(w.min_value) || !std::isfinite(w.max_value) ||
       w.max_value<=w.min_value)return std::nullopt;
    const float first=w.kind==RecoveredWidgetKind::Slider?
        frame.recovered_slider_overrides[id.widget_index].value_or(
            w.example_value):
        frame.recovered_range_overrides[id.widget_index].value_or(
            RecoveredRangePreview{w.example_value,w.example_other}).lower;
    const std::optional<float> second=
        w.kind==RecoveredWidgetKind::Range?
        std::optional<float>(frame.recovered_range_overrides[id.widget_index]
            .value_or(RecoveredRangePreview{
                w.example_value,w.example_other}).upper):std::nullopt;
    if(!std::isfinite(first) || first<w.min_value ||
       first>w.max_value || (second &&
       (!std::isfinite(*second) || *second<first ||
        *second>w.max_value)))return std::nullopt;
    const auto format=menu_numeric_format_for_widget(id.widget_index);
    if(format.empty())return std::nullopt;
    const auto caption=menu_format_numeric_values(format,first,second);
    if(!caption || caption->empty())return std::nullopt;
    // A tooltip is bounded to the current logical panel; the source used
    // translateX(-50%) and a top offset of -28px from its value anchor.
    const float tipW=std::min(width-16.0f,
        static_cast<float>(caption->size())*7.4f+10.0f);
    if(tipW<12.0f)return std::nullopt;
    const float center=second?0.5f*(first+*second):first;
    const float ratio=std::clamp(
        (center-w.min_value)/(w.max_value-w.min_value),0.0f,1.0f);
    const float centerX=track->x+track->width*ratio;
    const float tooltipX=std::clamp(
        centerX-tipW*0.5f,8.0f,width-tipW-8.0f);
    const float tooltipY=std::clamp(
        track->y-22.0f,8.0f,height-28.0f);
    return MenuNumericTooltip{
        tooltipX,tooltipY,tipW,20.0f,*caption
    };
}
} // namespace slinkywinky
