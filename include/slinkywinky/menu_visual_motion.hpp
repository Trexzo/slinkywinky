#pragma once
#include "slinkywinky/menu.hpp"
#include <algorithm>
#include <cmath>

namespace slinkywinky {

// R149: value-only translation of the user's *candidate HTML* animation.
// Visual state is NOT a source of menu clicks, navigation or game input.
struct MenuVisualMotion {
    float tab_position=0.0f; // source: anim += (tab-anim)*min(1,dt/.05)
    float page_offset_y=0.0f; // -0.1 * logical-height * (tab-anim)
    float page_opacity=1.0f;  // max(0,1-2*abs(tab-anim))
    float opening_offset_y=0.0f; // source: -50px to 0px
    float opening_opacity=1.0f;  // source: 250ms ease-out opacity
    bool settled=true;
};
inline bool menu_visual_motion_valid(
        const MenuVisualMotion& visual,MenuTab tab,
        float logical_height) noexcept {
    const unsigned selected=static_cast<unsigned>(tab);
    if(selected>=6u || !std::isfinite(logical_height) ||
       logical_height<120.0f ||
       !std::isfinite(visual.tab_position) ||
       visual.tab_position<0.0f || visual.tab_position>5.0f ||
       !std::isfinite(visual.page_offset_y) ||
       !std::isfinite(visual.page_opacity) ||
       visual.page_opacity<0.0f || visual.page_opacity>1.0f ||
       !std::isfinite(visual.opening_offset_y) ||
       visual.opening_offset_y< -50.0f ||
       visual.opening_offset_y>0.0f ||
       !std::isfinite(visual.opening_opacity) ||
       visual.opening_opacity<0.0f ||
       visual.opening_opacity>1.0f)
        return false;
    const float diff=static_cast<float>(selected)-visual.tab_position;
    return std::fabs(visual.page_offset_y+
                     0.1f*logical_height*diff)<0.02f &&
           std::fabs(visual.page_opacity-
                     std::clamp(1.0f-2.0f*std::fabs(diff),
                                0.0f,1.0f))<0.02f;
}
class MenuVisualMotionClock final {
public:
    // A separate native preview owns the clock. No wall time, scheduler,
    // OS hooks, timers or mutable globals are used in this model.
    MenuVisualMotion advance(MenuTab selected,float elapsed_seconds,
                             float logical_height) noexcept {
        const auto idx=static_cast<unsigned>(selected);
        if(idx>=6u || !std::isfinite(elapsed_seconds) ||
           elapsed_seconds<0.0f ||
           !std::isfinite(logical_height) || logical_height<120.0f)
            return current_;
        if(!initialized_) {
            initialized_=true;
            current_.tab_position=static_cast<float>(idx);
            opening_elapsed_=0.0f;
        }
        const float dt=std::clamp(elapsed_seconds,0.0f,0.1f);
        const float target=static_cast<float>(idx);
        current_.tab_position+=(target-current_.tab_position)*
            std::min(1.0f,dt/0.05f);
        if(std::fabs(target-current_.tab_position)<0.001f)
            current_.tab_position=target;
        opening_elapsed_=std::min(0.25f,opening_elapsed_+dt);
        const float fraction=opening_elapsed_/0.25f;
        const float ease=1.0f-(1.0f-fraction)*
                               (1.0f-fraction)*(1.0f-fraction);
        current_.opening_offset_y=-50.0f*(1.0f-ease);
        current_.opening_opacity=ease;
        const float diff=target-current_.tab_position;
        current_.page_offset_y=-0.1f*logical_height*diff;
        current_.page_opacity=std::clamp(
            1.0f-2.0f*std::fabs(diff),0.0f,1.0f);
        current_.settled=(current_.tab_position==target &&
                          opening_elapsed_>=0.25f);
        return current_;
    }
    bool initialized() const noexcept {return initialized_;}
    bool settled() const noexcept {
        return initialized_ && current_.settled;
    }
    const MenuVisualMotion& value() const noexcept {return current_;}
    void reset() noexcept {
        initialized_=false;
        opening_elapsed_=0.0f;
        current_=MenuVisualMotion{};
    }
private:
    bool initialized_=false;
    float opening_elapsed_=0.0f;
    MenuVisualMotion current_{};
};
} // namespace slinkywinky
