#pragma once
#include "slinkywinky/menu_render.hpp"
#include <algorithm>
#include <optional>

namespace slinkywinky {

// R128: recovered clickgui.tpl.html shows a centered 600px-wide dark
// confirmation card with three 20px rows, 8px padding, 12px row gaps.
// The preview's Continue action never calls a Minecraft/native unload.
struct MenuUnloadModalRect {
    float x=0,y=0,width=0,height=100.0f;
    float close_x=0,close_y=0;
    float continue_x=0,continue_y=0,continue_width=0;
};
inline std::optional<MenuUnloadModalRect> menu_unload_modal_geometry(
        const MenuRenderFrame& frame,float width,float height) noexcept {
    if(!frame.unload_confirmation_preview || frame.dropdown_preview ||
       width<320.0f || height<240.0f ||
       frame.tab==MenuTab::Count || frame.tab==MenuTab::Unload)
        return std::nullopt;
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
} // namespace slinkywinky
