#include "slinkywinky/menu_render.hpp"
#include <limits>
#include <utility>

namespace slinkywinky {
bool MenuRenderSession::valid_viewport(unsigned width,unsigned height) noexcept {
    // Reject minimized, implausibly large and zero-sized host surfaces.
    return width>=320u && width<=8192u && height>=240u && height<=8192u;
}
bool MenuRenderSession::owner() const noexcept {
    return thread_==std::this_thread::get_id();
}
bool MenuRenderSession::attach(std::uint64_t epoch,unsigned width,
                               unsigned height) noexcept {
    if(active_ || epoch==0 || !valid_viewport(width,height))return false;
    thread_=std::this_thread::get_id();
    epoch_=epoch;
    width_=width;
    height_=height;
    sequence_=0;
    active_=true;
    return true;
}
bool MenuRenderSession::resize(unsigned width,unsigned height) noexcept {
    if(!active_ || !owner() || !valid_viewport(width,height))return false;
    width_=width;
    height_=height;
    return true;
}
std::optional<MenuRenderFrame> MenuRenderSession::next(const MenuState& menu) {
    if(!active_ || !owner() || !valid_viewport(width_,height_) ||
       sequence_==std::numeric_limits<std::uint64_t>::max())
        return std::nullopt;
    // Only the first-party model is copied. No raw pointers into a native
    // game, render context, reflection cache, or Java object are retained.
    MenuRenderFrame frame;
    frame.epoch=epoch_;
    frame.sequence=sequence_+1;
    frame.viewport_width=width_;
    frame.viewport_height=height_;
    frame.tab=menu.tab();
    frame.appearance=menu.appearance();
    frame.visible=menu.page();
    if(frame.visible.size()>8)return std::nullopt;
    frame.is_placeholder=frame.tab==MenuTab::Profiles;
    if(frame.tab==MenuTab::Count || frame.tab==MenuTab::Unload)
        return std::nullopt;
    sequence_++;
    return frame;
}
bool MenuRenderSession::detach() noexcept {
    if(!active_)return false;
    if(!owner())return false;
    active_=false;
    epoch_=0;
    sequence_=0;
    width_=height_=0;
    thread_=std::thread::id{};
    return true;
}
} // namespace slinkywinky
