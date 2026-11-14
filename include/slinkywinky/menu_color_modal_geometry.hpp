#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>

namespace slinkywinky {
// R200 pure, viewport-local first-party color dialog geometry. Coordinates
// are read identically by GL11 paint and synthetic native mouse hit testing.
// Approximate visuals until image comparison proves exact native dimensions.
struct MenuColorModalRect {
    float x=0.0f,y=0.0f,width=0.0f,height=0.0f;
    bool contains(float px,float py) const noexcept {
        return px>=x && px<x+width && py>=y && py<y+height;
    }
};
// R204 renderer-neutral rounded-gradient clip. The candidate NanoVG
// popup rounds the *outer* ends of its six-sector hue bar and both
// HSL gradient bars by 5px. This span describes the vertical interior
// at a logical x, without touching the full rectangular mouse hitbox.
struct MenuColorModalSpan {
    float top=0.0f,bottom=0.0f;
};
inline std::optional<MenuColorModalSpan> menu_color_bar_span(
        const MenuColorModalRect& bar,float x,
        float source_radius=5.0f) noexcept {
    if(!std::isfinite(bar.x) || !std::isfinite(bar.y) ||
       !std::isfinite(bar.width) || !std::isfinite(bar.height) ||
       !std::isfinite(x) || !std::isfinite(source_radius) ||
       bar.width<=0.0f || bar.height<=0.0f ||
       source_radius<0.0f || x<bar.x || x>bar.x+bar.width)
        return std::nullopt;
    const float radius=std::min(
        source_radius,0.5f*std::min(bar.width,bar.height));
    if(radius==0.0f)
        return MenuColorModalSpan{bar.y,bar.y+bar.height};
    const float edge=std::min(x-bar.x,bar.x+bar.width-x);
    if(edge>=radius)
        return MenuColorModalSpan{bar.y,bar.y+bar.height};
    const float d=radius-edge;
    const float quarter=std::sqrt(
        std::max(0.0f,radius*radius-d*d));
    return MenuColorModalSpan{
        bar.y+radius-quarter,
        bar.y+bar.height-radius+quarter};
}
struct MenuColorModalGeometry {
    MenuColorModalRect outer{},preview{},hex{};
    std::array<MenuColorModalRect,3> hsl{};
    std::array<MenuColorModalRect,8> recent{};
    MenuColorModalRect close{}; // R202: source-pinned popup title cross
};
inline std::optional<MenuColorModalGeometry> menu_color_modal_geometry(
        float logical_width,float logical_height) noexcept {
    if(!std::isfinite(logical_width) ||
       !std::isfinite(logical_height) ||
       logical_width<310.0f || logical_height<275.0f)
        return std::nullopt;
    MenuColorModalGeometry out;
    const float width=std::min(360.0f,logical_width-28.0f);
    // R202: remove R200's unused 18–32px footer reservation.
    // Still approximate; authenticated full-modal dimensions unknown.
    const float height=std::min(294.0f,logical_height-24.0f);
    const float yScale=height/294.0f;
    const float left=(logical_width-width)*0.5f;
    const float top=(logical_height-height)*0.5f;
    const auto r=[left,top,yScale](float x,float y,float w,float h){
        return MenuColorModalRect{left+x,top+y*yScale,w,h*yScale};
    };
    out.outer={left,top,width,height};
    out.preview=r(18.0f,43.0f,30.0f,24.0f);
    out.hex=r(64.0f,43.0f,width-82.0f,24.0f);
    for(unsigned i=0;i<3u;++i)
        out.hsl[i]=r(18.0f,101.0f+53.0f*i,
                     width-36.0f,15.0f); // R203 source font_size_15
    // R205: native candidate gui_popups.cpp uses begin_content(...,12)
    // and `sz=(row_width-56)*0.125`, `step=sz+8` for eight swatches.
    // Keep this one geometry for both GL11 paint and mouse hit testing.
    const float recentContentWidth=width-24.0f;
    const float recentWidth=(recentContentWidth-56.0f)*0.125f;
    for(unsigned i=0;i<8u;++i)
        out.recent[i]=r(12.0f+static_cast<float>(i)*
                        (recentWidth+8.0f),255.0f,
                        recentWidth,25.0f); // source row_height_25
    // Native candidate gui_popups.cpp has one 20px title-row cross,
    // not the synthetic Apply/Cancel footer added in R200.
    out.close=r(width-38.0f,12.0f,20.0f,20.0f);
    return out;
}
} // namespace slinkywinky
