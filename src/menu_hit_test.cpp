#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_dropdown_geometry.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
#include "slinkywinky/menu_color_geometry.hpp"
#include "slinkywinky/menu_text_geometry.hpp"
#include "slinkywinky/menu_bind_preview.hpp"
#include "slinkywinky/menu_unload_modal_geometry.hpp"
#include "slinkywinky/menu_profile_preview.hpp"
#include "slinkywinky/menu_profile_interaction_geometry.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <cmath>

namespace slinkywinky {
std::optional<MenuPanelGeometry> menu_panel_geometry(
    unsigned viewport_width,unsigned viewport_height,bool wide,
    MenuPanelMode mode) noexcept {
    if(viewport_width<320u || viewport_width>8192u ||
       viewport_height<240u || viewport_height>8192u)
        return std::nullopt;
    const bool standalone=mode==MenuPanelMode::StandalonePreview;
    const bool compact=mode==MenuPanelMode::CompactOverlay;
    if(!standalone && !compact && mode!=MenuPanelMode::BoundedOverlay)
        return std::nullopt;
    const unsigned pane_width=standalone?viewport_width:
        std::min<unsigned>(compact?480u:(wide?1500u:900u),viewport_width-24u);
    const unsigned pane_height=standalone?viewport_height:
        std::min<unsigned>(compact?420u:630u,viewport_height-24u);
    if(!pane_width || !pane_height)return std::nullopt;
    MenuPanelGeometry pane;
    pane.left=(viewport_width-pane_width)/2u;
    pane.top=(viewport_height-pane_height)/2u;
    pane.width=pane_width;
    pane.height=pane_height;
    pane.scale=compact?0.70f:1.0f;
    return pane;
}
std::optional<MenuHit> hit_test_menu(
    const MenuRenderFrame& frame,MenuPanelMode mode,
    int client_x,int client_y) noexcept {
    const auto panel=menu_panel_geometry(
        frame.viewport_width,frame.viewport_height,frame.appearance.wide,mode);
    if(!panel || !menu_settings_offsets_valid(frame) ||
       !menu_profile_preview_valid(
            frame,panel->logical_width(),panel->logical_height()) ||
       !frame.epoch || !frame.sequence ||
       frame.visible.size()>8 ||
       frame.tab==MenuTab::Count || frame.tab==MenuTab::Unload ||
       (frame.is_placeholder!=(frame.tab==MenuTab::Profiles)))
        return std::nullopt;
    const auto& catalog=official_module_catalog();
    std::size_t prior_index=0;
    bool first_row=true;
    for(const auto& row:frame.visible) {
        if(row.catalog_index>=catalog.size() ||
           catalog[row.catalog_index].tab!=frame.tab ||
           row.title!=catalog[row.catalog_index].name ||
           (!first_row && row.catalog_index<=prior_index))
            return std::nullopt;
        prior_index=row.catalog_index;
        first_row=false;
    }
    if(frame.is_placeholder && !frame.visible.empty())
        return std::nullopt;
    // Bound before subtraction: negative or outside-panel client input may
    // never wrap to the opposite edge or reach clipped/offscreen controls.
    if(client_x<0 || client_y<0 ||
       client_x<static_cast<int>(panel->left) ||
       client_y<static_cast<int>(panel->top) ||
       client_x>=static_cast<int>(panel->left+panel->width) ||
       client_y>=static_cast<int>(panel->top+panel->height))
        return std::nullopt;
    const float x=static_cast<float>(
        client_x-static_cast<int>(panel->left))/panel->scale;
    const float y=static_cast<float>(
        client_y-static_cast<int>(panel->top))/panel->scale;
    const float w=panel->logical_width();
    // R128: confirmation owns the entire pointer surface. Only the
    // recovered X and Continue button can be hit; no click-through.
    if(frame.profile_action_preview) {
        const auto modal=menu_profile_modal_geometry(
            frame,w,panel->logical_height());
        if(!modal)return std::nullopt;
        if(x>=modal->close_x && x<modal->close_x+20.0f &&
           y>=modal->close_y && y<modal->close_y+20.0f)
            return MenuHit{MenuHitKind::ProfileModalClose,frame.tab,0};
        if(x>=modal->continue_x &&
           x<modal->continue_x+modal->continue_width &&
           y>=modal->continue_y && y<modal->continue_y+20.0f)
            return MenuHit{MenuHitKind::ProfileModalContinue,frame.tab,0};
        return std::nullopt; // no click-through from modal background
    }
    if(frame.unload_confirmation_preview) {
        const auto modal=menu_unload_modal_geometry(
            frame,w,panel->logical_height());
        if(!modal)return std::nullopt;
        if(x>=modal->close_x && x<modal->close_x+20.0f &&
           y>=modal->close_y && y<modal->close_y+20.0f)
            return MenuHit{MenuHitKind::ModalClose,frame.tab,0};
        if(x>=modal->continue_x &&
           x<modal->continue_x+modal->continue_width &&
           y>=modal->continue_y && y<modal->continue_y+20.0f)
            return MenuHit{MenuHitKind::ModalContinue,frame.tab,0};
        return std::nullopt;
    }
    // R127: an open dropdown owns the only admissible mouse targets.
    // No click may pass through the floating popup to modules or nav tabs.
    if(frame.dropdown_preview) {
        const auto pop=menu_dropdown_popup_geometry(
            frame,w,panel->logical_height());
        if(!pop || x<pop->x || x>=pop->x+pop->width ||
           y<pop->y || y>=pop->y+pop->row_height*pop->option_count)
            return std::nullopt;
        const auto option=static_cast<std::size_t>(
            (y-pop->y)/pop->row_height);
        if(option>=pop->option_count)return std::nullopt;
        return MenuHit{MenuHitKind::DropdownOption,frame.tab,
                       frame.dropdown_preview->catalog_index,
                       pop->first_option+option};
    }
    // R113: exactly the same shared geometry as the Slinky-style renderer.
    // "Unload" is displayed in red, but is NEVER an admitted mouse action.
    if(y>=menu_nav_y && y<menu_nav_y+menu_nav_height &&
       x>=menu_nav_x && x<w-menu_nav_x) {
        const unsigned tab=static_cast<unsigned>(
            (x-menu_nav_x)/menu_tab_width(w));
        if(tab<6u)return MenuHit{
            MenuHitKind::Tab,static_cast<MenuTab>(tab),0};
        if(tab==6u)return MenuHit{
            MenuHitKind::UnloadTab,MenuTab::Unload,0};
        return std::nullopt;
    }
    if(frame.tab==MenuTab::Profiles) {
        for(std::size_t i=0;i<frame.profile_cards.size();++i) {
            const auto bind=menu_profile_bind_rect(
                frame,w,panel->logical_height(),i);
            if(bind && menu_profile_contains(*bind,x,y))
                return MenuHit{MenuHitKind::ProfileBindChip,frame.tab,i};
        }
        // Source icon row: four 20x20 controls 12px apart, ordered
        // load/save/folder/delete. Default Save/Delete are disabled.
        for(std::size_t i=0;i<frame.profile_cards.size();++i) {
            for(unsigned action=0;action<4u;++action) {
                const auto rect=menu_profile_action_rect(
                    frame,w,panel->logical_height(),i,action);
                if(rect && menu_profile_contains(*rect,x,y))
                    return MenuHit{MenuHitKind::ProfileAction,frame.tab,
                                   i,action};
            }
        }
        const auto name=menu_profile_input_rect(
            frame,w,panel->logical_height());
        if(name && menu_profile_contains(*name,x,y))
            return MenuHit{MenuHitKind::ProfileNameField,frame.tab,
                           frame.profile_cards.size()};
        const auto create=menu_profile_create_rect(
            frame,w,panel->logical_height());
        if(create && menu_profile_contains(*create,x,y))
            return MenuHit{MenuHitKind::ProfileCreate,frame.tab,
                           frame.profile_cards.size()};
        return std::nullopt;
    }
    if(frame.tab>=MenuTab::Profiles)return std::nullopt;
    if(x<menu_panel_padding || x>=w-menu_panel_padding)
        return std::nullopt;
    const float body=menu_row_body_height(frame);
    for(std::size_t i=0;i<frame.visible.size();++i) {
        const auto rect=menu_module_card_rect(frame,w,i);
        // Match the renderer's recovered grid, including the real column
        // gap. Never dispatch an input through a clipped or expanded pane.
        if(rect.y+rect.height>panel->logical_height()-menu_panel_padding)
            continue;
        if(x<rect.x || x>=rect.x+rect.width ||
           y<rect.y || y>=rect.y+rect.height)continue;
        const std::size_t index=frame.visible[i].catalog_index;
        // R140: source-style key and HOLD chips live in the card's
        // first row, even when a module's settings are expanded. Reject
        // missing/hidden/clipped or no-bind module chips by construction.
        const auto& bind=frame.bind_previews[index];
        const bool capturing=frame.bind_capture_index==index;
        const std::string label=capturing?"PRESS A KEY...":
            menu_bind_code_label(bind.code);
        const auto chips=menu_bind_chip_geometry(
            frame,w,panel->logical_height(),i,index,label,capturing);
        if(chips && y>=chips->key.y &&
           y<chips->key.y+chips->key.height) {
            if(x>=chips->key.x &&
               x<chips->key.x+chips->key.width)
                return MenuHit{MenuHitKind::BindChip,frame.tab,index};
            if(bind.code &&
               (bind.hold || frame.bind_hover_row==index) &&
               x>=chips->hold.x &&
               x<chips->hold.x+chips->hold.width)
                return MenuHit{MenuHitKind::BindHold,frame.tab,index};
        }
        // R126: only source-identified dropdowns in Aim Assist and the
        // existing Piercing sandbox can mutate local preview state.
        // The hit geometry mirrors the GL11 renderer in all three modes.
        if(frame.visible[i].expanded && y>=rect.y+body) {
            std::array<const RecoveredWidget*,5> widgets{};
            const auto& card=frame.visible[i];
            const auto count=recovered_widget_visible_page(
                card.title.c_str(),card.settings_offset,
                static_cast<int>(frame.aim_preview_mode),
                static_cast<int>(frame.target_preview_mode),widgets);
            if(!count || (index==11 && count!=8u))
                return std::nullopt; // source shape contract
            const float controlX=rect.x+rect.width*0.54f;
            const float controlW=std::max(16.0f,rect.width*0.46f-10.0f);
            const float settingsY=rect.y+body;
            float extraHeadings=0.0f;
            std::string_view previousGroup=widgets[0]?
                std::string_view(widgets[0]->group):std::string_view{};
            for(std::size_t j=0;j<widgets.size() && widgets[j];++j) {
                const std::string_view group(widgets[j]->group);
                if(j>0 && group!=previousGroup) {
                    extraHeadings+=20.0f;
                    previousGroup=group;
                }
                const auto& widget=*widgets[j];
                const std::size_t control=card.settings_offset+j;
                const float controlY=settingsY+43.0f+
                    27.0f*static_cast<float>(j)+extraHeadings+2.0f;
                const bool dropdown=widget.kind==RecoveredWidgetKind::Dropdown &&
                    x>=controlX && x<controlX+controlW &&
                    y>=controlY && y<controlY+22.0f;
                if(index==0) {
                    // Only two known standalone preview enums have a
                    // writable owner. All other recovered widgets are inert.
                    if(dropdown && std::string_view(widget.label)=="Mode") {
                        if(control==0u &&
                           std::string_view(widget.group)=="Aim")
                            return MenuHit{MenuHitKind::AimModeControl,
                                           frame.tab,index,control};
                        if(std::string_view(widget.group)=="Target")
                            return MenuHit{MenuHitKind::TargetModeControl,
                                           frame.tab,index,control};
                    }
                }
                if(index==11u) {
                    if(control>=8u)return std::nullopt;
                    if(dropdown && control==0u)
                        return MenuHit{MenuHitKind::PiercingControl,
                                       frame.tab,index,control};
                    if(widget.kind==RecoveredWidgetKind::Boolean &&
                       control>0u && x>=controlX+controlW-34.0f &&
                       x<controlX+controlW &&
                       y>=controlY+1.0f && y<controlY+19.0f)
                        return MenuHit{MenuHitKind::PiercingControl,
                                       frame.tab,index,control};
                    continue;
                }
                if(dropdown) {
                    const auto& all=recovered_widget_catalog();
                    const auto global=static_cast<std::size_t>(
                        widgets[j]-all.data());
                    if(global>=all.size())return std::nullopt;
                    return MenuHit{MenuHitKind::RecoveredChoiceControl,
                                   frame.tab,index,global};
                }
                if(widget.kind==RecoveredWidgetKind::Text) {
                    const auto& all=recovered_widget_catalog();
                    const auto global=static_cast<std::size_t>(
                        widgets[j]-all.data());
                    const auto field=menu_text_field_geometry(
                        frame,w,panel->logical_height(),index,global);
                    if(field && x>=field->x &&
                       x<field->x+field->width &&
                       y>=field->y &&
                       y<field->y+field->height)
                        return MenuHit{MenuHitKind::RecoveredTextField,
                                       frame.tab,index,global};
                }
                if(widget.kind==RecoveredWidgetKind::Color) {
                    const auto& all=recovered_widget_catalog();
                    const auto global=static_cast<std::size_t>(
                        widgets[j]-all.data());
                    const auto swatch=menu_color_swatch_geometry(
                        frame,w,panel->logical_height(),index,global);
                    if(swatch && x>=swatch->x &&
                       x<swatch->x+swatch->width &&
                       y>=swatch->y &&
                       y<swatch->y+swatch->height)
                        return MenuHit{MenuHitKind::RecoveredColorSwatch,
                                       frame.tab,index,global};
                }
                if(widget.kind==RecoveredWidgetKind::MultiSelect &&
                   x>=controlX && x<controlX+controlW &&
                   y>=controlY && y<controlY+22.0f) {
                    const auto& all=recovered_widget_catalog();
                    const auto global=static_cast<std::size_t>(
                        widgets[j]-all.data());
                    if(global>=all.size())return std::nullopt;
                    return MenuHit{MenuHitKind::RecoveredMultiControl,
                                   frame.tab,index,global};
                }
                // R132: the same numeric track geometry is used to
                // calculate release values in the Windows preview. A hit
                // always identifies the immutable recovered widget ID.
                if(widget.kind==RecoveredWidgetKind::Slider) {
                    const auto& all=recovered_widget_catalog();
                    const auto global=static_cast<std::size_t>(
                        widgets[j]-all.data());
                    const auto numeric=menu_numeric_track_geometry(
                        frame,w,panel->logical_height(),index,global);
                    if(numeric && x>=numeric->x &&
                       x<numeric->x+numeric->width &&
                       y>=numeric->y &&
                       y<numeric->y+numeric->height)
                        return MenuHit{MenuHitKind::RecoveredSliderPreview,
                                       frame.tab,index,global};
                }
                if(widget.kind==RecoveredWidgetKind::Range) {
                    const auto& all=recovered_widget_catalog();
                    const auto global=static_cast<std::size_t>(
                        widgets[j]-all.data());
                    const auto track=menu_numeric_track_geometry(
                        frame,w,panel->logical_height(),index,global);
                    if(track && x>=track->x &&
                       x<track->x+track->width &&
                       y>=track->y &&
                       y<track->y+track->height) {
                        const auto value=frame.recovered_range_overrides[global]
                            .value_or(RecoveredRangePreview{
                                widget.example_value,widget.example_other});
                        const float span=widget.max_value-widget.min_value;
                        if(!std::isfinite(span) || span<=0.0f ||
                           !std::isfinite(value.lower) ||
                           !std::isfinite(value.upper) ||
                           value.lower<widget.min_value ||
                           value.upper>widget.max_value ||
                           value.lower>value.upper)
                            return std::nullopt;
                        const float lowerX=track->x+track->width*
                            ((value.lower-widget.min_value)/span);
                        const float upperX=track->x+track->width*
                            ((value.upper-widget.min_value)/span);
                        // Same nearest-handle choice as the recovered
                        // clickgui.tpl.html pointerdown algorithm. Ties
                        // deterministically select the lower endpoint.
                        const unsigned handle=std::fabs(x-lowerX)<=
                            std::fabs(x-upperX)?0u:1u;
                        return MenuHit{MenuHitKind::RecoveredRangePreview,
                                       frame.tab,index,global,handle};
                    }
                }
                // Generic typed Boolean fixture: the pointer returned by
                // recovered_widget_visible_page identifies the exact
                // callsite, including Aim's conditional branches.
                if(widget.kind==RecoveredWidgetKind::Boolean &&
                   x>=controlX+controlW-34.0f &&
                   x<controlX+controlW &&
                   y>=controlY+1.0f && y<controlY+19.0f) {
                    const auto& all=recovered_widget_catalog();
                    const auto global=static_cast<std::size_t>(
                        widgets[j]-all.data());
                    if(global>=all.size())return std::nullopt;
                    return MenuHit{MenuHitKind::RecoveredBooleanPreview,
                                   frame.tab,index,global};
                }
            }
            return std::nullopt; // headings and body are never actions
        }
        if(y>=rect.y+body)continue;
        const bool implemented=
            index<catalog.size() && module_has_profile_setting(index);
        const float toggleX=rect.x+rect.width-
                            menu_card_inner_padding-menu_switch_width;
        const float toggleY=rect.y+menu_card_inner_padding;
        if(implemented &&
           x>=toggleX && x<toggleX+menu_switch_width &&
           y>=toggleY && y<toggleY+menu_switch_height)
            return MenuHit{MenuHitKind::Toggle,frame.tab,index};
        const float expandX=frame.appearance.compact?
                            rect.x+8.0f:rect.x+20.0f;
        const float expandY=frame.appearance.compact?
                            rect.y+8.0f:rect.y+40.0f;
        if(recovered_widget_count(frame.visible[i].title)>0 &&
           x>=expandX && x<expandX+menu_expand_size &&
           y>=expandY && y<expandY+menu_expand_size)
            return MenuHit{MenuHitKind::Expand,frame.tab,index};
        return MenuHit{MenuHitKind::RowBody,frame.tab,index};
    }
    return std::nullopt;
}
std::optional<std::size_t> settings_page_wheel_target(
        const MenuRenderFrame& frame,MenuPanelMode mode,
        int client_x,int client_y) noexcept {
    const auto panel=menu_panel_geometry(
        frame.viewport_width,frame.viewport_height,
        frame.appearance.wide,mode);
    if(!panel || !frame.epoch || !frame.sequence ||
       frame.tab>MenuTab::Utility || frame.is_placeholder ||
       frame.visible.empty() || frame.visible.size()>8 ||
       !menu_settings_offsets_valid(frame) ||
       !menu_profile_preview_valid(
           frame,panel->logical_width(),panel->logical_height()) ||
       client_x<static_cast<int>(panel->left) ||
       client_y<static_cast<int>(panel->top) ||
       client_x>=static_cast<int>(panel->left+panel->width) ||
       client_y>=static_cast<int>(panel->top+panel->height))
        return std::nullopt;
    const auto& catalog=official_module_catalog();
    std::size_t previous=0;
    bool first=true;
    // Validate the ENTIRE snapshot before returning any wheel target.
    // A forged late row must not be hidden by an early valid card hit.
    for(const auto& item:frame.visible) {
        if(item.catalog_index>=catalog.size() ||
           catalog[item.catalog_index].tab!=frame.tab ||
           item.title!=catalog[item.catalog_index].name ||
           (!first && item.catalog_index<=previous))
            return std::nullopt;
        previous=item.catalog_index;first=false;
    }
    const float logicalX=static_cast<float>(
        client_x-static_cast<int>(panel->left))/panel->scale;
    const float logicalY=static_cast<float>(
        client_y-static_cast<int>(panel->top))/panel->scale;
    for(std::size_t i=0;i<frame.visible.size();++i) {
        const auto& item=frame.visible[i];
        if(!item.expanded || recovered_widget_visible_count(
                item.title,static_cast<int>(frame.aim_preview_mode),
                static_cast<int>(frame.target_preview_mode))<=5)
            continue;
        const auto card=menu_module_card_rect(
            frame,panel->logical_width(),i);
        if(card.y+card.height>
           panel->logical_height()-menu_panel_padding)
            continue;
        const float settingsY=card.y+menu_row_body_height(frame);
        if(logicalX>=card.x && logicalX<card.x+card.width &&
           logicalY>=settingsY && logicalY<card.y+card.height)
            return item.catalog_index;
    }
    return std::nullopt;
}
} // namespace slinkywinky
