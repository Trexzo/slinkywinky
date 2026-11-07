#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_profile_preview.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>

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
    if(!panel || !menu_profile_preview_valid(
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
    // R113: exactly the same shared geometry as the Slinky-style renderer.
    // "Unload" is displayed in red, but is NEVER an admitted mouse action.
    if(y>=menu_nav_y && y<menu_nav_y+menu_nav_height &&
       x>=menu_nav_x && x<w-menu_nav_x) {
        const unsigned tab=static_cast<unsigned>(
            (x-menu_nav_x)/menu_tab_width(w));
        if(tab<6u)return MenuHit{
            MenuHitKind::Tab,static_cast<MenuTab>(tab),0};
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
           y<rect.y || y>=rect.y+body)continue;
        const std::size_t index=frame.visible[i].catalog_index;
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
} // namespace slinkywinky
