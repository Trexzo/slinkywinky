#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_color_geometry.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#define CHECK(x) do { if(!(x)) throw std::runtime_error( \
    "R136 line " + std::to_string(__LINE__) + ": " #x); } while(false)
using namespace slinkywinky;

int main() {
    const auto& widgets=recovered_widget_catalog();
    CHECK(menu_parse_color_hex("#ABCDEF")==0xABCDEFu);
    CHECK(menu_parse_color_hex("#8b7cf6")==0x8B7CF6u);
    CHECK(menu_parse_color_hex("#000000")==0u);
    CHECK(menu_parse_color_hex("#ffffff")==0xFFFFFFu);
    CHECK(!menu_parse_color_hex("ffffff"));
    CHECK(!menu_parse_color_hex("#gg0000"));
    CHECK(!menu_parse_color_hex("#abcde"));
    CHECK(!menu_parse_color_hex("#1234567"));
    CHECK(!menu_parse_color_hex("#12 456"));
    CHECK(!menu_source_widget_color(widgets.size()));
    std::size_t colorCount=0,accent=widgets.size(),foreign=widgets.size();
    for(std::size_t i=0;i<widgets.size();++i) {
        if(widgets[i].kind!=RecoveredWidgetKind::Color)continue;
        ++colorCount;
        CHECK(menu_source_widget_color(i).has_value());
        if(std::string_view(widgets[i].module)=="GUI" &&
           std::string_view(widgets[i].label)=="Accent color")
            accent=i;
        if(std::string_view(widgets[i].module)=="Backtrack")
            foreign=i;
    }
    CHECK(colorCount==13u && accent<widgets.size() &&
          foreign<widgets.size());
    CHECK(*menu_source_widget_color(accent)==0x8B7CF6u);
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        CHECK(nav.select_tab(MenuTab::Visual));
        // The GUI module is visual catalog #32 (Visual ordinal 5).
        // R135 makes this source card reachable in a narrow overlay.
        CHECK(profile.scroll_by_visible(5,3));
        CHECK(menu.page().front().catalog_index==32u);
        CHECK(nav.focus_visible_row(32u));
        CHECK(nav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession session;
        CHECK(session.attach(136,900u,630u));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto logicalPoint=[&](float x,float y) {
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        auto frame=session.next(menu,&profile,&nav);
        CHECK(frame && frame->visible[0].catalog_index==32u &&
              frame->visible[0].expanded);
        const auto box=menu_color_swatch_geometry(
            *frame,panel->logical_width(),panel->logical_height(),
            32u,accent);
        CHECK(box && box->width==42.0f && box->height==22.0f);
        const auto inside=logicalPoint(box->x+21.0f,box->y+11.0f);
        const auto before=hit_test_menu(*frame,mode,
                                        inside.first,inside.second);
        CHECK(before && before->kind==MenuHitKind::RecoveredColorSwatch &&
              before->catalog_index==32u &&
              before->control_index==accent);
        const auto label=logicalPoint(box->x-52.0f,box->y+11.0f);
        CHECK(!hit_test_menu(*frame,mode,label.first,label.second));
        CHECK(!menu_color_swatch_geometry(
            *frame,panel->logical_width(),panel->logical_height(),
            32u,foreign));
        CHECK(!menu.set_recovered_color_preview(
            32u,foreign,0x112233u));
        CHECK(!menu.set_recovered_color_preview(
            31u,accent,0x112233u));
        CHECK(!menu.set_recovered_color_preview(
            32u,widgets.size(),0x112233u));
        CHECK(!menu.set_recovered_color_preview(
            32u,accent,0x1000000u));
        CHECK(!menu.recovered_color_override(accent));
        MenuMouseGesture gesture;
        CHECK(!gesture.press(*frame,mode,
             inside.first,inside.second,200,false));
        CHECK(gesture.press(*frame,mode,
             inside.first,inside.second,300,true));
        const auto committed=gesture.release(*frame,mode,
             inside.first,inside.second,320,true);
        CHECK(committed && committed->kind==MenuHitKind::RecoveredColorSwatch);
        CHECK(menu.set_recovered_color_preview(
             committed->catalog_index,committed->control_index,0x12ABEFu));
        CHECK(menu.recovered_color_override(accent)==0x12ABEFu);
        CHECK(menu.appearance().accent_rgb==0xF35D12u);
        CHECK(!menu.preview_enabled(32u) && !profile.enabled(32u));
        const auto updated=session.next(menu,&profile,&nav);
        CHECK(updated && updated->recovered_color_overrides[accent]==0x12ABEFu);
        CHECK(gesture.press(*updated,mode,
             inside.first,inside.second,1000,true));
        CHECK(menu.set_recovered_color_preview(32u,accent,0x135724u));
        const auto changed=session.next(menu,&profile,&nav);
        CHECK(!gesture.release(*changed,mode,
             inside.first,inside.second,1020,true));
        CHECK(changed->recovered_color_overrides[accent]==0x135724u);
        CHECK(menu.open_unload_confirmation_preview());
        CHECK(!menu.set_recovered_color_preview(32u,accent,0xAA5500u));
        CHECK(menu.close_unload_confirmation_preview());
        CHECK(menu.select(MenuTab::Move));
        CHECK(!menu.set_recovered_color_preview(32u,accent,0xAA5500u));
        CHECK(session.detach());
    }
    return 0;
}
