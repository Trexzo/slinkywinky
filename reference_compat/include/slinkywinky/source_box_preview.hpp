#pragma once
// R333: source-shaped 0.6 x 2.0 remote position boxes as render proposals.
// This is GL-free, snapshot-only geometry; it does not render into Minecraft.
#include "slinkywinky/timer_range_source_prediction.hpp"
#include <array>
#include <cmath>
#include <stdexcept>

namespace slinkywinky::reference_compat {
struct SourceBoxObservation {
    SourceVec3 previous{},current{},camera{};
    float partial_ticks=0;
    float yaw_previous=0,yaw_current=0;
};
struct SourceBoxStyle {
    std::array<float,3> color{{1,1,1}};
    float line_width=1;
    bool filled=false,rotate=false;
};
struct SourceBoxProposal {
    bool draw=false;
    SourceVec3 translate{},scale{0.3,1.0,0.3};
    bool rotate=false,filled=false;
    float rotation_y_degrees=0,line_width=1,fill_alpha=0.4f;
    std::array<float,3> color{{1,1,1}};
};
inline SourceBoxProposal source_box_preview(
    bool is_visible,const SourceBoxObservation& s,const SourceBoxStyle& style,
    bool supports_rotation){
    if(!source_finite(s.previous)||!source_finite(s.current)||
       !source_finite(s.camera)||!std::isfinite(s.partial_ticks)||
       !std::isfinite(s.yaw_previous)||!std::isfinite(s.yaw_current)||
       !std::isfinite(style.line_width)||style.line_width<=0||
       style.line_width>64||s.partial_ticks<0||s.partial_ticks>1)
        throw std::invalid_argument("invalid source box frame");
    for(float c:style.color)
        if(!std::isfinite(c)||c<0||c>1)
            throw std::invalid_argument("invalid source box color");
    SourceBoxProposal result{};
    if(!is_visible)return result;
    const double t=s.partial_ticks;
    const SourceVec3 at{
        s.previous.x+(s.current.x-s.previous.x)*t,
        s.previous.y+(s.current.y-s.previous.y)*t,
        s.previous.z+(s.current.z-s.previous.z)*t
    };
    result.draw=true;
    result.translate=source_sub(at,s.camera);
    result.translate.y+=1.0;
    result.rotate=supports_rotation&&style.rotate;
    if(result.rotate)result.rotation_y_degrees=180.0f-
        (s.yaw_previous+(s.yaw_current-s.yaw_previous)*s.partial_ticks);
    result.filled=style.filled;
    result.line_width=style.line_width;
    result.color=style.color;
    return result;
}
} // namespace slinkywinky::reference_compat
