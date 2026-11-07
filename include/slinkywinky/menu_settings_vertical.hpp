#pragma once
#include "slinkywinky/recovered_settings.hpp"
#include <array>
#include <algorithm>
#include <cstddef>
#include <string_view>
namespace slinkywinky {
// R152: candidate ClickGUI .in flex-column gap:12, .row:20,
// .sec:19, card padding:8. All offsets are from the closed-card body
// height (68px regular, 36px compact).
constexpr float menu_settings_row_pitch=32.0f;
constexpr float menu_settings_section_pitch=31.0f;
constexpr float menu_settings_first_control_offset=35.0f;
constexpr float menu_settings_pagination_pitch=32.0f;
// Count section transitions from widget 0 through widget i (inclusive).
inline unsigned menu_settings_section_changes(
        const std::array<const RecoveredWidget*,5>& page,
        std::size_t i) noexcept {
    if(i>=page.size() || !page[0] || !page[i])return 0;
    unsigned changes=0;
    for(std::size_t j=1;j<=i;++j) {
        if(!page[j] || !page[j-1])break;
        if(std::string_view(page[j]->group)!=
           std::string_view(page[j-1]->group))++changes;
    }
    return changes;
}
inline float menu_settings_control_offset(
        const std::array<const RecoveredWidget*,5>& page,
        std::size_t index) noexcept {
    return menu_settings_first_control_offset+
        menu_settings_row_pitch*static_cast<float>(index)+
        menu_settings_section_pitch*
            static_cast<float>(menu_settings_section_changes(page,index));
}
inline float menu_settings_section_text_offset(
        const std::array<const RecoveredWidget*,5>& page,
        std::size_t index) noexcept {
    // 19px heading aligned to bottom, 15px font starts 4px in.
    return menu_settings_control_offset(page,index)-27.0f;
}
inline float menu_settings_next_row_offset(
        const std::array<const RecoveredWidget*,5>& page,
        std::size_t visible) noexcept {
    if(visible==0u)return menu_settings_first_control_offset;
    const std::size_t last=std::min<std::size_t>(visible,5u)-1u;
    return menu_settings_control_offset(page,last)+menu_settings_row_pitch;
}
inline float menu_settings_expanded_extra(
        const std::array<const RecoveredWidget*,5>& page,
        std::size_t visible,bool pagination) noexcept {
    const float n=static_cast<float>(std::min<std::size_t>(visible,5u));
    const unsigned sections=visible?
        menu_settings_section_changes(page,std::min<std::size_t>(visible,5u)-1u):0u;
    return 31.0f+menu_settings_row_pitch*n+
           menu_settings_section_pitch*static_cast<float>(sections)+
           (pagination?menu_settings_pagination_pitch:0.0f);
}
} // namespace slinkywinky
