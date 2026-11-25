#pragma once
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>
namespace slinkywinky::reference_compat {
// Observed event facts -> typed diagnostic records, no packet interception.
class DebugController {
public:
    struct Settings {bool hurt_time,item_use_state,clicks;};
    enum class Observed {other,use_item,release_item};
    enum class Kind {hurt,item,click};
    struct Line {
        Kind kind; unsigned stamp; unsigned first,second;
        char first_color,second_color,third_color;
    };
private:
    bool current_=false,last_=false;
    unsigned left_=0,right_=0,window_=0;
    static unsigned increment(unsigned v)noexcept{
        return v==std::numeric_limits<unsigned>::max()?v:v+1;
    }
public:
    void observe_packet(int type,Observed packet)noexcept{
        if(type!=0&&type!=3)return;
        if(packet==Observed::use_item)current_=true;
        else if(packet==Observed::release_item)current_=false;
    }
    void observe_click(const Settings& s,bool left)noexcept{
        if(!s.clicks)return;
        if(left)left_=increment(left_);else right_=increment(right_);
        window_=0;
    }
    std::vector<Line> tick(const Settings& s,bool in_game,unsigned ticks,
                           int hurt,bool sprint,bool item){
        std::vector<Line> out;
        if(!in_game)return out;
        if(s.hurt_time&&hurt>=7){
            constexpr char colors[]={'a','e','c','4'};
            out.push_back({Kind::hurt,ticks%1000,static_cast<unsigned>(hurt),0,
                hurt<=10?colors[hurt-7]:'f',sprint?'8':'c',item?'c':'8'});
        }
        if(s.item_use_state&&current_!=last_){
            last_=current_;
            out.push_back({Kind::item,ticks%1000,current_?1u:0u,0,current_?'a':'c',' ',' '});
        }
        return out;
    }
    std::vector<Line> late_tick(const Settings& s,bool in_game,unsigned ticks){
        std::vector<Line> out;
        if(!in_game)return out;
        if(s.clicks){
            if(window_<=9)out.push_back({Kind::click,ticks%1000,left_,right_,
                left_==0?'8':left_==1?'a':'2',right_==0?'8':right_==1?'b':'3',' '});
            left_=right_=0;
        }
        window_=increment(window_);
        return out;
    }
    void reset_session()noexcept{current_=last_=false;left_=right_=window_=0;}
};
}
