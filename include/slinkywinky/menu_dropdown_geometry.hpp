#pragma once
#include "slinkywinky/menu_setting_layout.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <cmath>

namespace slinkywinky {

// R127: pure geometry/labels for three isolated GUI preview dropdowns.
// 28-pixel option rows and rounded borders are grounded in the recovered
// clickgui.tpl.html reference. No Minecraft input, JNI, packets or globals.
struct MenuDropdownPopupRect {
    float x=0, y=0, width=0, row_height=28.0f;
    unsigned option_count=0; // visible rows, capped at six
    unsigned total_options=0;
    unsigned first_option=0;
    unsigned selected=0;
    std::uint32_t selected_mask=0; // multiselect only
};
inline unsigned menu_source_choice_count(
        std::size_t widget_index) noexcept {
    const auto& all=recovered_widget_catalog();
    if(widget_index>=all.size() ||
       all[widget_index].kind!=RecoveredWidgetKind::Dropdown)return 0;
    const std::string_view options(all[widget_index].options);
    if(options.empty())return 0;
    unsigned count=1;
    bool needLetter=true;
    for(char c:options) {
        if(c=='|') {
            if(needLetter || count>=12u)return 0;
            ++count;
            needLetter=true;
        } else needLetter=false;
    }
    return needLetter?0u:count;
}
inline std::string_view menu_source_choice_label(
        std::size_t widget_index,unsigned ordinal) noexcept {
    if(ordinal>=menu_source_choice_count(widget_index))return {};
    std::string_view options(recovered_widget_catalog()[widget_index].options);
    std::size_t begin=0;
    for(unsigned i=0;i<ordinal;++i) {
        const auto pipe=options.find('|',begin);
        if(pipe==std::string_view::npos)return {};
        begin=pipe+1;
    }
    const auto end=options.find('|',begin);
    return options.substr(begin,end==std::string_view::npos?
                                 end:end-begin);
}
inline unsigned menu_source_multi_count(
        std::size_t widget_index) noexcept {
    const auto& all=recovered_widget_catalog();
    if(widget_index>=all.size() ||
       all[widget_index].kind!=RecoveredWidgetKind::MultiSelect)return 0u;
    const std::string_view options(all[widget_index].options);
    if(options.empty())return 0u;
    unsigned count=1u;
    bool needs_text=true;
    for(const char c:options) {
        if(c=='|') {
            if(needs_text || count>=16u)return 0u;
            ++count;
            needs_text=true;
        } else needs_text=false;
    }
    return needs_text?0u:count;
}
inline std::string_view menu_source_multi_label(
        std::size_t widget_index,unsigned ordinal) noexcept {
    if(ordinal>=menu_source_multi_count(widget_index))return {};
    std::string_view options(recovered_widget_catalog()[widget_index].options);
    std::size_t start=0;
    for(unsigned i=0;i<ordinal;++i) {
        const auto pos=options.find('|',start);
        if(pos==std::string_view::npos)return {};
        start=pos+1;
    }
    const auto end=options.find('|',start);
    return options.substr(start,end==std::string_view::npos?
                          end:end-start);
}
inline unsigned menu_dropdown_option_count(
        MenuDropdownKind kind,std::size_t widget_index=0) noexcept {
    switch(kind) {
        case MenuDropdownKind::AimMode:return 4u;
        case MenuDropdownKind::TargetMode:return 2u;
        case MenuDropdownKind::PiercingSort:return 2u;
        case MenuDropdownKind::RecoveredChoice:
            return menu_source_choice_count(widget_index);
        case MenuDropdownKind::RecoveredMulti:
            return menu_source_multi_count(widget_index);
        default:return 0u;
    }
}
inline std::string_view menu_dropdown_option_label(
        MenuDropdownKind kind,unsigned ordinal,
        std::size_t widget_index=0) noexcept {
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
        case MenuDropdownKind::RecoveredChoice:
            return menu_source_choice_label(widget_index,ordinal);
        case MenuDropdownKind::RecoveredMulti:
            return menu_source_multi_label(widget_index,ordinal);
        default:return {};
    }
}
inline std::optional<MenuDropdownPopupRect> menu_dropdown_popup_geometry(
        const MenuRenderFrame& frame,float logical_width,
        float logical_height) noexcept {
    if(!frame.dropdown_preview || frame.is_placeholder ||
       logical_width<320.0f ||
       logical_height<240.0f)return std::nullopt;
    const auto popup=*frame.dropdown_preview;
    if(popup.kind!=MenuDropdownKind::RecoveredChoice &&
       popup.kind!=MenuDropdownKind::RecoveredMulti &&
       frame.tab!=MenuTab::Combat)return std::nullopt;
    const unsigned options=menu_dropdown_option_count(
        popup.kind,popup.widget_index);
    if(!options)return std::nullopt;
    const std::size_t expected_index=
        (popup.kind==MenuDropdownKind::RecoveredChoice ||
         popup.kind==MenuDropdownKind::RecoveredMulti)?
        popup.catalog_index:
        (popup.kind==MenuDropdownKind::PiercingSort?11u:0u);
    if(expected_index>=official_module_catalog().size() ||
       official_module_catalog()[expected_index].tab!=frame.tab)
        return std::nullopt;
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
    if(!count || position>=widgets.size() || !widgets[position])
        return std::nullopt;
    if(popup.kind==MenuDropdownKind::RecoveredMulti) {
        if(widgets[position]->kind!=RecoveredWidgetKind::MultiSelect)
            return std::nullopt;
    } else if(widgets[position]->kind!=RecoveredWidgetKind::Dropdown)
        return std::nullopt;
    const auto& widget=*widgets[position];
    if(popup.kind==MenuDropdownKind::RecoveredChoice ||
       popup.kind==MenuDropdownKind::RecoveredMulti) {
        const auto& all=recovered_widget_catalog();
        if(popup.widget_index>=all.size() ||
           widgets[position]!=&all[popup.widget_index])
            return std::nullopt;
    }
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
    result.x=menu_setting_column(rect.x,rect.width).x;
    result.width=menu_setting_column(rect.x,rect.width).width;
    result.total_options=options;
    result.option_count=std::min(options,6u);
    if(popup.option_offset>options-result.option_count)
        return std::nullopt;
    result.first_option=popup.option_offset;
    if(popup.kind==MenuDropdownKind::RecoveredMulti) {
        result.selected_mask=
            frame.recovered_multi_overrides[popup.widget_index].value_or(0u);
        if((result.selected_mask>>options)!=0u)return std::nullopt;
    } else if(popup.kind==MenuDropdownKind::RecoveredChoice) {
        const float example=widget.example_value;
        if(!std::isfinite(example) || example<0.0f ||
           example>=static_cast<float>(options))
            return std::nullopt;
        result.selected=frame.recovered_choice_overrides[popup.widget_index]
            .value_or(static_cast<unsigned>(example));
    } else result.selected=popup.kind==MenuDropdownKind::AimMode?
        frame.aim_preview_mode:
        (popup.kind==MenuDropdownKind::TargetMode?
         frame.target_preview_mode:frame.piercing_preview.sort);
    if(popup.kind!=MenuDropdownKind::RecoveredMulti &&
       result.selected>=options)return std::nullopt;
    const float height=result.row_height*
        static_cast<float>(result.option_count);
    result.y=controlY;
    // Open upward when the reference-style option list would be clipped.
    // If no full, safe placement exists, the popup does not receive input.
    if(result.y+height>logical_height-menu_panel_padding)
        result.y=controlY-height+20.0f;
    if(result.y<menu_first_row_y ||
       result.y+height>logical_height-menu_panel_padding ||
       result.x<menu_panel_padding ||
       result.x+result.width>logical_width-menu_panel_padding)
        return std::nullopt;
    return result;
}
} // namespace slinkywinky
