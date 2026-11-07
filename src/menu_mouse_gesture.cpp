#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_numeric_geometry.hpp"
#include <utility>

namespace slinkywinky {
namespace {
constexpr std::uint64_t max_press_ms=1500;
constexpr std::uint64_t max_numeric_press_ms=30000; // bounded captured drag
constexpr std::uint64_t max_configured_double_click_ms=5000;
}

MenuMouseGesture::MenuMouseGesture() noexcept :
    owner_(std::this_thread::get_id()) {}

bool MenuMouseGesture::owner_thread() const noexcept {
    return owner_==std::this_thread::get_id();
}
bool MenuMouseGesture::pending() const noexcept {
    return owner_thread() && active_.has_value();
}
bool MenuMouseGesture::set_double_click_window(
    std::uint64_t milliseconds) noexcept {
    if(!owner_thread() || milliseconds==0 ||
       milliseconds>max_configured_double_click_ms)return false;
    active_.reset(); // A changed timing policy cannot validate a held click.
    double_click_ms_=milliseconds;
    return true;
}
bool MenuMouseGesture::cancel() noexcept {
    if(!owner_thread())return false;
    active_.reset();
    return true;
}
bool MenuMouseGesture::same_hit(
    const MenuHit& a,const MenuHit& b) noexcept {
    return a.kind==b.kind && a.tab==b.tab &&
           a.catalog_index==b.catalog_index &&
           a.control_index==b.control_index &&
           a.sub_index==b.sub_index;
}
// Only a source-typed range track may change which handle is nearest
// during a drag. The originally captured handle remains the owner.
static bool numeric_hit(const MenuHit& hit) noexcept {
    return hit.kind==MenuHitKind::RecoveredSliderPreview ||
           hit.kind==MenuHitKind::RecoveredRangePreview;
}
// A numeric drag can follow pointer capture beyond the track, provided
// the pointer still belongs to the current isolated preview panel and the
// original numeric track remains source-visible. Other GUI gestures keep
// the strict same-hit rule from R98.
static bool numeric_captured_point(const MenuRenderFrame& frame,
                                   MenuPanelMode mode,
                                   const MenuHit& held,int x,int y) noexcept {
    if(!numeric_hit(held))return false;
    const auto panel=menu_panel_geometry(
        frame.viewport_width,frame.viewport_height,frame.appearance.wide,mode);
    if(!panel || x<static_cast<int>(panel->left) ||
       y<static_cast<int>(panel->top) ||
       static_cast<std::int64_t>(x)>=
         static_cast<std::int64_t>(panel->left)+panel->width ||
       static_cast<std::int64_t>(y)>=
         static_cast<std::int64_t>(panel->top)+panel->height)
        return false;
    return menu_numeric_track_geometry(
        frame,panel->logical_width(),panel->logical_height(),
        held.catalog_index,held.control_index).has_value();
}
static bool same_range_track(const MenuHit& press,
                             const MenuHit& next) noexcept {
    return press.kind==MenuHitKind::RecoveredRangePreview &&
           next.kind==MenuHitKind::RecoveredRangePreview &&
           press.tab==next.tab &&
           press.catalog_index==next.catalog_index &&
           press.control_index==next.control_index;
}
bool MenuMouseGesture::equivalent_frame(
    const MenuRenderFrame& frame,MenuPanelMode mode) const noexcept {
    if(!active_ || active_->mode!=mode)return false;
    const auto& original=active_->frame;
    // R148: a numeric press may change ONLY its own temporary display
    // value while captured. All other UI mutations still invalidate the
    // gesture, including settings, hover identity, layout and modal state.
    if(frame.numeric_drag_preview!=original.numeric_drag_preview) {
        const auto& hit=active_->hit;
        const auto& drag=frame.numeric_drag_preview;
        const bool slider=hit.kind==MenuHitKind::RecoveredSliderPreview;
        const bool range=hit.kind==MenuHitKind::RecoveredRangePreview;
        if(!drag || original.numeric_drag_preview ||
           (!slider && !range) ||
           drag->catalog_index!=hit.catalog_index ||
           drag->widget_index!=hit.control_index ||
           drag->handle!=(range?hit.sub_index:0u))
            return false;
    }
    if(frame.epoch!=original.epoch || frame.sequence<original.sequence ||
       frame.viewport_width!=original.viewport_width ||
       frame.viewport_height!=original.viewport_height ||
       frame.tab!=original.tab ||
       frame.aim_preview_mode!=original.aim_preview_mode ||
       frame.target_preview_mode!=original.target_preview_mode ||
       frame.piercing_selected!=original.piercing_selected ||
       frame.dropdown_preview!=original.dropdown_preview ||
       frame.unload_confirmation_preview!=original.unload_confirmation_preview ||
       frame.bind_previews!=original.bind_previews ||
       frame.bind_capture_index!=original.bind_capture_index ||
       frame.bind_hover_row!=original.bind_hover_row ||
       frame.numeric_hover!=original.numeric_hover ||
       frame.recovered_boolean_overrides!=original.recovered_boolean_overrides ||
       frame.recovered_choice_overrides!=original.recovered_choice_overrides ||
       frame.recovered_multi_overrides!=original.recovered_multi_overrides ||
       frame.recovered_color_overrides!=original.recovered_color_overrides ||
       frame.recovered_text_overrides!=original.recovered_text_overrides ||
       frame.focused_text_widget!=original.focused_text_widget ||
       frame.recovered_slider_overrides!=original.recovered_slider_overrides ||
       frame.recovered_range_overrides!=original.recovered_range_overrides ||
       !(frame.piercing_preview==original.piercing_preview) ||
       frame.is_placeholder!=original.is_placeholder ||
       frame.profile_cards.size()!=original.profile_cards.size() ||
       frame.profile_name_draft!=original.profile_name_draft ||
       frame.profile_name_focused!=original.profile_name_focused ||
       frame.profile_name_taken!=original.profile_name_taken ||
       frame.profile_action_preview!=original.profile_action_preview ||
       frame.profile_icon_hover!=original.profile_icon_hover ||
       frame.profile_bind_capture_index!=
           original.profile_bind_capture_index ||
       frame.appearance.wide!=original.appearance.wide ||
       frame.appearance.compact!=original.appearance.compact ||
       frame.appearance.allow_input_while_open!=
           original.appearance.allow_input_while_open ||
       frame.appearance.accent_rgb!=original.appearance.accent_rgb ||
       frame.appearance.scale!=original.appearance.scale ||
       frame.visible.size()!=original.visible.size() ||
       frame.parameters.size()!=original.parameters.size() ||
       frame.profile_cards.size()!=original.profile_cards.size())
        return false;
    for(std::size_t i=0;i<frame.visible.size();++i) {
        const auto& a=frame.visible[i];
        const auto& b=original.visible[i];
        if(a.catalog_index!=b.catalog_index || a.title!=b.title ||
           a.preview_enabled!=b.preview_enabled || a.expanded!=b.expanded ||
           a.settings_offset!=b.settings_offset)
            return false;
    }
    for(std::size_t i=0;i<frame.parameters.size();++i) {
        const auto& a=frame.parameters[i];
        const auto& b=original.parameters[i];
        if(a.catalog_index!=b.catalog_index || a.caption!=b.caption ||
           a.numeric!=b.numeric || a.value!=b.value ||
           a.minimum!=b.minimum || a.maximum!=b.maximum)
            return false;
    }
    // Profile card changes invalidate any held navigation click, even
    // though card icons themselves cannot trigger an action in R114.
    for(std::size_t i=0;i<frame.profile_cards.size();++i) {
        const auto& a=frame.profile_cards[i];
        const auto& b=original.profile_cards[i];
        if(a.name!=b.name || a.description!=b.description ||
           a.bind!=b.bind || a.is_default!=b.is_default)
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
    if(hit && (same_hit(*hit,active_->hit) ||
               same_range_track(active_->hit,*hit)))
        return true;
    if(numeric_captured_point(
        frame,mode,active_->hit,x,y))return true;
    // Non-numeric drags and any pointer outside the owned panel still
    // cancel terminally; re-entering never revives the old gesture.
    active_.reset();
    return false;
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
        monotonic_ms-pressed_at<=
            (numeric_hit(previous)?max_numeric_press_ms:max_press_ms);
    active_.reset(); // A release is consumed even if it fails validation.
    if(!valid)return std::nullopt;
    const auto released=hit_test_menu(frame,mode,x,y);
    if(!(released && (same_hit(previous,*released) ||
                       same_range_track(previous,*released))) &&
       !numeric_captured_point(frame,mode,previous,x,y))
        return std::nullopt;
    // R133: retain the *press-selected* range handle. Moving across the
    // other handle never changes ownership or causes a cross-handle write.
    // A pointer-captured slider, like a dual-handle range, retains
    // the exact numeric press owner even if the release now hits a label,
    // another row or empty panel space. Never dereference an absent
    // released hit or dispatch the unrelated row's action.
    const MenuHit committed=numeric_hit(previous)?previous:*released;
    // Ignore the second click of a rapid double click on the same action.
    // Allow a different target (e.g. a different row) without delay.
    if(last_committed_ && same_hit(committed,*last_committed_) &&
       monotonic_ms>=last_committed_ms_ &&
       monotonic_ms-last_committed_ms_<=double_click_ms_)
        return std::nullopt;
    last_committed_=committed;
    last_committed_ms_=monotonic_ms;
    return committed;
}
} // namespace slinkywinky
