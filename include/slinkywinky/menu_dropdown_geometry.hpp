#pragma once
#include "slinkywinky/menu_settings_vertical.hpp"
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
    unsigned first_selectable_option=0; // R226: original search excludes None.
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
// Pinned original gui_controls.cpp uses items+1, n-1 for a large
// optional selector. Only this ten-choice recovered source has a
// verified None entry and a dedicated right-hand clear control.
inline bool menu_source_large_optional_choice(
        std::size_t widget_index) noexcept {
    const auto& all=recovered_widget_catalog();
    if(widget_index>=all.size())return false;
    const auto& widget=all[widget_index];
    return widget.kind==RecoveredWidgetKind::Dropdown &&
           std::string_view(widget.module)=="Inv Manager" &&
           std::string_view(widget.group)=="Sort hotbar" &&
           std::string_view(widget.label)=="Slot 1" &&
           menu_source_choice_count(widget_index)==10u &&
           menu_source_choice_label(widget_index,0u)=="None";
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
// The source X and its actual hit box share pixel-identical geometry.
// Never expose a clear target while the popup or a modal owns input.
struct MenuDropdownClearRect {
    float x=0.0f,y=0.0f,width=20.0f,height=20.0f;
    bool contains(float px,float py) const noexcept {
        return px>=x && px<x+width && py>=y && py<y+height;
    }
};
inline std::optional<MenuDropdownClearRect> menu_dropdown_clear_geometry(
        const MenuRenderFrame& frame,float logical_width,
        float logical_height,std::size_t module,std::size_t widget_index) noexcept {
    if(!menu_source_large_optional_choice(widget_index) ||
       frame.dropdown_preview || frame.unload_confirmation_preview ||
       frame.profile_action_preview || frame.is_placeholder ||
       module>=official_module_catalog().size() ||
       official_module_catalog()[module].tab!=frame.tab ||
       logical_width<320.0f || logical_height<240.0f)
        return std::nullopt;
    const auto& all=recovered_widget_catalog();
    const auto& source=all[widget_index];
    const auto total=menu_source_choice_count(widget_index);
    if(!std::isfinite(source.example_value) || source.example_value<0.0f ||
       source.example_value>=static_cast<float>(total))return std::nullopt;
    const unsigned selected=frame.recovered_choice_overrides[widget_index]
        .value_or(static_cast<unsigned>(source.example_value));
    if(selected==0u || selected>=total)return std::nullopt;
    for(std::size_t i=0;i<frame.visible.size();++i) {
        const auto& row=frame.visible[i];
        if(row.catalog_index!=module)continue;
        if(!row.expanded || row.title!=official_module_catalog()[module].name)
            return std::nullopt;
        std::array<const RecoveredWidget*,5> widgets{};
        if(!recovered_widget_visible_page(row.title.c_str(),row.settings_offset,
              static_cast<int>(frame.aim_preview_mode),
              static_cast<int>(frame.target_preview_mode),widgets))
            return std::nullopt;
        for(std::size_t j=0;j<widgets.size();++j) {
            if(widgets[j]!=&source)continue;
            const auto rect=menu_module_card_rect(frame,logical_width,i);
            if(rect.y+rect.height>logical_height-menu_panel_padding)
                return std::nullopt;
            const auto col=menu_setting_column(rect.x,rect.width);
            const float y=rect.y+menu_row_body_height(frame)+
                menu_settings_control_offset(widgets,j);
            const float x=col.x+col.width-20.0f;
            if(x<menu_panel_padding || x+20.0f>
                   logical_width-menu_panel_padding ||
               y<menu_first_row_y || y+20.0f>
                   logical_height-menu_panel_padding)
                return std::nullopt;
            return MenuDropdownClearRect{x,y,20.0f,20.0f};
        }
        return std::nullopt;
    }
    return std::nullopt;
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
    const float controlY=rect.y+menu_row_body_height(frame)+
        menu_settings_control_offset(widgets,position);
    MenuDropdownPopupRect result;
    result.x=menu_setting_column(rect.x,rect.width).x;
    result.width=menu_setting_column(rect.x,rect.width).width;
    // Source optional selections skip the None entry, using absolute
    // source option indices for hit-testing and final storage.
    const unsigned excluded=popup.kind==MenuDropdownKind::RecoveredChoice &&
        menu_source_large_optional_choice(popup.widget_index)?1u:0u;
    const unsigned available=options-excluded;
    result.total_options=available;
    result.option_count=std::min(available,6u);
    result.first_selectable_option=excluded;
    if(popup.option_offset<excluded ||
       popup.option_offset>options-result.option_count)
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
// R227: original gui_popups.cpp kind 4 is a searchable Select item
// modal with two columns of 25px rows, not an inline six-item scroller.
// Keep the source indices absolute; index zero is the optional None/X.
struct MenuSearchRect {
    float x=0.0f,y=0.0f,width=0.0f,height=0.0f;
    bool contains(float px,float py) const noexcept {
        return px>=x && px<x+width && py>=y && py<y+height;
    }
};
struct MenuSearchMatches {
    std::array<unsigned,30> indices{};
    unsigned count=0u;
};
inline char menu_search_ascii_lower(char c) noexcept {
    return c>='A' && c<='Z'?static_cast<char>(c-'A'+'a'):c;
}
inline bool menu_search_ascii_valid(std::string_view query) noexcept {
    if(query.size()>32u)return false;
    for(const char c:query)
        if(!((c>='a'&&c<='z') || (c>='A'&&c<='Z') ||
             (c>='0'&&c<='9') || c==' '))return false;
    return true;
}
inline MenuSearchMatches menu_source_search_matches(
        std::size_t widget,std::string_view query) noexcept {
    MenuSearchMatches result;
    if(!menu_source_large_optional_choice(widget) ||
       !menu_search_ascii_valid(query))return result;
    const unsigned total=menu_source_choice_count(widget);
    for(unsigned index=1u;index<total;++index) {
        const auto label=menu_source_choice_label(widget,index);
        if(query.size()>label.size())continue;
        bool matched=false;
        for(std::size_t offset=0;offset+query.size()<=label.size();++offset) {
            std::size_t i=0;
            while(i<query.size() &&
                  menu_search_ascii_lower(label[offset+i])==
                  menu_search_ascii_lower(query[i]))++i;
            if(i==query.size()){matched=true;break;}
        }
        if(matched && result.count<result.indices.size())
            result.indices[result.count++]=index;
    }
    return result;
}
struct MenuSearchPopupGeometry {
    MenuSearchRect panel{},title{},close{},field{};
    MenuSearchMatches matches{};
    MenuSearchRect result_cell(unsigned ordinal) const noexcept {
        if(ordinal>=matches.count)return {};
        const float w=(panel.width-24.0f)*0.5f;
        return {panel.x+12.0f+(ordinal%2u)*w,
                field.y+field.height+8.0f+(ordinal/2u)*25.0f,
                w,25.0f};
    }
};
inline std::optional<MenuSearchPopupGeometry> menu_search_popup_geometry(
        const MenuRenderFrame& frame,float width,float height) noexcept {
    if(!frame.dropdown_preview ||
       frame.dropdown_preview->kind!=MenuDropdownKind::RecoveredChoice ||
       !menu_source_large_optional_choice(
           frame.dropdown_preview->widget_index) ||
       !menu_search_ascii_valid(frame.dropdown_search_query) ||
       width<320.0f || height<240.0f ||
       frame.dropdown_preview->option_offset!=1u ||
       frame.is_placeholder || frame.unload_confirmation_preview ||
       frame.profile_action_preview)
        return std::nullopt;
    // The source modal is admitted only when its underlying typed
    // dropdown is genuinely on the current visible settings page.
    const auto popup=*frame.dropdown_preview;
    const auto& all=recovered_widget_catalog();
    const auto& catalog=official_module_catalog();
    if(popup.catalog_index>=catalog.size() ||
       popup.widget_index>=all.size() ||
       catalog[popup.catalog_index].tab!=frame.tab ||
       std::string_view(all[popup.widget_index].module)!=
           catalog[popup.catalog_index].name)
        return std::nullopt;
    bool source_visible=false;
    for(const auto& row:frame.visible) {
        if(row.catalog_index!=popup.catalog_index || !row.expanded ||
           row.settings_offset!=popup.page_offset)continue;
        std::array<const RecoveredWidget*,5> widgets{};
        if(!recovered_widget_visible_page(row.title.c_str(),
              row.settings_offset,static_cast<int>(frame.aim_preview_mode),
              static_cast<int>(frame.target_preview_mode),widgets))
            continue;
        for(std::size_t i=0;i<widgets.size();++i)
            if(widgets[i]==&all[popup.widget_index] &&
               popup.control_index==row.settings_offset+i)
                source_visible=true;
    }
    if(!source_visible)return std::nullopt;
    MenuSearchPopupGeometry result;
    result.matches=menu_source_search_matches(
        popup.widget_index,frame.dropdown_search_query);
    const float rows=static_cast<float>(
        std::max(1u,(result.matches.count+1u)/2u));
    const float modalWidth=std::min(420.0f,width-32.0f);
    const float modalHeight=12.0f+20.0f+12.0f+30.0f+
        8.0f+rows*25.0f+12.0f;
    if(modalHeight>height-32.0f)return std::nullopt;
    result.panel={(width-modalWidth)*0.5f,(height-modalHeight)*0.5f,
                  modalWidth,modalHeight};
    result.title={result.panel.x+12.0f,result.panel.y+12.0f,
                  modalWidth-24.0f,20.0f};
    result.close={result.title.x+result.title.width-20.0f,
                  result.title.y,20.0f,20.0f};
    result.field={result.panel.x+12.0f,result.title.y+32.0f,
                  modalWidth-24.0f,30.0f};
    return result;
}
} // namespace slinkywinky
