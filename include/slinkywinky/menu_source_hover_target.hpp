#pragma once
// R179: first-party source/geometry-only hit target; NO OS input or JNI.
// Shared by the actual retained native WGL owner and disposable JNI tests.
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include "slinkywinky/menu.hpp"
#include <cmath>
#include <cstddef>
#include <optional>
#include <string_view>

namespace slinkywinky {
struct MenuSourceHoverTarget final {
    int left=0,top=0,right=0,bottom=0,center_x=0,center_y=0;
    std::size_t catalog_index=0,widget_index=0;
    bool contains(int x,int y) const noexcept {
        return x>=left && x<right && y>=top && y<bottom;
    }
};

// This is the exact first-party recovered Auto Block -> Main -> Range
// slider. It is visible ONLY if the source page, geometry and same native
// hit-test all agree. Coordinates are CLIENT pixels, not OS screen space.
inline std::optional<MenuSourceHoverTarget>
menu_original_auto_block_range_target(
        const MenuRenderFrame& frame,unsigned width,unsigned height) noexcept {
    if(frame.viewport_width!=width || frame.viewport_height!=height ||
       width<320 || width>8192 || height<240 || height>8192 ||
       frame.tab!=MenuTab::Combat || frame.is_placeholder)
        return std::nullopt;
    const auto& modules=official_module_catalog();
    const auto& controls=recovered_widget_catalog();
    std::size_t module=modules.size(),widget=controls.size();
    for(std::size_t i=0;i<modules.size();i++)
        if(std::string_view(modules[i].name)=="Auto Block" &&
           modules[i].tab==MenuTab::Combat) {
            if(module!=modules.size())return std::nullopt;
            module=i;
        }
    for(std::size_t i=0;i<controls.size();i++) {
        const auto& control=controls[i];
        if(std::string_view(control.module)=="Auto Block" &&
           std::string_view(control.group)=="Main" &&
           std::string_view(control.label)=="Range" &&
           control.kind==RecoveredWidgetKind::Slider) {
            if(widget!=controls.size())return std::nullopt;
            widget=i;
        }
    }
    if(module==modules.size() || widget==controls.size())return std::nullopt;
    const auto panel=menu_panel_geometry(
        width,height,frame.appearance.wide,MenuPanelMode::CompactOverlay);
    if(!panel || !std::isfinite(panel->scale) || panel->scale<=0.0f)
        return std::nullopt;
    const auto track=menu_numeric_track_geometry(
        frame,panel->logical_width(),panel->logical_height(),module,widget);
    if(!track || track->width<6.0f || track->height<8.0f ||
       !std::isfinite(track->x) || !std::isfinite(track->y))
        return std::nullopt;
    // A strictly contained physical-pixel subrectangle avoids possible
    // edge rounding and clipped hit-tests after GUI scale changes.
    const int left=static_cast<int>(
        std::ceil(panel->left+track->x*panel->scale));
    const int top=static_cast<int>(
        std::ceil(panel->top+track->y*panel->scale));
    const int right=static_cast<int>(
        std::floor(panel->left+(track->x+track->width)*panel->scale));
    const int bottom=static_cast<int>(
        std::floor(panel->top+(track->y+track->height)*panel->scale));
    const int cx=left+(right-left)/2,cy=top+(bottom-top)/2;
    if(left<0 || top<0 || right>static_cast<int>(width) ||
       bottom>static_cast<int>(height) ||
       right-left<4 || bottom-top<4)return std::nullopt;
    // A center-only check can admit a rectangle whose edges are owned
    // by another hit target after physical-pixel rounding. The source
    // rectangle is evidence only when its center and all four inclusive
    // interior corners resolve to the SAME recovered slider via the real
    // renderer hit tester. This remains read-only (no input ownership).
    const auto matches_source=[&](int x,int y) noexcept {
        const auto hit=hit_test_menu(
            frame,MenuPanelMode::CompactOverlay,x,y);
        return hit && hit->kind==MenuHitKind::RecoveredSliderPreview &&
               hit->catalog_index==module && hit->control_index==widget;
    };
    if(!matches_source(cx,cy) ||
       !matches_source(left,top) ||
       !matches_source(right-1,top) ||
       !matches_source(left,bottom-1) ||
       !matches_source(right-1,bottom-1))
        return std::nullopt;
    return MenuSourceHoverTarget{
        left,top,right,bottom,cx,cy,module,widget};
}
} // namespace slinkywinky
