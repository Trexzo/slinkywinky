#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <vector>

namespace slinkywinky {
namespace {
std::vector<std::size_t> indices_for(MenuTab tab) {
    std::vector<std::size_t> result;
    const auto& catalog=official_module_catalog();
    for(std::size_t i=0;i<catalog.size();++i)
        if(catalog[i].tab==tab)result.push_back(i);
    return result;
}
}

MenuNavigation::MenuNavigation(MenuState& menu,
                               MenuProfileBridge& profile)
    : menu_(menu),profile_(profile),thread_(std::this_thread::get_id()) {
    if(profile_.owns(menu_)) {
        const auto visible=menu_.page();
        if(!visible.empty())focused_=visible.front().catalog_index;
    }
}
bool MenuNavigation::owns(const MenuState& menu,
                          const MenuProfileBridge& profile) const noexcept {
    return &menu_==&menu && &profile_==&profile &&
           profile_.owns(menu_) && owner_thread();
}
bool MenuNavigation::owner_thread() const noexcept {
    return thread_==std::this_thread::get_id();
}
bool MenuNavigation::color_modal_open() const noexcept {
    return owner_thread() && menu_.color_popup_owner().has_value();
}
std::optional<std::size_t> MenuNavigation::visible_focus() const {
    if(!owner_thread() || !focused_ || !profile_.owns(menu_))
        return std::nullopt;
    const auto visible=menu_.page();
    const auto found=std::find_if(visible.begin(),visible.end(),
        [this](const MenuItem& row){
            return row.catalog_index==*focused_;
        });
    if(found==visible.end())return std::nullopt;
    return focused_;
}
bool MenuNavigation::focused_visible(std::size_t* index) const {
    const auto current=visible_focus();
    if(!current || !index)return false;
    *index=*current;
    return true;
}
bool MenuNavigation::select_tab(MenuTab tab) {
    if(menu_.color_popup_owner() ||
       menu_.unload_confirmation_preview() ||
       !owner_thread() || !profile_.owns(menu_) ||
       tab==MenuTab::Count || tab==MenuTab::Unload)return false;
    if(!profile_.select(tab))return false;
    const auto visible=menu_.page();
    focused_=visible.empty()?std::nullopt:
             std::optional<std::size_t>(visible.front().catalog_index);
    return true;
}
bool MenuNavigation::focus_visible_row(std::size_t catalog_index) {
    if(menu_.color_popup_owner() ||
       menu_.unload_confirmation_preview() ||
       !owner_thread() || !profile_.owns(menu_))return false;
    const auto rows=menu_.page();
    const auto found=std::find_if(rows.begin(),rows.end(),
        [catalog_index](const MenuItem& row){
            return row.catalog_index==catalog_index;
        });
    if(found==rows.end())return false;
    focused_=catalog_index;
    return true;
}
bool MenuNavigation::switch_tab(int direction) {
    if(direction!=1 && direction!=-1)return false;
    // Only the six informational/preview tabs are keyboard navigable.
    // Unload and the Count sentinel must never be selected.
    const int before=static_cast<int>(menu_.tab());
    if(before<0 || before>5)return false;
    const int after=(before+direction+6)%6;
    return select_tab(static_cast<MenuTab>(after));
}
bool MenuNavigation::move_row(int direction) {
    if(direction!=1 && direction!=-1)return false;
    const auto indices=indices_for(menu_.tab());
    if(indices.empty())return false;
    const auto it=focused_?
        std::find(indices.begin(),indices.end(),*focused_):indices.end();
    std::size_t ordinal=0;
    if(it==indices.end()) {
        ordinal=0;
    } else {
        ordinal=static_cast<std::size_t>(it-indices.begin());
        if((direction<0 && ordinal==0) ||
           (direction>0 && ordinal+1==indices.size()))
            return false;
        ordinal=direction>0?ordinal+1:ordinal-1;
    }
    const auto next=indices[ordinal];
    const auto currentScroll=menu_.scroll();
    const std::size_t desiredScroll=
        ordinal<currentScroll?ordinal:
        (ordinal>=currentScroll+8?ordinal-7:currentScroll);
    if(desiredScroll!=currentScroll &&
       !profile_.scroll_by(static_cast<int>(desiredScroll)-
                           static_cast<int>(currentScroll)))
        return false;
    focused_=next;
    return visible_focus()==focused_;
}
bool MenuNavigation::apply(MenuNavAction action) {
    if(!owner_thread() || !profile_.owns(menu_) ||
       menu_.color_popup_owner() ||
       menu_.unload_confirmation_preview())return false;
    switch(action) {
        case MenuNavAction::NextTab: return switch_tab(1);
        case MenuNavAction::PreviousTab: return switch_tab(-1);
        case MenuNavAction::NextRow: return move_row(1);
        case MenuNavAction::PreviousRow: return move_row(-1);
        case MenuNavAction::NextPiercingControl:
        case MenuNavAction::PreviousPiercingControl:
        case MenuNavAction::ActivatePiercingControl: {
            std::size_t index=0;
            if(!focused_visible(&index) || index!=11 ||
               menu_.tab()!=MenuTab::Combat || !menu_.expanded(index))
                return false;
            if(action==MenuNavAction::ActivatePiercingControl)
                return menu_.activate_piercing_selection();
            return menu_.move_piercing_selection(
                action==MenuNavAction::NextPiercingControl?1:-1);
        }
        case MenuNavAction::NextAimPreviewMode:
        case MenuNavAction::NextTargetPreviewMode: {
            std::size_t index=0;
            if(!focused_visible(&index) || index!=0 ||
               menu_.tab()!=MenuTab::Combat || !menu_.expanded(index))
                return false;
            if(action==MenuNavAction::NextAimPreviewMode)
                return menu_.set_aim_preview_mode(
                    (menu_.aim_preview_mode()+1u)%4u);
            return menu_.set_target_preview_mode(
                (menu_.target_preview_mode()+1u)%2u);
        }
        case MenuNavAction::NextSettingsPage:
        case MenuNavAction::PreviousSettingsPage:
        case MenuNavAction::ExpandFocused:
        case MenuNavAction::ToggleFocusedPreview:
        case MenuNavAction::IncreaseSetting:
        case MenuNavAction::DecreaseSetting: {
            std::size_t index=0;
            if(!focused_visible(&index))return false;
            // Read-only source settings may be expanded even when their
            // gameplay module has no first-party implementation. Preview
            // enablement and numeric mutation remain gated separately.
            if(action==MenuNavAction::ExpandFocused)
                return recovered_widget_count(
                    official_module_catalog()[index].name)>0 &&
                    menu_.toggle_expanded(index);
            if(action==MenuNavAction::NextSettingsPage ||
               action==MenuNavAction::PreviousSettingsPage)
                return menu_.change_settings_page(index,
                    action==MenuNavAction::NextSettingsPage?1:-1);
            if(!module_has_profile_setting(index))
                return false;
            if(action==MenuNavAction::ToggleFocusedPreview)
                return profile_.toggle(index);
            return profile_.adjust_primary(
                index,action==MenuNavAction::IncreaseSetting?1:-1);
        }
    }
    return false;
}
} // namespace slinkywinky
