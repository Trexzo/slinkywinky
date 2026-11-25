#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <vector>
namespace slinkywinky::reference_compat {
// Bounded free-flight diagnostic model; no world sampling, collision or aim control.
class TrajectoriesController {
public:
    struct Settings {bool bow,uncharged_bow,pearls,throwables;};
    struct Launch {float power,next_power,gravity;};
    struct Point {double x,y,z;};
    static float bow_power(int ticks){
        if(ticks<0)return 0;
        const float t=static_cast<float>(ticks)/20.f;
        const float v=(t*t+(t+t))/3.f;
        if(static_cast<double>(v)<.1)return 0;
        const double p=static_cast<double>(std::min(1.f,v))*1.5;
        return static_cast<float>(p+p);
    }
    static std::optional<Launch> select(const Settings& s,bool in_game,bool silent_rotation,
                    bool third_person,int item,bool using_item,int remaining){
        if(!in_game||silent_rotation||third_person)return {};
        if(item==0x170&&s.pearls)return Launch{1.5f,1.5f,.03f};
        if((item==0x14c||item==0x158)&&s.throwables)return Launch{1.5f,1.5f,.03f};
        if(item!=0x105||!s.bow)return {};
        if(!using_item)return s.uncharged_bow?std::optional<Launch>{Launch{3,3,.05f}}:std::nullopt;
        if(remaining<0||remaining>72000)return {};
        const float p=bow_power(72000-remaining);
        if(p==0)return {};
        return Launch{bow_power(71999-remaining),p,.05f};
    }
    static std::vector<Point> free_flight(Point start,Point velocity,float gravity,unsigned ticks){
        if(ticks>100||!std::isfinite(gravity)||gravity<0)throw std::invalid_argument("trajectory bound");
        auto finite=[](Point p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);};
        if(!finite(start)||!finite(velocity))throw std::invalid_argument("trajectory input");
        std::vector<Point> result;result.reserve(ticks);
        for(unsigned i=0;i<ticks;++i){
            start={start.x+velocity.x,start.y+velocity.y,start.z+velocity.z};
            if(!finite(start))throw std::invalid_argument("trajectory overflow");
            result.push_back(start);
            velocity={velocity.x*.99,velocity.y*.99-gravity,velocity.z*.99};
        }
        return result;
    }
};
}
