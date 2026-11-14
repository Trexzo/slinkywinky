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
    const float recentWidth=(width-36.0f-7.0f*6.0f)/8.0f;
    for(unsigned i=0;i<8u;++i)
        out.recent[i]=r(18.0f+i*(recentWidth+6.0f),
                        255.0f,recentWidth,25.0f); // R203 source row_height_25
    // Native candidate gui_popups.cpp has one 20px title-row cross,
    // not the synthetic Apply/Cancel footer added in R200.
    out.close=r(width-38.0f,12.0f,20.0f,20.0f);
    return out;
}
} // namespace slinkywinky
