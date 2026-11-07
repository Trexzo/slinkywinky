#pragma once
#include "slinkywinky/recovered_settings.hpp"
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>

namespace slinkywinky {

// R147: 115 numeric format strings from the user-supplied candidate
// re/mods_block.js. These are NOT authenticated original native code.
// The 119 GUI callsites comprise these 115 plus two additional Aim Assist
// Multi-point sliders and two repeated Aim speed display branches.
struct MenuNumericFormatEntry {
    std::string_view module;
    std::string_view label;
    std::string_view format;
};
inline constexpr std::array<MenuNumericFormatEntry,115>
menu_numeric_formats{{
    {"Aim Assist","Horizontal speed","%.1f"},
    {"Aim Assist","Vertical speed","%.1f"},
    {"Aim Assist","Randomization","%.0f%%"},
    {"Aim Assist","Predict","%.0f%%"},
    {"Aim Assist","Range","%0.1f - %0.1f blocks"},
    {"Aim Assist","FOV","%0.0f - %0.0f"},
    {"Auto Clicker","Target CPS","%0.1f CPS"},
    {"Reach","Distance","%0.2f blocks"},
    {"Reach","Activate for","%.0f tick(s)"},
    {"Velocity","Chance","%.0f%%"},
    {"Velocity","Reduce to","%.0f%%"},
    {"Velocity","Reduce delay","%.0f tick(s)"},
    {"Velocity","FOV","%.0f"},
    {"Auto Block","Maximum hurt time","%.0f ms"},
    {"Auto Block","Range","%.1f blocks"},
    {"Auto Block","Maximum hold duration","%.0f ms"},
    {"Auto Block","Chance","%.0f %%"},
    {"Auto Block","Maximum duration","%.0f ms"},
    {"Criticals","Chance","%.0f%%"},
    {"Criticals","Maximum delay","%.0f ms"},
    {"Criticals","Timer speed","%.2fx"},
    {"Hit Select","Cancel rate","%.0f%%"},
    {"Hit Select","Pause duration","%.0f ms"},
    {"Backtrack","Maximum delay","%.0f ms"},
    {"Backtrack","Target distance","%.1f - %.1f blocks"},
    {"Backtrack","Cooldown","%.1f s"},
    {"Backtrack","Line width","%0.0f px"},
    {"Backtrack","Maximum hurt time","%.0f ms"},
    {"Hitboxes","Expand","%0.2f blocks"},
    {"Keep Sprint","Chance","%.0f%%"},
    {"Keep Sprint","Retained speed","%.0f%%"},
    {"Sprint Reset","Delay","%.0f ms"},
    {"Sprint Reset","Stop duration","%.0f ms"},
    {"Lag Range","Maximum delay","%.0f ms"},
    {"Lag Range","Line width","%0.0f px"},
    {"Knockback Delay","Chance","%.0f%%"},
    {"Knockback Delay","Maximum delay","%.0f ms"},
    {"Knockback Delay","Distance to target","%.1f blocks"},
    {"Knockback Displace","Angle","%.0f"},
    {"Knockback Displace","Cooldown","%.0f ms"},
    {"Auto Weapon","Activation time","%.0f ms"},
    {"Auto Rod","Throw speed","%.1f CPS"},
    {"Auto Rod","Retract speed","%.2fx"},
    {"Auto Rod","Delay between actions","%.0f ms"},
    {"Timer Range","Duration","%0.0f ms"},
    {"Timer Range","Minimum distance","%0.2f blocks"},
    {"Timer Range","Cooldown","%0.1f s"},
    {"Fake Lag","Inbound","%.0f ms"},
    {"Fake Lag","Outbound","%.0f ms"},
    {"Strafe","Air","%.0f%%"},
    {"Strafe","Jump","%.0f%%"},
    {"Strafe","Ground","%.0f%%"},
    {"Strafe","Maximum hurt time","%.0f ms"},
    {"Fast Accel","Boost","%.0f%%"},
    {"No Slow","Bow","%.0f%%"},
    {"No Slow","Sword","%.0f%%"},
    {"No Slow","Consumables","%.0f%%"},
    {"Inv Walk","Stop duration","%.0f ms"},
    {"Safe Walk","Motion","%.2fx"},
    {"Timer","Speed","%0.2fx"},
    {"Blink","Maximum duration","%.2f s"},
    {"Player ESP","Range","%0.0f blocks"},
    {"Player ESP","Line width","%0.0f px"},
    {"Chest ESP","Range","%0.0f blocks"},
    {"Chest ESP","Line width","%0.0f px"},
    {"Block ESP","Range","%0.0f chunk(s)"},
    {"Block ESP","Limit per chunk section","%0.0f block(s)"},
    {"Block ESP","Line width","%0.0f px"},
    {"Item ESP","Range","%0.0f blocks"},
    {"Name Tags","Scale","%0.0f %%"},
    {"Name Tags","Range","%0.0f blocks"},
    {"Pointers","Scale","%.0f%%"},
    {"Pointers","Radius","%.0f"},
    {"Pointers","Range","%0.0f blocks"},
    {"Pointers","Ignore within FOV","%0.0f"},
    {"Pointers","Transition between","%0.0f - %0.0f blocks"},
    {"Indicators","Scale","%.0f%%"},
    {"Indicators","Radius","%.0f"},
    {"Trajectories","Line width","%0.0f px"},
    {"Mod Overlay","Text size","%.0f px"},
    {"Mod Overlay","Rounding","%.0f px"},
    {"Mod Overlay","Padding X","%.0f px"},
    {"Mod Overlay","Padding Y","%.0f px"},
    {"Fast Place","Maximum CPS","%0.1f CPS"},
    {"Fast Place","Activate delay","%0.0f ms"},
    {"Bridge Assist","Edge offset","%.2f blocks"},
    {"Bridge Assist","Unsneak delay","%.0f ms"},
    {"Fast Mine","Speed","%.2fx"},
    {"Fast Mine","Break delay","%.0f ms"},
    {"Auto Tool","Activation time","%.0f ms"},
    {"Clutch","Click speed","%.1f CPS"},
    {"Clutch","Duration","%.0f ms"},
    {"Clutch","Base speed","%.0f /s"},
    {"Clutch","Acceleration","%.3fx"},
    {"Clutch","Randomization","%.0f%%"},
    {"Clutch","Delay","%.0f ms"},
    {"Clutch","FOV","%.0f"},
    {"Clutch","Minimum height","%.0f block(s)"},
    {"Block In","Speed","%.0f%%"},
    {"Anti Bot","Minimum entity age","%.0f s"},
    {"Inv Manager","Speed","%.1fx"},
    {"Inv Manager","Delay after open","%.0f ms"},
    {"Chest Stealer","Speed","%.1fx"},
    {"Chest Stealer","Delay after open","%.0f ms"},
    {"Chest Stealer","Delay before close","%.0f ms"},
    {"Refill","Speed","%.1fx"},
    {"Refill","Delay after open","%.0f ms"},
    {"Refill","Delay before close","%.0f ms"},
    {"Auto Pot","Delay between actions","%.0f ms"},
    {"Auto Soup","Single use","%.1f"},
    {"Auto Soup","Double use","%.1f"},
    {"Auto Soup","Before switching slots","%.0f ms"},
    {"Auto Soup","Between actions","%.0f ms"},
    {"Auto Soup","Cooldown","%.0f ms"},
    {"Auto Soup","Randomization","%.0f%%"}
}};

inline std::string_view menu_numeric_format_for_widget(
        std::size_t index) noexcept {
    const auto& widgets=recovered_widget_catalog();
    if(index>=widgets.size())return {};
    const auto& w=widgets[index];
    if(w.kind!=RecoveredWidgetKind::Slider &&
       w.kind!=RecoveredWidgetKind::Range)return {};
    for(const auto& rule:menu_numeric_formats)
        if(rule.module==w.module && rule.label==w.label)
            return rule.format;
    return {}; // Multi-point numeric fixture has no HTML format.
}

// Format exact candidate %0.1f, %.0f and %% syntax using a validated
// parser, *never* a source-controlled printf format string.
inline std::optional<std::string> menu_format_numeric_values(
        std::string_view format,float first,
        std::optional<float> second={}) {
    if(format.empty() || format.size()>64u ||
       !std::isfinite(first) || (second && !std::isfinite(*second)))
        return std::nullopt;
    std::string result;
    result.reserve(64u);
    unsigned values=0u;
    for(std::size_t i=0;i<format.size();) {
        if(format[i]!='%') {
            result.push_back(format[i++]);
            continue;
        }
        ++i;
        if(i>=format.size())return std::nullopt;
        if(format[i]=='%') {
            result.push_back('%');++i;continue;
        }
        if(format[i]=='0')++i;
        if(i>=format.size() || format[i]!='.')return std::nullopt;
        ++i;
        if(i>=format.size() || format[i]<'0' || format[i]>'3')
            return std::nullopt;
        const int decimals=format[i++]-'0';
        if(i>=format.size() || format[i++]!='f')return std::nullopt;
        if(values>=2u || (values==1u && !second))return std::nullopt;
        const double value=values++==0u?
            static_cast<double>(first):static_cast<double>(*second);
        char number[48]{};
        const int used=std::snprintf(
            number,sizeof(number),"%.*f",decimals,value);
        if(used<0 || static_cast<std::size_t>(used)>=sizeof(number))
            return std::nullopt;
        result+=number;
        if(result.size()>96u)return std::nullopt;
    }
    if(values!=(second?2u:1u) || result.size()>96u)
        return std::nullopt;
    return result;
}
} // namespace slinkywinky
