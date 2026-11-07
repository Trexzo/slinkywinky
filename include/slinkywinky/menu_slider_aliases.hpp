#pragma once
#include "slinkywinky/recovered_settings.hpp"
#include <cstddef>
#include <optional>
#include <string_view>

namespace slinkywinky {

// R146: the candidate native mod_aim_assist.cpp registers one Horizontal
// speed and one Vertical speed numeric setting, not two of each. The R116
// presentation fixture contains separate *visibility callsites* in Silent
// and Regular/Linear modes. They must edit the same local preview value.
inline bool menu_is_aim_speed_branch(
        std::size_t widget_index) noexcept {
    const auto& all=recovered_widget_catalog();
    if(widget_index>=all.size())return false;
    const auto& w=all[widget_index];
    return std::string_view(w.module)=="Aim Assist" &&
           std::string_view(w.group)=="Aim" &&
           (std::string_view(w.label)=="Horizontal speed" ||
            std::string_view(w.label)=="Vertical speed") &&
           w.kind==RecoveredWidgetKind::Slider &&
           (w.visibility==RecoveredWidgetVisibility::AimModeSilent ||
            w.visibility==RecoveredWidgetVisibility::AimModeRegularLinear);
}
inline std::optional<std::size_t> menu_aim_speed_alias_peer(
        std::size_t widget_index) noexcept {
    if(!menu_is_aim_speed_branch(widget_index))return std::nullopt;
    const auto& all=recovered_widget_catalog();
    const auto& item=all[widget_index];
    const auto opposite=item.visibility==
        RecoveredWidgetVisibility::AimModeSilent?
        RecoveredWidgetVisibility::AimModeRegularLinear:
        RecoveredWidgetVisibility::AimModeSilent;
    std::optional<std::size_t> peer;
    for(std::size_t i=0;i<all.size();++i) {
        if(i==widget_index || !menu_is_aim_speed_branch(i))continue;
        const auto& w=all[i];
        if(w.visibility!=opposite ||
           std::string_view(w.label)!=item.label)continue;
        if(peer || w.min_value!=item.min_value ||
           w.max_value!=item.max_value || w.step!=item.step ||
           w.example_value!=item.example_value)
            return std::nullopt; // ambiguity: fail closed
        peer=i;
    }
    return peer;
}
} // namespace slinkywinky
