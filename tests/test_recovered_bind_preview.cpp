#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_bind_preview.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/recovered_settings.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R140 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    CHECK(menu_bind_supported_module(0u));
    CHECK(menu_bind_supported_module(2u));
    CHECK(menu_bind_supported_module(32u));
    CHECK(!menu_bind_supported_module(16u)); // Weapons no binding
    CHECK(!menu_bind_supported_module(57u)); // Friends no binding
    CHECK(!menu_bind_supported_module(65u)); // Teams no binding
    CHECK(!menu_bind_supported_module(67u));
    CHECK(menu_bind_code_label(0u)=="NONE");
    CHECK(menu_bind_code_label(0xA1u)=="RSHIFT");
    CHECK(menu_bind_code_label(0x52u)=="R");
    CHECK(menu_bind_code_label(0x7Bu)=="F12");
    CHECK(menu_bind_code_label(menu_bind_mouse2)=="MOUSE2");
    CHECK(menu_bind_code_label(menu_bind_mouse3)=="MOUSE3");
    CHECK(menu_bind_code_label(menu_bind_mouse4)=="MOUSE4");
    CHECK(menu_bind_code_label(menu_bind_mouse5)=="MOUSE5");
    CHECK(menu_valid_bind_code(0u));
    CHECK(menu_valid_bind_code(0xA1u));
    CHECK(menu_valid_bind_code(menu_bind_mouse5));
    CHECK(!menu_valid_bind_code(0x1Bu)); // ESC clears with code 0
    CHECK(!menu_valid_bind_code(0xFFFFu));
    {
        const MenuState defaults;
        CHECK(defaults.bind_previews()[0].code==0x52u);
        CHECK(defaults.bind_previews()[2].code==menu_bind_mouse4);
        CHECK(defaults.bind_previews()[15].code==0x56u);
        CHECK(defaults.bind_previews()[32].code==0xA1u);
        CHECK(defaults.bind_previews()[53].code==0x43u &&
              defaults.bind_previews()[53].hold);
        CHECK(defaults.bind_previews()[57].code==0u);
        CHECK(defaults.bind_previews()[65].code==0u);
    }
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge profile(menu);
        MenuNavigation nav(menu,profile);
        MenuRenderSession frames;
        CHECK(frames.attach(140,900,630));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto pixel=[&](float x,float y) {
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(panel->scale*x)),
                static_cast<int>(panel->top+std::lround(panel->scale*y))
            };
        };
        auto frame=frames.next(menu,&profile,&nav);
        CHECK(frame && frame->visible[0].catalog_index==0u);
        auto chips=menu_bind_chip_geometry(
            *frame,panel->logical_width(),panel->logical_height(),
            0u,0u,"R",false);
        CHECK(chips && chips->key.width>=41.0f &&
              chips->hold.width==45.0f);
        const auto key= pixel(chips->key.x+chips->key.width*0.5f,
                              chips->key.y+chips->key.height*0.5f);
        const auto hold=pixel(chips->hold.x+chips->hold.width*0.5f,
                              chips->hold.y+chips->hold.height*0.5f);
        const auto keyHit=hit_test_menu(
            *frame,mode,key.first,key.second);
        CHECK(keyHit && keyHit->kind==MenuHitKind::BindChip &&
              keyHit->catalog_index==0u);
        // R141: source HTML's HOLD chip is hidden until the row
        // is hovered, unless HOLD has already been enabled.
        CHECK(!hit_test_menu(*frame,mode,hold.first,hold.second) ||
              hit_test_menu(*frame,mode,hold.first,hold.second)->kind!=
                  MenuHitKind::BindHold);
        CHECK(!menu.set_bind_hover_row(57u));
        CHECK(menu.set_bind_hover_row(0u));
        frame=frames.next(menu,&profile,&nav);
        CHECK(frame && frame->bind_hover_row==0u);
        const auto holdHit=hit_test_menu(
            *frame,mode,hold.first,hold.second);
        CHECK(holdHit && holdHit->kind==MenuHitKind::BindHold &&
              holdHit->catalog_index==0u);
        CHECK(!menu.begin_bind_capture(57u));
        CHECK(!menu.begin_bind_capture(65u));
        CHECK(!menu.begin_bind_capture(67u));
        CHECK(!menu.toggle_bind_hold(16u));
        CHECK(!menu.commit_bind_capture(0x5Au));
        MenuMouseGesture gesture;
        CHECK(!gesture.press(*frame,mode,key.first,key.second,100,false));
        CHECK(gesture.press(*frame,mode,key.first,key.second,200,true));
        const auto released=gesture.release(
            *frame,mode,key.first,key.second,220,true);
        CHECK(released && released->kind==MenuHitKind::BindChip);
        CHECK(menu.begin_bind_capture(released->catalog_index));
        CHECK(!menu.begin_bind_capture(0u));
        CHECK(!menu.toggle_bind_hold(0u));
        CHECK(!menu.commit_bind_capture(0xFFFFu));
        CHECK(menu.bind_capture_index()==0u);
        auto waiting=frames.next(menu,&profile,&nav);
        CHECK(waiting && waiting->bind_capture_index==0u);
        CHECK(menu.commit_bind_capture(0x5Au)); // Z
        CHECK(!menu.bind_capture_index());
        CHECK(menu.bind_previews()[0].code==0x5Au);
        CHECK(menu.toggle_bind_hold(0u));
        CHECK(menu.bind_previews()[0].hold);
        CHECK(menu.set_bind_hover_row(std::nullopt));
        auto persistentHold=frames.next(menu,&profile,&nav);
        const auto remains=hit_test_menu(
            *persistentHold,mode,hold.first,hold.second);
        CHECK(remains && remains->kind==MenuHitKind::BindHold);
        CHECK(!menu.preview_enabled(0u) && !profile.enabled(0u));
        // A prior click cannot commit after binding state changed.
        const auto edited=frames.next(menu,&profile,&nav);
        CHECK(edited && edited->bind_previews[0].hold);
        CHECK(gesture.press(*edited,mode,
              key.first,key.second,1200,true));
        CHECK(menu.toggle_bind_hold(0u));
        CHECK(!menu.bind_previews()[0].hold);
        const auto changed=frames.next(menu,&profile,&nav);
        CHECK(!gesture.release(*changed,mode,
              key.first,key.second,1220,true));
        // Escape clears the key and resets HOLD in one bounded commit.
        CHECK(menu.begin_bind_capture(0u));
        CHECK(menu.commit_bind_capture(0u));
        CHECK(menu.bind_previews()[0].code==0u &&
              !menu.bind_previews()[0].hold);
        CHECK(!menu.toggle_bind_hold(0u));
        CHECK(menu.begin_bind_capture(0u));
        CHECK(menu.commit_bind_capture(menu_bind_mouse5));
        CHECK(menu.bind_previews()[0].code==menu_bind_mouse5);
        CHECK(menu.begin_bind_capture(0u));
        CHECK(menu.cancel_bind_capture()); // focus loss, no rewrite
        CHECK(menu.bind_previews()[0].code==menu_bind_mouse5);
        CHECK(!menu.cancel_bind_capture());
        CHECK(menu.begin_bind_capture(0u));
        CHECK(menu.open_unload_confirmation_preview());
        CHECK(!menu.bind_capture_index());
        CHECK(!menu.begin_bind_capture(0u));
        CHECK(menu.close_unload_confirmation_preview());
        CHECK(menu.select(MenuTab::Move));
        CHECK(!menu.begin_bind_capture(0u));
        CHECK(!menu.toggle_bind_hold(0u));
        CHECK(frames.detach());
    }
    return 0;
}
