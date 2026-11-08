#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include <algorithm>
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
    return next(menu,profile,nullptr);
}
std::optional<MenuRenderFrame> MenuRenderSession::next(
    const MenuState& menu,const MenuProfileBridge* profile,
    const MenuNavigation* navigation) {
    std::lock_guard<std::mutex> lock(mutex_);
    if(!active_ || !owner() || !valid_viewport(width_,height_) ||
       (profile!=nullptr && !profile->owns(menu)) ||
       (navigation!=nullptr &&
        (profile==nullptr || !navigation->owns(menu,*profile))) ||
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
    frame.aim_preview_mode=menu.aim_preview_mode();
    frame.target_preview_mode=menu.target_preview_mode();
    frame.piercing_preview=menu.piercing_preview();
    frame.dropdown_preview=menu.dropdown_preview();
    frame.unload_confirmation_preview=menu.unload_confirmation_preview();
    frame.bind_previews=menu.bind_previews();
    frame.bind_capture_index=menu.bind_capture_index();
    frame.bind_hover_row=menu.bind_hover_row();
    frame.numeric_hover=menu.numeric_hover();
    frame.numeric_drag_preview=menu.numeric_drag_preview();
    frame.recovered_boolean_overrides=menu.recovered_boolean_overrides();
    // Source-matched native Boolean state wins after a profile load.
    if(profile) {
        for(std::size_t widget=0;
            widget<frame.recovered_boolean_overrides.size();++widget) {
            if(const auto mapped=profile->recovered_boolean_setting(widget))
                frame.recovered_boolean_overrides[widget]=*mapped;
        }
    }
    frame.recovered_choice_overrides=menu.recovered_choice_overrides();
    frame.recovered_multi_overrides=menu.recovered_multi_overrides();
    frame.recovered_color_overrides=menu.recovered_color_overrides();
    // Avoid a large dense per-frame std::optional<std::string> copy:
    // the source fixture has six Text callsites among 385 widgets.
    frame.recovered_text_overrides.reserve(6u);
    const auto& sourceTexts=menu.recovered_text_overrides();
    for(std::size_t i=0;i<sourceTexts.size();++i) {
        if(!sourceTexts[i])continue;
        if(frame.recovered_text_overrides.size()>=6u)
            return std::nullopt;
        frame.recovered_text_overrides.push_back(
            MenuRenderTextValue{i,*sourceTexts[i]});
    }
    frame.focused_text_widget=menu.focused_text_widget();
    frame.recovered_slider_overrides=menu.recovered_slider_overrides();
    // R155: native profile is authoritative over five supported source
    // controls even after a profile load or an independent primary edit.
    if(profile) {
        for(std::size_t widget=0;
            widget<frame.recovered_slider_overrides.size();++widget) {
            if(const auto mapped=profile->recovered_slider_setting(widget))
                frame.recovered_slider_overrides[widget]=*mapped;
        }
    }
    frame.recovered_range_overrides=menu.recovered_range_overrides();
    frame.piercing_selected=menu.piercing_selected();
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
            frame.parameters.push_back({item.catalog_index,value,
                parameter->value,parameter->minimum,parameter->maximum,true});
        }
    }
    frame.is_placeholder=frame.tab==MenuTab::Profiles;
    if(frame.is_placeholder) {
        for(std::size_t i=0;i<menu.local_profiles().size();++i) {
            const auto& p=menu.local_profiles()[i];
            frame.profile_cards.push_back({
                p.name,p.description,p.bind,i==0u
            });
        }
        frame.profile_name_draft=menu.profile_name_draft();
        frame.profile_name_focused=menu.profile_name_focused();
        frame.profile_name_taken=menu.profile_name_taken();
        frame.profile_action_preview=menu.profile_action_preview();
        frame.profile_icon_hover=menu.profile_icon_hover();
        frame.profile_bind_capture_index=menu.profile_bind_capture_index();
    }
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
