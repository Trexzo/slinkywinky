#pragma once
#include "slinkywinky/menu_profile_preview.hpp"
#include "slinkywinky/menu_unload_modal_geometry.hpp"
#include <algorithm>
#include <optional>
#include <string>

namespace slinkywinky {

// Shared by GL11 and all three hit-test panel modes. No OS/file actions.
struct MenuProfileControlRect {
    float x=0,y=0,width=0,height=0;
};
inline bool menu_profile_contains(const MenuProfileControlRect& rect,
                                  float x,float y) noexcept {
    return x>=rect.x && x<rect.x+rect.width &&
           y>=rect.y && y<rect.y+rect.height;
}
inline std::optional<MenuProfileControlRect> menu_profile_bind_rect(
        const MenuRenderFrame& frame,float width,float height,
        std::size_t profile_index) noexcept {
    if(!menu_profile_preview_valid(frame,width,height) ||
       frame.profile_action_preview || frame.unload_confirmation_preview ||
       profile_index>=frame.profile_cards.size())
        return std::nullopt;
    const auto card=menu_profile_card_rect(frame,width,profile_index);
    const float nameX=card.x+8.0f;
    const float iconX=card.x+card.width-124.0f;
    const float titleWidth=std::max(30.0f,iconX-nameX-45.0f);
    const float titleW=std::min(titleWidth,
        static_cast<float>(frame.profile_cards[profile_index].name.size())*
        9.0f);
    const float chipX=std::min(iconX-46.0f,nameX+titleW+5.0f);
    const bool capturing=frame.profile_bind_capture_index==profile_index;
    const std::size_t chars=capturing?14u:
        (frame.profile_cards[profile_index].bind.empty()?4u:
         frame.profile_cards[profile_index].bind.size());
    const float chipWidth=std::min(iconX-chipX-5.0f,
        std::max(41.0f,static_cast<float>(chars)*7.0f+12.0f));
    if(chipX<nameX+15.0f || chipWidth<41.0f ||
       chipX+chipWidth>iconX-4.0f)return std::nullopt;
    return MenuProfileControlRect{
        chipX,card.y+8.0f,chipWidth,20.0f
    };
}
inline std::optional<MenuProfileControlRect> menu_profile_action_rect(
        const MenuRenderFrame& frame,float width,float height,
        std::size_t profile_index,unsigned action) noexcept {
    if(!menu_profile_preview_valid(frame,width,height) ||
       frame.profile_action_preview || frame.unload_confirmation_preview ||
       action>3u || profile_index>=frame.profile_cards.size() ||
       (profile_index==0u && (action==1u || action==3u)))
        return std::nullopt;
    const auto card=menu_profile_card_rect(frame,width,profile_index);
    const float x=card.x+card.width-124.0f+
                  static_cast<float>(action)*32.0f;
    const float y=card.y+10.0f;
    if(x<card.x || x+20.0f>card.x+card.width ||
       y+20.0f>card.y+card.height)return std::nullopt;
    return MenuProfileControlRect{x,y,20.0f,20.0f};
}
inline std::optional<MenuProfileControlRect> menu_profile_input_rect(
        const MenuRenderFrame& frame,float width,float height) noexcept {
    if(!menu_profile_preview_valid(frame,width,height) ||
       frame.profile_action_preview || frame.unload_confirmation_preview ||
       frame.profile_cards.size()>=6u)return std::nullopt;
    auto rect=menu_profile_card_rect(
        frame,width,frame.profile_cards.size());
    rect.height=68.0f; // two 20px rows, 12px gap and 8px padding
    if(rect.y+rect.height>height-menu_panel_padding)
        return std::nullopt;
    return MenuProfileControlRect{
        rect.x+8.0f,rect.y+8.0f,rect.width-16.0f,20.0f
    };
}
inline std::optional<MenuProfileControlRect> menu_profile_create_rect(
        const MenuRenderFrame& frame,float width,float height) noexcept {
    const auto input=menu_profile_input_rect(frame,width,height);
    if(!input)return std::nullopt;
    return MenuProfileControlRect{
        input->x,input->y+32.0f,input->width,20.0f
    };
}
inline std::optional<MenuUnloadModalRect> menu_profile_modal_geometry(
        const MenuRenderFrame& frame,float width,float height) noexcept {
    if(!frame.profile_action_preview ||
       frame.unload_confirmation_preview ||
       !menu_profile_preview_valid(frame,width,height) ||
       width<320.0f || height<240.0f)return std::nullopt;
    MenuUnloadModalRect result;
    result.width=std::min(600.0f,width-16.0f);
    result.x=(width-result.width)*0.5f;
    result.y=(height-result.height)*0.5f;
    result.close_x=result.x+result.width-28.0f;
    result.close_y=result.y+8.0f;
    result.continue_x=result.x+8.0f;
    result.continue_y=result.y+72.0f;
    result.continue_width=result.width-16.0f;
    return result;
}
inline std::string menu_profile_action_question(
        const MenuRenderFrame& frame) {
    if(!frame.profile_action_preview ||
       frame.profile_action_preview->profile_index>=
           frame.profile_cards.size())return {};
    const auto& action=*frame.profile_action_preview;
    const auto& name=frame.profile_cards[action.profile_index].name;
    switch(action.action) {
        case MenuProfileAction::Load:
            return "Load profile '"+name+"'?";
        case MenuProfileAction::Save:
            return "Save profile '"+name+"'?";
        case MenuProfileAction::OpenFolder:
            return "Open folder for profile '"+name+"'?";
        case MenuProfileAction::Delete:
            return "Delete profile '"+name+"'?";
        default:return {};
    }
}
} // namespace slinkywinky
