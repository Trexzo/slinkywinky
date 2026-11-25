#pragma once
#include <array>
#include <cmath>
#include <stdexcept>
namespace slinkywinky::reference_compat {
// Offline lifecycle intents only. No game input consumption or OS calls.
class GuiController {
public:
    enum class Request { none, open, close };
    struct InputDecision { bool candidate_consumed; Request request; };
    struct Transition {
        bool center_cursor=false, release_mouse=false, restore_mouse=false;
        bool reset_popup=false, reset_drag=false, start_animation=false, save=false;
    };
    struct Appearance {float scale;bool compact,wide;std::array<float,3> accent;};
private:
    bool open_=false, shutting_down_=false;
public:
    InputDecision input(int type,int code,bool down,bool shift,bool handled,
                        int toggle_key,bool allow_input)const noexcept{
        if(shutting_down_)return {false,Request::none};
        if(open_&&handled)return {true,Request::none};
        if(type==2&&down&&code!=0){
            if(!shift&&code==toggle_key)return {true,open_?Request::close:Request::open};
            if(code==0x57&&open_)return {true,Request::close};
        }
        if(!open_)return {false,Request::none};
        return {type!=2||!allow_input||static_cast<unsigned>(code)<=5,Request::none};
    }
    Transition toggle(bool own,bool on,bool grabbed,bool in_game)noexcept{
        Transition out;
        if(!own||on==open_||(on&&shutting_down_))return out;
        open_=on;
        if(on){out.center_cursor=grabbed;out.release_mouse=grabbed;out.start_animation=true;}
        else{
            out.restore_mouse=in_game;out.reset_popup=true;out.reset_drag=true;
            out.save=!shutting_down_;
        }
        return out;
    }
    bool release_mouse_on_tick(bool grabbed)const noexcept{return open_&&grabbed;}
    unsigned hud_flags(unsigned flags)const noexcept{return flags|(open_?3u:0u);}
    void shutdown()noexcept{shutting_down_=true;}
    bool open()const noexcept{return open_;}
    static Appearance appearance(float scale,bool compact,bool wide,std::array<float,3> rgb){
        if(!std::isfinite(scale)||scale<=0)throw std::invalid_argument("GUI scale");
        for(float v:rgb)if(!std::isfinite(v)||v<0||v>1)throw std::invalid_argument("GUI RGB");
        return {scale,compact,wide,rgb};
    }
};
}
