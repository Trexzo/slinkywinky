#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
namespace slinkywinky {
// R150: standalone-preview paint state; never dispatches game input.
struct MenuControlMotion {
    std::optional<std::size_t> hovered_boolean{};
    float switch_hover_opacity=0.0f;
    float popup_opacity=1.0f;
};
inline bool menu_control_motion_valid(const MenuControlMotion& m) noexcept {
    return std::isfinite(m.switch_hover_opacity) &&
        std::isfinite(m.popup_opacity) &&
        m.switch_hover_opacity>=0.0f && m.switch_hover_opacity<=1.0f &&
        m.popup_opacity>=0.0f && m.popup_opacity<=1.0f &&
        (!m.hovered_boolean || *m.hovered_boolean<385u) &&
        (m.hovered_boolean || m.switch_hover_opacity==0.0f);
}
inline float menu_control_center_text_y(
        float top,float height,float font_size) noexcept {
    if(!std::isfinite(top) || !std::isfinite(height) ||
       !std::isfinite(font_size) || height<0.0f || font_size<=0.0f)
        return top;
    return top+std::max(0.0f,(height-font_size)*0.5f);
}
// Approximate hover color pending comparison to actual original screenshots.
inline std::uint32_t menu_control_hover_tint(
        std::uint32_t color,float alpha) noexcept {
    if(!std::isfinite(alpha))return color;
    const float t=0.13f*std::clamp(alpha,0.0f,1.0f);
    std::uint32_t out=0;
    for(const unsigned shift:{16u,8u,0u}) {
        const auto c=(color>>shift)&255u;
        out|=static_cast<std::uint32_t>(
            std::lround(static_cast<float>(c)+(255.0f-c)*t))<<shift;
    }
    return out;
}
class MenuControlMotionClock final {
public:
    bool retarget(bool popup_open,
                  std::optional<std::size_t> hovered_boolean) noexcept {
        if(hovered_boolean && *hovered_boolean>=385u)
            hovered_boolean.reset();
        if(popup_open)hovered_boolean.reset();
        const bool changed=popup_open!=popup_open_ ||
                           hovered_boolean!=hover_target_;
        if(popup_open!=popup_open_) {
            popup_open_=popup_open;
            popup_opacity_=popup_open?0.0f:1.0f;
        }
        if(hovered_boolean!=hover_target_) {
            hover_target_=hovered_boolean;
            if(hover_target_) {
                displayed_hover_=hover_target_;
                hover_opacity_=0.0f;
            }
        }
        if(!hover_target_ && hover_opacity_==0.0f) displayed_hover_.reset();
        return changed;
    }
    bool advance(float seconds) noexcept {
        if(!std::isfinite(seconds) || seconds<=0.0f)return false;
        const float dt=std::clamp(seconds,0.0f,0.1f);
        const float prevPopup=popup_opacity_,prevHover=hover_opacity_;
        if(popup_open_) {
            popup_opacity_+=(1.0f-popup_opacity_)*
                             std::min(1.0f,dt/0.12f);
            if(popup_opacity_>0.999f)popup_opacity_=1.0f;
        }
        const float desired=hover_target_?1.0f:0.0f;
        hover_opacity_+=(desired-hover_opacity_)*
                        std::min(1.0f,dt/0.10f);
        if(std::fabs(desired-hover_opacity_)<0.001f)
            hover_opacity_=desired;
        if(!hover_target_ && hover_opacity_==0.0f)
            displayed_hover_.reset();
        return prevPopup!=popup_opacity_ || prevHover!=hover_opacity_;
    }
    bool settled() const noexcept {
        return (!popup_open_ || popup_opacity_==1.0f) &&
               (hover_target_?hover_opacity_==1.0f:
                              hover_opacity_==0.0f);
    }
    std::optional<std::size_t> hover_target() const noexcept {
        return hover_target_;
    }
    MenuControlMotion value() const noexcept {
        return MenuControlMotion{displayed_hover_,hover_opacity_,
                                 popup_opacity_};
    }
    void reset() noexcept {*this=MenuControlMotionClock{};}
private:
    bool popup_open_=false;
    float popup_opacity_=1.0f;
    std::optional<std::size_t> hover_target_{};
    std::optional<std::size_t> displayed_hover_{};
    float hover_opacity_=0.0f;
};
}
