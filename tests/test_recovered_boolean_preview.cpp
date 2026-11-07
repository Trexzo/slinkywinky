#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_setting_layout.hpp"
#include "slinkywinky/menu_settings_vertical.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R130 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    const auto& catalog=recovered_widget_catalog();
    std::size_t typedBools=0;
    for(const auto& widget:catalog)
        if(widget.kind==RecoveredWidgetKind::Boolean)++typedBools;
    CHECK(typedBools>=190u);
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        MenuRenderSession frames;
        CHECK(frames.attach(130,900,630));
        // Auto Clicker has a mixed dropdown/slider/Boolean first page.
        CHECK(nav.focus_visible_row(2u));
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto xy=[&](float x,float y) {
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        auto frame=frames.next(menu,&profile,&nav);
        CHECK(frame && frame->visible[2].catalog_index==2u &&
              frame->visible[2].expanded && !frame->dropdown_preview);
        const auto rect=menu_module_card_rect(
            *frame,panel->logical_width(),2u);
        CHECK(rect.y+rect.height<
              panel->logical_height()-menu_panel_padding);
        std::array<const RecoveredWidget*,5> widgets{};
        const auto count=recovered_widget_visible_page(
            "Auto Clicker",0,0,0,widgets);
        CHECK(count==9u && widgets[2]);
        CHECK(widgets[2]->kind==RecoveredWidgetKind::Boolean);
        const auto idx=static_cast<std::size_t>(widgets[2]-catalog.data());
        CHECK(idx<catalog.size());
        CHECK(!menu.recovered_boolean_override(idx));
        const bool sourceExample=widgets[2]->example_enabled;
        // These coordinates intentionally mirror the actual GL drawing:
        // right-aligned 40x20 switch, third recovered setting row.
        const auto column=menu_setting_column(rect.x,rect.width);
        const float controlW=column.width;
        const float controlX=column.x;
        const float y=rect.y+menu_row_body_height(*frame)+
                      menu_settings_control_offset(widgets,2u);
        const auto onSwitch=xy(controlX+controlW-17.0f,y+10.0f);
        const auto onLabel=xy(rect.x+16.0f,y+10.0f);
        const auto hit=hit_test_menu(
            *frame,mode,onSwitch.first,onSwitch.second);
        CHECK(hit && hit->kind==MenuHitKind::RecoveredBooleanPreview &&
              hit->catalog_index==2u && hit->control_index==idx);
        CHECK(!hit_test_menu(*frame,mode,onLabel.first,onLabel.second));
        CHECK(!menu.toggle_recovered_boolean_preview(2u,catalog.size()));
        CHECK(!menu.toggle_recovered_boolean_preview(0u,idx));
        // Last-page controls may not be edited from the first page.
        std::size_t lastBool=catalog.size();
        for(std::size_t i=0;i<catalog.size();++i)
            if(std::string_view(catalog[i].module)=="Auto Clicker" &&
               std::string_view(catalog[i].label)=="Not using item")
                lastBool=i;
        CHECK(lastBool<catalog.size() &&
              lastBool!=idx &&
              !menu.toggle_recovered_boolean_preview(2u,lastBool));
        MenuMouseGesture gesture;
        CHECK(!gesture.press(*frame,mode,onSwitch.first,onSwitch.second,
                             100,false));
        CHECK(gesture.press(*frame,mode,onSwitch.first,onSwitch.second,
                            200,true));
        const auto committed=gesture.release(*frame,mode,
                   onSwitch.first,onSwitch.second,220,true);
        CHECK(committed &&
              committed->kind==MenuHitKind::RecoveredBooleanPreview);
        CHECK(menu.toggle_recovered_boolean_preview(
            committed->catalog_index,committed->control_index));
        CHECK(menu.recovered_boolean_override(idx) &&
              *menu.recovered_boolean_override(idx)==!sourceExample);
        const auto changed=frames.next(menu,&profile,&nav);
        CHECK(changed &&
              changed->recovered_boolean_overrides[idx].has_value() &&
              *changed->recovered_boolean_overrides[idx]==!sourceExample);
        // A button held during an independent preview edit is invalidated.
        CHECK(gesture.press(*changed,mode,
                            onSwitch.first,onSwitch.second,1000,true));
        CHECK(menu.toggle_recovered_boolean_preview(2u,idx));
        const auto stale=frames.next(menu,&profile,&nav);
        CHECK(stale && !gesture.release(*stale,mode,
                   onSwitch.first,onSwitch.second,1020,true));
        CHECK(menu.recovered_boolean_override(idx) &&
              *menu.recovered_boolean_override(idx)==sourceExample);
        // No editing while popup/modal owns the input surface.
        CHECK(menu.change_settings_page(2u,1));
        CHECK(!menu.toggle_recovered_boolean_preview(2u,idx));
        CHECK(menu.open_unload_confirmation_preview());
        CHECK(!menu.toggle_recovered_boolean_preview(2u,lastBool));
        CHECK(menu.close_unload_confirmation_preview());
        CHECK(frames.detach());
    }
    // A branch-hidden Aim Assist switch is not editable merely because
    // it exists in the 385-record fixture. Switching to Lock-on reveals
    // the exact recovered callsite on the first five-control page.
    MenuState aim;
    MenuProfileBridge aimProfile(aim);
    MenuNavigation aimNav(aim,aimProfile);
    CHECK(aimNav.apply(MenuNavAction::ExpandFocused));
    std::size_t vertical=catalog.size();
    for(std::size_t i=0;i<catalog.size();++i)
        if(std::string_view(catalog[i].module)=="Aim Assist" &&
           std::string_view(catalog[i].group)=="Aim" &&
           std::string_view(catalog[i].label)=="Vertical" &&
           catalog[i].visibility==RecoveredWidgetVisibility::AimModeLockOn)
            vertical=i;
    CHECK(vertical<catalog.size());
    CHECK(!aim.toggle_recovered_boolean_preview(0u,vertical));
    CHECK(aim.set_aim_preview_mode(2u)); // Lock-on
    CHECK(aim.toggle_recovered_boolean_preview(0u,vertical));
    CHECK(aim.recovered_boolean_override(vertical).has_value());
    CHECK(aim.set_aim_preview_mode(0u)); // conditional control is hidden
    CHECK(!aim.toggle_recovered_boolean_preview(0u,vertical));
    // Existing Piercing settings are deliberately routed to their own
    // preview state and cannot be edited by the generic Boolean pathway.
    MenuState piercing;
    MenuProfileBridge profile(piercing);
    MenuNavigation nav(piercing,profile);
    CHECK(profile.scroll_by(9));
    CHECK(nav.focus_visible_row(11u));
    CHECK(nav.apply(MenuNavAction::ExpandFocused));
    std::size_t pierceId=catalog.size();
    for(std::size_t i=0;i<catalog.size();++i)
        if(std::string_view(catalog[i].module)=="Piercing" &&
           catalog[i].kind==RecoveredWidgetKind::Boolean)
            {pierceId=i;break;}
    CHECK(pierceId<catalog.size() &&
          !piercing.toggle_recovered_boolean_preview(11u,pierceId));
    return 0;
}
