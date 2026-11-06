#include "slinkywinky/menu_mouse_gesture.hpp"
#include <utility>

namespace slinkywinky {
namespace {
constexpr std::uint64_t max_press_ms=1500;
constexpr std::uint64_t repeat_reject_ms=375;
}

MenuMouseGesture::MenuMouseGesture() noexcept :
    owner_(std::this_thread::get_id()) {}

bool MenuMouseGesture::owner_thread() const noexcept {
    return owner_==std::this_thread::get_id();
}
bool MenuMouseGesture::pending() const noexcept {
    return owner_thread() && active_.has_value();
}
bool MenuMouseGesture::cancel() noexcept {
    if(!owner_thread())return false;
    active_.reset();
    return true;
}
bool MenuMouseGesture::same_hit(
    const MenuHit& a,const MenuHit& b) noexcept {
    return a.kind==b.kind && a.tab==b.tab &&
           a.catalog_index==b.catalog_index;
}
bool MenuMouseGesture::equivalent_frame(
    const MenuRenderFrame& frame,MenuPanelMode mode) const noexcept {
    if(!active_ || active_->mode!=mode)return false;
    const auto& original=active_->frame;
    if(frame.epoch!=original.epoch || frame.sequence<original.sequence ||
       frame.viewport_width!=original.viewport_width ||
       frame.viewport_height!=original.viewport_height ||
       frame.tab!=original.tab ||
       frame.is_placeholder!=original.is_placeholder ||
       frame.appearance.wide!=original.appearance.wide ||
       frame.appearance.compact!=original.appearance.compact ||
       frame.appearance.allow_input_while_open!=
           original.appearance.allow_input_while_open ||
       frame.appearance.accent_rgb!=original.appearance.accent_rgb ||
       frame.appearance.scale!=original.appearance.scale ||
       frame.visible.size()!=original.visible.size() ||
       frame.parameters.size()!=original.parameters.size())
        return false;
    for(std::size_t i=0;i<frame.visible.size();++i) {
        const auto& a=frame.visible[i];
        const auto& b=original.visible[i];
        if(a.catalog_index!=b.catalog_index || a.title!=b.title ||
           a.preview_enabled!=b.preview_enabled || a.expanded!=b.expanded)
            return false;
    }
    for(std::size_t i=0;i<frame.parameters.size();++i) {
        const auto& a=frame.parameters[i];
        const auto& b=original.parameters[i];
        if(a.catalog_index!=b.catalog_index || a.caption!=b.caption)
            return false;
    }
    return true;
}
bool MenuMouseGesture::press(
    const MenuRenderFrame& frame,MenuPanelMode mode,
    int x,int y,std::uint64_t monotonic_ms,bool foreground) {
    if(!owner_thread())return false;
    // A second down while the button is already held is never a click.
    if(active_) {active_.reset();return false;}
    if(!foreground)return false;
    const auto hit=hit_test_menu(frame,mode,x,y);
    if(!hit)return false;
    active_=Press{frame,*hit,mode,monotonic_ms};
    return true;
}
bool MenuMouseGesture::move(
    const MenuRenderFrame& frame,MenuPanelMode mode,
    int x,int y,bool foreground) noexcept {
    if(!owner_thread() || !active_)return false;
    if(!foreground || !equivalent_frame(frame,mode)) {
        active_.reset();
        return false;
    }
    const auto hit=hit_test_menu(frame,mode,x,y);
    if(!hit || !same_hit(*hit,active_->hit)) {
        // Dragging off the target is terminal; re-entering cannot revive it.
        active_.reset();
        return false;
    }
    return true;
}
std::optional<MenuHit> MenuMouseGesture::release(
    const MenuRenderFrame& frame,MenuPanelMode mode,
    int x,int y,std::uint64_t monotonic_ms,
    bool foreground) noexcept {
    if(!owner_thread() || !active_)return std::nullopt;
    const auto previous=active_->hit;
    const auto pressed_at=active_->timestamp;
    const bool valid=foreground && equivalent_frame(frame,mode) &&
        monotonic_ms>=pressed_at &&
        monotonic_ms-pressed_at<=max_press_ms;
    active_.reset(); // A release is consumed even if it fails validation.
    if(!valid)return std::nullopt;
    const auto released=hit_test_menu(frame,mode,x,y);
    if(!released || !same_hit(previous,*released))return std::nullopt;

    // Ignore the second click of a rapid double click on the same action.
    // Allow a different target (e.g. a different row) without delay.
    if(last_committed_ && same_hit(*released,*last_committed_) &&
       monotonic_ms>=last_committed_ms_ &&
       monotonic_ms-last_committed_ms_<repeat_reject_ms)
        return std::nullopt;
    last_committed_=*released;
    last_committed_ms_=monotonic_ms;
    return released;
}
} // namespace slinkywinky
