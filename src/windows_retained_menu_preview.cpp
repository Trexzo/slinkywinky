#ifndef NOMINMAX
#define NOMINMAX
#endif
#include "slinkywinky/windows_retained_menu_preview.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
#include <GL/gl.h>
#include <cmath>
#include <limits>
#include <utility>

namespace slinkywinky {
WglRetainedMenuPreview::WglRetainedMenuPreview()
    : profile_(menu_),navigation_(menu_,profile_),
      owner_(std::this_thread::get_id()) {}
bool WglRetainedMenuPreview::owner_thread() const noexcept {
    return owner_==std::this_thread::get_id();
}
// R182: sequence provenance, not a permission grant. Counters cannot
// overflow because each increment is bounded by the admitted sample count.
void WglRetainedMenuPreview::note_focus_sample(
        bool foreground,bool exact_source_hit) noexcept {
    if(focus_sample_seen_) {
        if(previous_foreground_ && !foreground) {
            ++foreground_losses_;
            post_regain_epoch_=false;
        } else if(!previous_foreground_ && foreground && had_foreground_) {
            ++foreground_regains_;
            post_regain_epoch_=true;
        }
    }
    if(foreground)had_foreground_=true;
    if(foreground && exact_source_hit && post_regain_epoch_)
        ++post_regain_exact_source_hits_;
    previous_foreground_=foreground;
    focus_sample_seen_=true;
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
    const bool was_active=active_;
    // Failed paint attempts must not retain previously certified input geometry.
    last_drawn_frame_.reset();
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
    // R180: snapshot ONLY after a successful real GL11 frame. A newly
    // navigated (but not yet drawn) menu cannot expose a source hover target.
    const auto rendered_frame=pointer_frames_.next(
        menu_,&profile_,&navigation_);
    if(!rendered_frame)return false;
    if(was_active) {
        if(source_visible_draws_==
           std::numeric_limits<std::uint64_t>::max())return false;
        if(menu_original_auto_block_range_target(
            *rendered_frame,width,height))++source_visible_draws_;
    }
    last_drawn_frame_=std::move(*rendered_frame);
    drawn_frames_++;
    return true;
}
bool WglRetainedMenuPreview::apply(MenuNavAction action) {
    if(!active_ || !owner_thread() ||
       accepted_actions_==std::numeric_limits<std::uint64_t>::max() ||
       refused_actions_==std::numeric_limits<std::uint64_t>::max())
        return false;
    // Navigation invalidates held gestures AND the last-painted hit surface.
    pointer_gesture_.cancel();
    last_drawn_frame_.reset();
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
    // A preview gesture may change presentation; require repaint before hover.
    last_drawn_frame_.reset();
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
bool WglRetainedMenuPreview::observe_hover(
        HGLRC context,HDC dc,unsigned width,unsigned height,
        int x,int y,bool foreground_verified) {
    // A read-only visual hint, not input ownership. A pending synthetic
    // gesture cannot have its immutable input frame altered by hover.
    if(!owner_thread() || !active_ || !drawn_frames_ ||
       !context || !dc || context!=context_ || dc!=dc_ ||
       width!=width_ || height!=height_ ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc ||
       pointer_gesture_.pending() ||
       hover_samples_==std::numeric_limits<std::uint64_t>::max() ||
       hover_foreground_==std::numeric_limits<std::uint64_t>::max() ||
       hover_numeric_==std::numeric_limits<std::uint64_t>::max() ||
       hover_bind_==std::numeric_limits<std::uint64_t>::max())return false;
    if(!last_drawn_frame_) {
        // R180: the old image is invalidated by navigation. Count this
        // original WGL observation as EMPTY (preserves 63-frame telemetry)
        // but never hit-test a control that has not yet been painted.
        menu_.set_numeric_hover(std::nullopt);
        menu_.set_bind_hover_row(std::nullopt);
        ++hover_samples_;
        if(foreground_verified)++hover_foreground_;
        note_focus_sample(foreground_verified,false);
        return true;
    }
    std::optional<MenuNumericHover> numeric;
    std::optional<std::size_t> bind;
    bool exactSourceHit=false;
    if(foreground_verified && x>=0 && y>=0 &&
       static_cast<unsigned>(x)<width &&
       static_cast<unsigned>(y)<height) {
        const auto& frame=*last_drawn_frame_;
        const auto hit=hit_test_menu(
            frame,MenuPanelMode::CompactOverlay,x,y);
        if(hit) {
            if(hit->kind==MenuHitKind::RecoveredSliderPreview ||
               hit->kind==MenuHitKind::RecoveredRangePreview)
                numeric=MenuNumericHover{
                    hit->catalog_index,hit->control_index};
            else if(hit->kind==MenuHitKind::BindChip ||
                    hit->kind==MenuHitKind::BindHold)
                bind=hit->catalog_index;
            if(numeric) {
                const auto source=menu_original_auto_block_range_target(
                    frame,width,height);
                exactSourceHit=source &&
                    source->catalog_index==numeric->catalog_index &&
                    source->widget_index==numeric->widget_index &&
                    source->contains(x,y);
            }
        }
    }
    // The source-typed MenuState validates even these ephemeral hints.
    // Unchanged empty hover is valid observation, not a failed mutation.
    if(numeric && menu_.numeric_hover()!=numeric &&
       !menu_.set_numeric_hover(numeric))return false;
    if(!numeric)menu_.set_numeric_hover(std::nullopt);
    if(bind && menu_.bind_hover_row()!=bind &&
       !menu_.set_bind_hover_row(bind))return false;
    if(!bind)menu_.set_bind_hover_row(std::nullopt);
    ++hover_samples_;
    // A foreground observation may be outside the original game client.
    // Only actual source hit-tests can set numeric/bind hover counters.
    if(foreground_verified)++hover_foreground_;
    if(numeric)++hover_numeric_;
    if(bind)++hover_bind_;
    if(exactSourceHit) {
        if(source_exact_hover_hits_==
           std::numeric_limits<std::uint64_t>::max())return false;
        ++source_exact_hover_hits_;
    }
    note_focus_sample(foreground_verified,exactSourceHit);
    return true;
}
std::optional<MenuSourceHoverTarget>
WglRetainedMenuPreview::source_hover_target(
        HGLRC context,HDC dc,unsigned width,unsigned height) {
    if(!owner_thread() || !active_ || !drawn_frames_ ||
       !context || !dc || context!=context_ || dc!=dc_ ||
       width!=width_ || height!=height_ ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc)
        return std::nullopt;
    return last_drawn_frame_?
        menu_original_auto_block_range_target(
            *last_drawn_frame_,width,height):std::nullopt;
}
std::optional<std::array<std::uint64_t,3>>
WglRetainedMenuPreview::source_hover_evidence(
        HGLRC context,HDC dc,unsigned width,unsigned height) const noexcept {
    if(!owner_thread() || !active_ || !drawn_frames_ ||
       !context || !dc || context!=context_ || dc!=dc_ ||
       width!=width_ || height!=height_ ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc ||
       source_visible_draws_>drawn_frames_ ||
       source_exact_hover_hits_>hover_foreground_ ||
       source_exact_hover_hits_>hover_numeric_)
        return std::nullopt;
    return std::array<std::uint64_t,3>{
        drawn_frames_,source_visible_draws_,source_exact_hover_hits_
    };
}
bool WglRetainedMenuPreview::observe_original_window_focus(
        HGLRC context,HDC dc,unsigned width,unsigned height,
        OriginalWindowFocusState state,bool pointer_admitted) noexcept {
    // One companion OS status per already-admitted passive hover. A
    // missing/repeated sample cannot be used to forge a focus transition.
    if(!owner_thread() || !active_ || !drawn_frames_ ||
       !context || !dc || context!=context_ || dc!=dc_ ||
       width!=width_ || height!=height_ ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc ||
       hover_samples_!=strict_hover_samples_+1 ||
       source_exact_hover_hits_<strict_source_hits_ ||
       source_exact_hover_hits_-strict_source_hits_>1)
        return false;
    const bool hit=source_exact_hover_hits_>strict_source_hits_;
    if(!exact_window_focus_.observe(state,pointer_admitted,hit))
        return false;
    strict_hover_samples_=hover_samples_;
    strict_source_hits_=source_exact_hover_hits_;
    return true;
}
std::optional<std::array<std::uint64_t,6>>
WglRetainedMenuPreview::original_window_focus_evidence(
        HGLRC context,HDC dc,unsigned width,unsigned height) const noexcept {
    if(!owner_thread() || !active_ || !drawn_frames_ ||
       !context || !dc || context!=context_ || dc!=dc_ ||
       width!=width_ || height!=height_ ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc ||
       strict_hover_samples_!=hover_samples_ ||
       exact_window_focus_.samples()!=hover_samples_)
        return std::nullopt;
    const auto counts=exact_window_focus_.counters();
    if(counts[0]+counts[1]+counts[2]!=hover_samples_ ||
       counts[0]>hover_foreground_ ||
       counts[4]>counts[3] || counts[5]>source_exact_hover_hits_)
        return std::nullopt;
    return counts;
}
std::optional<std::array<std::uint64_t,3>>
WglRetainedMenuPreview::focus_cycle_evidence(
        HGLRC context,HDC dc,unsigned width,unsigned height) const noexcept {
    if(!owner_thread() || !active_ || !drawn_frames_ ||
       !context || !dc || context!=context_ || dc!=dc_ ||
       width!=width_ || height!=height_ ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc ||
       foreground_losses_>hover_samples_ ||
       foreground_regains_>foreground_losses_ ||
       post_regain_exact_source_hits_>source_exact_hover_hits_ ||
       (foreground_regains_==0 && post_regain_exact_source_hits_!=0))
        return std::nullopt;
    return std::array<std::uint64_t,3>{
        foreground_losses_,foreground_regains_,
        post_regain_exact_source_hits_
    };
}
std::optional<std::array<std::uint64_t,5>>
WglRetainedMenuPreview::hover_evidence(
        HGLRC context,HDC dc,unsigned width,unsigned height) const noexcept {
    if(!owner_thread() || !active_ || !drawn_frames_ ||
       !context || !dc || context!=context_ || dc!=dc_ ||
       width!=width_ || height!=height_ ||
       wglGetCurrentContext()!=context || wglGetCurrentDC()!=dc ||
       hover_foreground_>hover_samples_ ||
       hover_numeric_>hover_foreground_ ||
       hover_bind_>hover_foreground_ ||
       hover_numeric_+hover_bind_<hover_numeric_ ||
       hover_numeric_+hover_bind_>hover_foreground_)
        return std::nullopt;
    return std::array<std::uint64_t,5>{
        drawn_frames_,hover_samples_,hover_foreground_,
        hover_numeric_,hover_bind_
    };
}
std::optional<MenuNumericHover>
WglRetainedMenuPreview::observed_numeric_hover() const noexcept {
    return owner_thread() && active_?menu_.numeric_hover():std::nullopt;
}
std::optional<std::size_t>
WglRetainedMenuPreview::observed_bind_hover() const noexcept {
    return owner_thread() && active_?menu_.bind_hover_row():std::nullopt;
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
    last_drawn_frame_.reset();
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
