#pragma once
#include <algorithm>
#include <cmath>
namespace slinkywinky {
// Candidate ClickGUI CSS: .in padding:8 and control left:50% right:0.
struct MenuSettingColumn {float x=0.0f,width=0.0f;};
inline MenuSettingColumn menu_setting_column(float x,float width) noexcept {
    if(!std::isfinite(x) || !std::isfinite(width) || width<16.0f)
        return {};
    const float inner=width-16.0f;
    return {x+8.0f+inner*0.5f,inner*0.5f};
}
inline float menu_setting_switch_x(MenuSettingColumn c) noexcept {
    return c.x+c.width-40.0f;
}
inline constexpr float menu_setting_row_height=20.0f;
inline constexpr float menu_setting_switch_width=40.0f;
inline constexpr float menu_setting_control_font=15.0f;
}
