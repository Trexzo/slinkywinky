#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_retained_menu_preview.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
#include <GL/gl.h>
#include <cmath>
#include <limits>

namespace slinkywinky {
WglRetainedMenuPreview::WglRetainedMenuPreview()
    : profile_(menu_),navigation_(menu_,profile_),
      owner_(std::this_thread::get_id()) {}
bool WglRetainedMenuPreview::owner_thread() const noexcept {
    return owner_==std::this_thread::get_id();
}
bool WglRetainedMenuPreview::draw(
    HGLRC context,HDC dc,unsigned width,unsigned height) {
    if(!owner_thread() || !context || !dc ||
       width<320 || width>8192 || height<240 || height>8192 ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc ||
       drawn_frames_==std::numeric_limits<std::uint64_t>::max())
        return false;
    if(active_ && (context_!=context || dc_!=dc ||
                   width_!=width || height_!=height))return false;
    // First observed frame pins a single original OpenGL render epoch.
    // A changed GL context/viewport requires an explicit new session.
    WglMenuOverlayHost host;
    if(!host.open(context,dc,width,height))return false;
    bool rendered=false;
    try {
        rendered=host.draw(menu_,&profile_,MenuGL11Mode::CompactOverlay,
                           &navigation_);
    } catch(...) {rendered=false;}
    const bool closed=host.close(); // same admitted graphics owner
    if(!rendered || !closed)return false;
    if(!active_) {
        // Separate source-identified pointer snapshots from WGL paint
        // frames. Both are owner-thread local and pinned to one viewport.
        if(!pointer_frames_.attach(1u,width,height))return false;
        context_=context;
        dc_=dc;
        width_=width;
        height_=height;
        active_=true;
    }
    drawn_frames_++;
    return true;
}
bool WglRetainedMenuPreview::apply(MenuNavAction action) {
    if(!active_ || !owner_thread() ||
       accepted_actions_==std::numeric_limits<std::uint64_t>::max() ||
       refused_actions_==std::numeric_limits<std::uint64_t>::max())
        return false;
    // Navigation invalidates any partially held pointer transaction.
    pointer_gesture_.cancel();
    if(!navigation_.apply(action)) {
        refused_actions_++;
        return false;
    }
    accepted_actions_++;
    return true;
}
bool WglRetainedMenuPreview::pointer(
        RetainedPointerPhase phase,HGLRC context,HDC dc,
        unsigned width,unsigned height,int x,int y,
        std::uint64_t monotonic_ms,bool exclusive_dispatch_attested,
        bool foreground_verified) {
    if(!owner_thread() || !active_ || drawn_frames_==0)
        return false;
    if(!context || !dc || context!=context_ || dc!=dc_ ||
       width!=width_ || height!=height_ ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc) {
        pointer_gesture_.cancel(); // A changed render owner aborts a held press.
        return false;
    }
    if(phase==RetainedPointerPhase::Cancel)
        return pointer_gesture_.cancel();
    if(!exclusive_dispatch_attested || !foreground_verified ||
       !monotonic_ms || !pointer_frames_.attached()) {
        pointer_gesture_.cancel(); // Fail closed on lost ownership/focus.
        return false;
    }
    if(phase!=RetainedPointerPhase::Down &&
       phase!=RetainedPointerPhase::Move &&
       phase!=RetainedPointerPhase::Up) {
        pointer_gesture_.cancel();
        return false;
    }
    const auto frame=pointer_frames_.next(menu_,&profile_,&navigation_);
    if(!frame) {
        pointer_gesture_.cancel();
        return false;
    }
    constexpr auto mode=MenuPanelMode::CompactOverlay;
    if(phase==RetainedPointerPhase::Down)
        return pointer_gesture_.press(
            *frame,mode,x,y,monotonic_ms,true);
    if(phase==RetainedPointerPhase::Move)
        return pointer_gesture_.move(*frame,mode,x,y,true);
    const auto hit=pointer_gesture_.release(
        *frame,mode,x,y,monotonic_ms,true);
    if(!hit)return false;
    // The precise source hit was certified at mouse-down and mouse-up.
    // No arbitrary module values or gameplay input are dispatched.
    switch(hit->kind) {
        case MenuHitKind::Tab:
            return navigation_.select_tab(hit->tab);
        case MenuHitKind::RowBody:
            return navigation_.focus_visible_row(hit->catalog_index);
        case MenuHitKind::Expand:
            return navigation_.focus_visible_row(hit->catalog_index) &&
                   menu_.toggle_expanded(hit->catalog_index);
        case MenuHitKind::Toggle:
            return navigation_.focus_visible_row(hit->catalog_index) &&
                   profile_.toggle(hit->catalog_index);
        // R169: restore already source-validated dropdown hit dispatch.
        // This edits LOCAL preview selection only, never an in-game module.
        case MenuHitKind::AimModeControl:
            return navigation_.focus_visible_row(hit->catalog_index) &&
                   menu_.open_preview_dropdown(
                       MenuDropdownKind::AimMode,hit->control_index);
        case MenuHitKind::TargetModeControl:
            return navigation_.focus_visible_row(hit->catalog_index) &&
                   menu_.open_preview_dropdown(
                       MenuDropdownKind::TargetMode,hit->control_index);
        case MenuHitKind::RecoveredChoiceControl:
            return navigation_.focus_visible_row(hit->catalog_index) &&
                   menu_.open_recovered_choice_preview(
                       hit->catalog_index,hit->control_index);
        case MenuHitKind::DropdownOption:
            if(hit->control_index>
               static_cast<std::size_t>(
                   std::numeric_limits<unsigned>::max()))return false;
            return menu_.select_preview_dropdown_option(
                static_cast<unsigned>(hit->control_index));
        case MenuHitKind::RecoveredBooleanPreview:
            return navigation_.focus_visible_row(hit->catalog_index) &&
                   profile_.toggle_recovered_boolean(
                       hit->catalog_index,hit->control_index);
        case MenuHitKind::RecoveredSliderPreview: {
            const auto panel=menu_panel_geometry(
                width,height,frame->appearance.wide,mode);
            if(!panel || panel->scale<=0.0f)return false;
            const auto track=menu_numeric_track_geometry(
                *frame,panel->logical_width(),panel->logical_height(),
                hit->catalog_index,hit->control_index);
            const auto& catalog=recovered_widget_catalog();
            if(!track || hit->control_index>=catalog.size())return false;
            const float position=
                (static_cast<float>(x)-panel->left)/panel->scale;
            const auto value=menu_slider_value_at_x(
                catalog[hit->control_index],*track,position);
            return value &&
                navigation_.focus_visible_row(hit->catalog_index) &&
                profile_.set_recovered_slider(hit->catalog_index,
                                             hit->control_index,*value);
        }
        default:return false;
    }
}
std::optional<std::string> WglRetainedMenuPreview::profile_snapshot(
        HGLRC context,HDC dc,unsigned width,unsigned height) const {
    if(!active_ || !owner_thread() || drawn_frames_==0 ||
       !context || !dc || context!=context_ || dc!=dc_ ||
       width!=width_ || height!=height_ ||
       wglGetCurrentContext()!=context ||
       wglGetCurrentDC()!=dc)
        return std::nullopt;
    const std::string data=serialize_settings(profile_.settings());
    // This strict roundtrip ensures no invalid settings can cross the
    // JNI evidence boundary; no runtime receives or executes this data.
    const auto validated=parse_settings(data);
    if(!validated.valid || data.empty() || data.size()>4096u)
        return std::nullopt;
    return data;
}
bool WglRetainedMenuPreview::close() noexcept {
    if(!owner_thread() || !active_)return false;
    // Every frame uses a stack-local WGL host that is already closed.
    // Drop all held input and the owner-bound snapshot epoch.
    pointer_gesture_.cancel();
    if(!pointer_frames_.detach())return false;
    context_=nullptr;
    dc_=nullptr;
    width_=height_=0;
    active_=false;
    return true;
}
std::optional<std::size_t> WglRetainedMenuPreview::focused_row() const {
    return owner_thread() && active_?navigation_.visible_focus():std::nullopt;
}
std::optional<std::size_t> WglRetainedMenuPreview::settings_page(
        std::size_t index) const noexcept {
    if(!owner_thread() || !active_ || index>=official_module_catalog().size() ||
       official_module_catalog()[index].tab!=menu_.tab() ||
       !menu_.expanded(index))return std::nullopt;
    return menu_.settings_offset(index);
}
std::optional<unsigned> WglRetainedMenuPreview::aim_preview_mode() const noexcept {
    if(!owner_thread() || !active_)return std::nullopt;
    return menu_.aim_preview_mode();
}
bool WglRetainedMenuPreview::preview_dropdown_open() const noexcept {
    return owner_thread() && active_ &&
           menu_.dropdown_preview().has_value();
}
bool WglRetainedMenuPreview::preview_enabled(std::size_t index) const noexcept {
    return owner_thread() && active_ && profile_.enabled(index);
}
} // namespace slinkywinky
