#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_profile.hpp"
#include <iomanip>
#include <limits>
#include <sstream>
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
    std::lock_guard<std::mutex> lock(mutex_);
    if(active_ || epoch==0 || epoch<=last_epoch_ ||
       !valid_viewport(width,height))return false;
    thread_=std::this_thread::get_id();
    epoch_=epoch;
    last_epoch_=epoch;
    width_=width;
    height_=height;
    sequence_=0;
    active_=true;
    return true;
}
bool MenuRenderSession::resize(unsigned width,unsigned height) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if(!active_ || !owner() || !valid_viewport(width,height))return false;
    width_=width;
    height_=height;
    return true;
}
std::optional<MenuRenderFrame> MenuRenderSession::next(
    const MenuState& menu,const MenuProfileBridge* profile) {
    std::lock_guard<std::mutex> lock(mutex_);
    if(!active_ || !owner() || !valid_viewport(width_,height_) ||
       (profile!=nullptr && !profile->owns(menu)) ||
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
    if(profile) {
        for(const auto& item:frame.visible) {
            if(!item.expanded)continue;
            const auto parameter=profile->primary(item.catalog_index);
            if(!parameter)continue;
            // Captions are made only from the first-party typed configuration.
            // Rendering later never needs access to mutable profile state.
            std::ostringstream caption;
            caption<<parameter->name<<": "<<std::fixed
                   <<std::setprecision(parameter->integral?0:2)
                   <<parameter->value;
            const std::string value=caption.str();
            if(value.empty() || value.size()>48)return std::nullopt;
            frame.parameters.push_back({item.catalog_index,value});
        }
    }
    frame.is_placeholder=frame.tab==MenuTab::Profiles;
    if(frame.tab==MenuTab::Count || frame.tab==MenuTab::Unload)
        return std::nullopt;
    sequence_++;
    return frame;
}
bool MenuRenderSession::detach() noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    if(!active_ || !owner())return false;
    active_=false;
    epoch_=0;
    sequence_=0;
    width_=height_=0;
    thread_=std::thread::id{};
    return true;
}
bool MenuRenderSession::attached() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_;
}
std::uint64_t MenuRenderSession::sequence() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return sequence_;
}
} // namespace slinkywinky
