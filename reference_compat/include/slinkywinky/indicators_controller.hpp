#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
namespace slinkywinky::reference_compat {
class IndicatorsController {
public:
    struct Pos{double x,y,z;};
    struct Settings{bool arrows,fireballs,pearls,only_approaching;};
    struct Sample{int kind;bool grounded;Pos current,previous;};
    struct Entry{int kind;Pos current,previous;std::string label;std::uint32_t color;};
private:
    std::vector<Entry> entries_;
    static bool finite(Pos p)noexcept{
        return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
    }
    static double squared(Pos a,Pos b)noexcept{
        double x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;
        return x*x+y*y+z*z;
    }
public:
    void collect(const Settings& s,bool world,bool font,Pos player,const std::vector<Sample>& samples){
        entries_.clear();
        if(!world||!font)return;
        if(!finite(player))throw std::invalid_argument("invalid observer");
        for(const auto& p:samples){
            bool wanted=p.kind==10?(s.arrows&&!p.grounded):
                        p.kind==12?s.fireballs:p.kind==14?s.pearls:false;
            if(!wanted||!finite(p.current)||!finite(p.previous))continue;
            double d=squared(p.current,player),old=squared(p.previous,player);
            if(!std::isfinite(d)||!std::isfinite(old)||(s.only_approaching&&d>old))continue;
            double dist=std::sqrt(d);
            char label[10]{};
            std::snprintf(label,sizeof(label),"%.0fm",dist);
            entries_.push_back({p.kind,p.current,p.previous,label,
                dist>30?0xffffffffu:dist>15?0xffffff00u:0xffff0000u});
            if(entries_.size()==10)break;
        }
    }
    const std::vector<Entry>& entries()const noexcept{return entries_;}
    void disable(bool own_descriptor,bool on)noexcept{
        if(own_descriptor&&!on)entries_.clear();
    }
};
}
