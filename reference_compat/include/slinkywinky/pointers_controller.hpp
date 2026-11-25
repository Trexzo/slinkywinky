#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>
namespace slinkywinky::reference_compat {
// Source scalar math for a caller-supplied presentation sample, not world collection.
class PointersController {
public:
    using RGB=std::array<float,3>;
    struct Settings {float near_distance,far_distance,radius,size;bool pulse;RGB near_color,far_color;};
    struct Style {float gradient,radius,scale;RGB color;};
    static Style style(const Settings& s,float previous_distance,float current_distance,float partial){
        for(float v:{s.near_distance,s.far_distance,s.radius,s.size,previous_distance,current_distance,partial})
            if(!std::isfinite(v))throw std::invalid_argument("invalid pointer scalar");
        for(float v:s.near_color)if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("invalid color");
        for(float v:s.far_color)if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("invalid color");
        if(partial<0||partial>1||previous_distance<0||current_distance<0||s.radius<0||s.size<0)
            throw std::invalid_argument("invalid pointer bounds");
        const float distance=previous_distance+(current_distance-previous_distance)*partial;
        float range=s.far_distance-s.near_distance;
        if(range==0)range=1e-5f;
        const float fraction=std::clamp((distance-s.near_distance)/range,0.f,1.f);
        Style out{fraction,s.radius*(s.pulse?fraction+1.f:1.f),s.size*10.f,{}};
        for(unsigned i=0;i<3;++i)out.color[i]=s.near_color[i]+(s.far_color[i]-s.near_color[i])*fraction;
        if(!std::isfinite(out.radius)||!std::isfinite(out.scale)||!std::isfinite(fraction))
            throw std::invalid_argument("pointer overflow");
        return out;
    }
    static float tilt(bool style,float pitch){
        if(!std::isfinite(pitch))throw std::invalid_argument("invalid pitch");
        return style?90.f-std::max(30.f,pitch):0.f;
    }
};
}
