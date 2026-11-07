#pragma once
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include <cstddef>
#include <string_view>

namespace slinkywinky {
// No filesystem, profiles loader, actions, game state or injector.
inline bool menu_profile_text_valid(std::string_view text,
                                    std::size_t max_chars) noexcept {
    if(text.empty() || text.size()>max_chars)return false;
    for(const unsigned char c:text)
        if(c<32 || c>126 || c=='/' || c=='\\')return false;
    return true;
}
inline bool menu_profile_preview_valid(const MenuRenderFrame& frame,
                                       float panel_width,
                                       float panel_height) noexcept {
    if(frame.tab!=MenuTab::Profiles)
        return frame.profile_cards.empty() &&
               frame.profile_name_draft.empty() &&
               !frame.profile_name_focused &&
               !frame.profile_name_taken &&
               !frame.profile_action_preview &&
               !frame.profile_bind_capture_index;
    if(!frame.is_placeholder || !frame.visible.empty() ||
       !frame.parameters.empty() || frame.profile_cards.empty() ||
       frame.profile_cards.size()>6)return false;
    if(frame.profile_name_draft.size()>28u)return false;
    for(const unsigned char c:frame.profile_name_draft)
        if(c<32u || c>126u || c=='/' || c=='\\')return false;
    if(frame.profile_action_preview) {
        const auto& modal=*frame.profile_action_preview;
        if(modal.profile_index>=frame.profile_cards.size() ||
           static_cast<unsigned>(modal.action)>
             static_cast<unsigned>(MenuProfileAction::Delete) ||
           (modal.profile_index==0u &&
            (modal.action==MenuProfileAction::Save ||
             modal.action==MenuProfileAction::Delete)) ||
           frame.profile_name_focused || frame.unload_confirmation_preview)
            return false;
    }
    if(frame.profile_bind_capture_index &&
       (*frame.profile_bind_capture_index>=frame.profile_cards.size() ||
        frame.profile_name_focused || frame.profile_action_preview ||
        frame.unload_confirmation_preview))
        return false;
    if(frame.profile_name_focused && frame.profile_cards.size()>=6u)
        return false;
    bool has_default=false;
    for(std::size_t i=0;i<frame.profile_cards.size();++i) {
        const auto& profile=frame.profile_cards[i];
        if(!menu_profile_text_valid(profile.name,28) ||
           ((i==0u || !profile.description.empty()) &&
            !menu_profile_text_valid(profile.description,62)) ||
           (!profile.bind.empty() &&
            !menu_profile_text_valid(profile.bind,8)) ||
           (profile.is_default != (i==0)) ||
           (i==0 && profile.name!="Default"))return false;
        has_default=has_default||profile.is_default;
        const auto rect=menu_profile_card_rect(frame,panel_width,i);
        if(rect.width<130 || rect.height<35 ||
           rect.x<menu_nav_x || rect.y<menu_first_row_y ||
           rect.x+rect.width>panel_width-menu_nav_x+0.1f ||
           rect.y+rect.height>panel_height-12.0f+0.1f)
            return false; // Clipped cards cannot look interactive.
        for(std::size_t j=0;j<i;++j)
            if(frame.profile_cards[j].name==profile.name)return false;
    }
    return has_default;
}
} // namespace slinkywinky
