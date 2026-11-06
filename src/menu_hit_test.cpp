#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_profile.hpp"
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
        std::min<unsigned>(compact?480u:(wide?844u:676u),viewport_width-24u);
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
    if(!panel || !frame.epoch || !frame.sequence ||
       frame.visible.size()>8 ||
       frame.tab==MenuTab::Count || frame.tab==MenuTab::Unload ||
       (frame.is_placeholder!=(frame.tab==MenuTab::Profiles)))
        return std::nullopt;
    const auto& catalog=official_module_catalog();
    for(const auto& row:frame.visible) {
        if(row.catalog_index>=catalog.size() ||
           catalog[row.catalog_index].tab!=frame.tab ||
           row.title!=catalog[row.catalog_index].name)
            return std::nullopt;
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
    // Tabs exactly match the 7 equal cells drawn at x=18..w-18,
    // y=78..120. "Unload" has a painted label but no admitted hit.
    if(y>=78.0f && y<120.0f && x>=18.0f && x<w-18.0f) {
        const float cell=(w-36.0f)/7.0f;
        const unsigned tab=static_cast<unsigned>((x-18.0f)/cell);
        if(tab<6u)return MenuHit{
            MenuHitKind::Tab,static_cast<MenuTab>(tab),0};
        return std::nullopt;
    }
    if(frame.tab>=MenuTab::Profiles)return std::nullopt;
    if(x<18.0f || x>=w-18.0f)return std::nullopt;
    for(std::size_t i=0;i<frame.visible.size();++i) {
        const float top=153.0f+static_cast<float>(i)*49.0f;
        // A clipped partial row is deliberately not an actionable target.
        if(top+44.0f>panel->logical_height())continue;
        if(y<top || y>=top+44.0f)continue; // 5px row gaps inert.
        const std::size_t index=frame.visible[i].catalog_index;
        const bool implemented=
            index<catalog.size() && module_has_profile_setting(index);
        if(implemented &&
           x>=w-76.0f && x<w-31.0f &&
           y>=top+12.0f && y<top+34.0f)
            return MenuHit{MenuHitKind::Toggle,frame.tab,index};
        if(implemented &&
           x>=w-123.0f && x<w-105.0f &&
           y>=top+13.0f && y<top+31.0f)
            return MenuHit{MenuHitKind::Expand,frame.tab,index};
        return MenuHit{MenuHitKind::RowBody,frame.tab,index};
    }
    return std::nullopt;
}
} // namespace slinkywinky
