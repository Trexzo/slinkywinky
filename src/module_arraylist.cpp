#include "slinkywinky/module_arraylist.hpp"
#include "slinkywinky/menu.hpp"
#include <algorithm>
#include <array>
#include <mutex>
#include <utility>
#include <string_view>

namespace slinkywinky {
std::optional<ModuleArrayListDisplay> make_module_arraylist_display(
        const ModuleArrayListSnapshot& snapshot) {
    constexpr std::uint32_t version=1u;
    if(snapshot.contract_version!=version ||
       snapshot.source_sequence==0u || snapshot.modules.size()>66u ||
       (snapshot.world_ready && snapshot.world_generation==0u))
        return std::nullopt;
    std::array<bool,67> seen{};
    ModuleArrayListDisplay display;
    display.source_sequence=snapshot.source_sequence;
    display.world_generation=snapshot.world_generation;
    const auto& catalog=official_module_catalog();
    for(const auto& state:snapshot.modules) {
        if(state.canonical_name.empty() ||
           state.canonical_name.size()>48u ||
           state.world_generation!=snapshot.world_generation ||
           (state.active_in_world &&
             (!state.requested_enabled || !snapshot.world_ready)))
            return std::nullopt;
        const auto it=std::find_if(catalog.begin(),catalog.end(),
            [&state](const MenuModule& module){
                return std::string_view(module.name)==state.canonical_name;
            });
        if(it==catalog.end())return std::nullopt;
        const std::size_t index=static_cast<std::size_t>(
            std::distance(catalog.begin(),it));
        if(seen[index])return std::nullopt;
        seen[index]=true;
        if(state.active_in_world)
            display.active_names.push_back(state.canonical_name);
    }
    // This order is deterministic for portable fixtures. The GL renderer
    // sorts again by actual installed-font pixel width, as an ArrayList
    // normally does, without changing owner-provided activation status.
    std::sort(display.active_names.begin(),display.active_names.end(),
        [](const std::string& a,const std::string& b){
            return a.size()!=b.size()?a.size()>b.size():a<b;
        });
    return display;
}

bool ModuleArrayListFeed::publish(const ModuleArrayListSnapshot& snapshot) {
    // Validate and copy before taking the short publication lock.
    // A failed validation must not leave an older active list on screen.
    const auto admitted=make_module_arraylist_display(snapshot);
    std::optional<ModuleArrayListSnapshot> copy;
    if(admitted)copy=snapshot;
    std::lock_guard<std::mutex> lock(mutex_);
    if(retired_)return false;
    if(!copy || snapshot.source_sequence<=sequence_ ||
       snapshot.world_generation<last_world_generation_) {
        latest_.reset();
        retired_=true; // quarantine replay, malformed or regressing producer
        return false;
    }
    latest_=std::move(copy);
    sequence_=snapshot.source_sequence;
    last_world_generation_=snapshot.world_generation;
    return true;
}
std::optional<ModuleArrayListSnapshot> ModuleArrayListFeed::latest() const {
    std::lock_guard<std::mutex> lock(mutex_);
    if(retired_)return std::nullopt;
    return latest_; // deep value copy, never a producer-owned pointer
}
void ModuleArrayListFeed::retire() {
    std::lock_guard<std::mutex> lock(mutex_);
    latest_.reset();
    retired_=true;
}
bool ModuleArrayListFeed::retired() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return retired_;
}

} // namespace slinkywinky
