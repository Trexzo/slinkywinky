#include "slinkywinky/menu_keyboard_router.hpp"
#include <algorithm>

namespace slinkywinky {

MenuKeyboardRouter::MenuKeyboardRouter(MenuNavigation& navigation) noexcept
    : owner_(std::this_thread::get_id()),navigation_(navigation) {}

bool MenuKeyboardRouter::same_owner() const noexcept {
    return std::this_thread::get_id()==owner_ && navigation_.owner_thread();
}
bool MenuKeyboardRouter::all_released() const noexcept {
    return std::none_of(pressed_.begin(),pressed_.end(),
                        [](bool down){return down;});
}
bool MenuKeyboardRouter::begin(std::uint64_t epoch,bool exclusive_dispatch_attested,
                               bool keys_and_buttons_released) noexcept {
    if(!same_owner() || state_==State::Active || state_==State::Draining ||
       !epoch || epoch<=last_epoch_ || !exclusive_dispatch_attested ||
       !keys_and_buttons_released)return false;
    last_epoch_=epoch;
    pressed_.fill(false);
    state_=State::Active;
    return true;
}

std::optional<MenuNavAction> MenuKeyboardRouter::action_for(unsigned key) noexcept {
    // Exact LWJGL2 key codes; this model never calls Keyboard or reads OS keys.
    switch(key) {
        case 15: case 205:return MenuNavAction::NextTab;     // Tab / Right
        case 203:return MenuNavAction::PreviousTab;          // Left
        case 208:return MenuNavAction::NextRow;              // Down
        case 200:return MenuNavAction::PreviousRow;          // Up
        case 201:return MenuNavAction::PreviousSettingsPage; // LWJGL2 KEY_PRIOR / Page Up
        case 209:return MenuNavAction::NextSettingsPage;     // LWJGL2 KEY_NEXT / Page Down
        case 28:return MenuNavAction::ExpandFocused;         // Enter
        case 57:return MenuNavAction::ToggleFocusedPreview; // Space
        case 78:return MenuNavAction::IncreaseSetting;       // Numpad +
        case 74:return MenuNavAction::DecreaseSetting;       // Numpad -
        default:return std::nullopt;
    }
}
MenuKeyOutcome MenuKeyboardRouter::event(unsigned lwjgl_key,bool down,
        bool foreground_verified,bool event_owner_verified,
        bool menu_visible) noexcept {
    if(!same_owner())return {MenuKeyDelivery::Quarantined,false,false};
    if(state_==State::Revoked)
        return {MenuKeyDelivery::Quarantined,false,false};
    if(state_==State::Idle)
        return {menu_visible?MenuKeyDelivery::Quarantined:
                            MenuKeyDelivery::GameUnchanged,false,false};
    if(state_==State::Active) {
        if(!event_owner_verified || !foreground_verified) {
            // No ambiguous event is ever released to gameplay by this model.
            state_=State::Revoked;
            return {MenuKeyDelivery::Quarantined,false,false};
        }
        if(!menu_visible)state_=State::Draining;
    }
    if(state_==State::Draining) {
        if(lwjgl_key<pressed_.size() && !down)pressed_[lwjgl_key]=false;
        if(all_released())state_=State::Idle;
        return {MenuKeyDelivery::Quarantined,false,false};
    }
    if(lwjgl_key>=pressed_.size()) {
        state_=State::Revoked; // Unknown LWJGL code: no blind pass-through.
        return {MenuKeyDelivery::Quarantined,false,false};
    }
    const bool rising=down && !pressed_[lwjgl_key];
    pressed_[lwjgl_key]=down;
    if(!rising)return {MenuKeyDelivery::MenuExclusive,false,false};
    // R201: a modal captures all keyboard events. Do not translate Tab,
    // arrows, Enter or Escape into background navigation or menu-close.
    // The exact owner-bound WGL dispatcher may edit the HEX preview
    // separately after this quarantined, key-paired router receipt.
    if(navigation_.color_modal_open())
        return {MenuKeyDelivery::MenuExclusive,false,false};
    // Insert / Escape only request GUI close. The event and its release
    // remain GUI-owned until end() drains the close key.
    if(lwjgl_key==210 || lwjgl_key==1)
        return {MenuKeyDelivery::MenuExclusive,true,false};
    const auto action=action_for(lwjgl_key);
    return {MenuKeyDelivery::MenuExclusive,false,
            action && navigation_.apply(*action)};
}
bool MenuKeyboardRouter::end() noexcept {
    if(!same_owner() || state_!=State::Active)return false;
    state_=all_released()?State::Idle:State::Draining;
    return true;
}
bool MenuKeyboardRouter::release_after_abort(bool menu_closed,
        bool controls_released,bool original_input_restored) noexcept {
    if(!same_owner() ||
       (state_!=State::Revoked && state_!=State::Draining) ||
       !menu_closed || !controls_released || !original_input_restored)
        return false;
    // The host must independently certify restoration. Never infer it from
    // the visual overlay disappearing or from one released key event.
    pressed_.fill(false);
    state_=State::Idle;
    return true;
}
bool MenuKeyboardRouter::active() const noexcept {
    return same_owner() && state_==State::Active;
}
bool MenuKeyboardRouter::revoked() const noexcept {
    return !same_owner() || state_==State::Revoked;
}
std::uint64_t MenuKeyboardRouter::epoch() const noexcept {
    return same_owner()?last_epoch_:0;
}
} // namespace slinkywinky
