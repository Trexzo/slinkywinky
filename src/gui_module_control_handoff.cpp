#include "slinkywinky/gui_module_control_handoff.hpp"
#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include <algorithm>
#include <limits>
#include <string_view>

namespace slinkywinky {
GuiModuleControlHandoff::GuiModuleControlHandoff()
    :gui_owner_(std::this_thread::get_id()) {}

bool GuiModuleControlHandoff::module_owner_locked() const noexcept {
    return module_owner_!=std::thread::id{} &&
        module_owner_==std::this_thread::get_id();
}
void GuiModuleControlHandoff::retire_locked() noexcept {
    view_.phase=GuiModuleControlPhase::Retired;
    view_.request.reset();
    view_.last_confirmed_active=false;
}
bool GuiModuleControlHandoff::request_focused_toggle(
        const MenuState& menu,const MenuNavigation& navigation,
        const ModuleArrayListFeed& source) {
    if(std::this_thread::get_id()!=gui_owner_ ||
       !navigation.owns_menu(menu) || menu.color_popup_owner() ||
       menu.unload_confirmation_preview())
        return false;
    const auto focus=navigation.visible_focus();
    const auto& catalog=official_module_catalog();
    if(!focus || *focus>=catalog.size() ||
       catalog[*focus].tab!=menu.tab() ||
       std::string_view(catalog[*focus].name)!="Sprint")
        return false;
    // A snapshot may be read off the module owner, but cannot be forged
    // from MenuState::preview_enabled or interpreted as an acknowledgement.
    const auto copy=source.latest();
    if(!copy || !make_module_arraylist_display(*copy))return false;
    const auto it=std::find_if(copy->modules.begin(),copy->modules.end(),
        [](const ModuleArrayListState& row){
            return row.canonical_name=="Sprint";
        });
    if(it==copy->modules.end())return false;
    std::lock_guard<std::mutex> guard(mutex_);
    if(view_.phase==GuiModuleControlPhase::Retired ||
       view_.phase==GuiModuleControlPhase::Pending ||
       view_.phase==GuiModuleControlPhase::Dispatched ||
       next_request_id_==0 ||
       next_request_id_==std::numeric_limits<std::uint64_t>::max() ||
       copy->source_sequence<view_.last_confirmed_source_sequence)
        return false;
    GuiModuleControlRequest request;
    request.id=next_request_id_++;
    request.observed_source_sequence=copy->source_sequence;
    request.world_generation=copy->world_generation;
    request.canonical_name="Sprint";
    request.enable=!it->requested_enabled;
    view_.request=std::move(request);
    view_.phase=GuiModuleControlPhase::Pending;
    return true;
}
std::optional<GuiModuleControlRequest>
GuiModuleControlHandoff::take_on_module_owner() {
    std::lock_guard<std::mutex> guard(mutex_);
    if(view_.phase!=GuiModuleControlPhase::Pending ||
       !view_.request)return std::nullopt;
    if(module_owner_==std::thread::id{})
        module_owner_=std::this_thread::get_id();
    if(!module_owner_locked())return std::nullopt;
    view_.phase=GuiModuleControlPhase::Dispatched;
    return view_.request; // value-only, no GUI or game pointers
}
bool GuiModuleControlHandoff::acknowledge_on_module_owner(
        std::uint64_t id,const ModuleArrayListSnapshot& acknowledged,
        ModuleArrayListFeed& published_view) {
    std::lock_guard<std::mutex> guard(mutex_);
    if(!module_owner_locked() ||
       view_.phase!=GuiModuleControlPhase::Dispatched ||
       !view_.request || view_.request->id!=id)return false;
    const auto& request=*view_.request;
    const auto result=make_module_arraylist_display(acknowledged);
    const auto match=std::find_if(acknowledged.modules.begin(),
        acknowledged.modules.end(),[&request](const ModuleArrayListState& row){
            return row.canonical_name==request.canonical_name;
        });
    if(!result || match==acknowledged.modules.end() ||
       acknowledged.source_sequence<=request.observed_source_sequence ||
       acknowledged.world_generation<request.world_generation ||
       match->requested_enabled!=request.enable) {
        retire_locked();
        published_view.retire();
        return false;
    }
    if(!published_view.publish(acknowledged)) {
        retire_locked();
        published_view.retire();
        return false;
    }
    view_.phase=GuiModuleControlPhase::Acknowledged;
    view_.last_confirmed_source_sequence=acknowledged.source_sequence;
    view_.last_confirmed_active=match->active_in_world;
    return true;
}
bool GuiModuleControlHandoff::deny_on_module_owner(std::uint64_t id) {
    std::lock_guard<std::mutex> guard(mutex_);
    if(!module_owner_locked() ||
       view_.phase!=GuiModuleControlPhase::Dispatched ||
       !view_.request || view_.request->id!=id)return false;
    view_.phase=GuiModuleControlPhase::Denied;
    // The authoritative state feed remains unchanged on a denial.
    return true;
}
GuiModuleControlView GuiModuleControlHandoff::view() const {
    std::lock_guard<std::mutex> guard(mutex_);
    return view_;
}
void GuiModuleControlHandoff::retire() {
    std::lock_guard<std::mutex> guard(mutex_);
    retire_locked();
}
} // namespace slinkywinky
