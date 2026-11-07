#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_profile_preview.hpp"
#include "slinkywinky/menu_profile_interaction_geometry.hpp"
#include "slinkywinky/menu_render.hpp"
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R145 line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    CHECK(menu_profile_icon_tooltip(0u,0u)=="Click to load");
    CHECK(menu_profile_icon_tooltip(0u,1u)==
          "Default profile cannot be overwritten");
    CHECK(menu_profile_icon_tooltip(0u,2u)=="Click to open folder");
    CHECK(menu_profile_icon_tooltip(0u,3u)==
          "Default profile cannot be deleted");
    CHECK(menu_profile_icon_tooltip(1u,1u)=="Click to save");
    CHECK(menu_profile_icon_tooltip(1u,3u)=="Click to delete");
    CHECK(menu_profile_icon_tooltip(1u,4u).empty());

    MenuState invalid;
    CHECK(!invalid.set_profile_icon_hover(MenuProfileIconHover{0u,0u}));
    CHECK(!invalid.profile_icon_hover());

    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge bridge(menu);
        MenuNavigation nav(menu,bridge);
        CHECK(nav.select_tab(MenuTab::Profiles));
        MenuRenderSession render;
        CHECK(render.attach(145u,900u,630u));
        const auto panel=menu_panel_geometry(900u,630u,false,mode);
        CHECK(panel);
        const auto coords=[&](float x,float y) {
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(x*panel->scale)),
                static_cast<int>(panel->top+std::lround(y*panel->scale))
            };
        };
        auto frame=render.next(menu,&bridge,&nav);
        CHECK(frame && menu_profile_preview_valid(
            *frame,panel->logical_width(),panel->logical_height()));
        CHECK(!frame->profile_icon_hover);
        CHECK(!menu.set_profile_icon_hover(
            MenuProfileIconHover{999u,1u}));
        CHECK(!menu.set_profile_icon_hover(
            MenuProfileIconHover{0u,4u}));
        for(unsigned action=0u;action<4u;++action) {
            CHECK(menu.set_profile_icon_hover(
                MenuProfileIconHover{0u,action}));
            frame=render.next(menu,&bridge,&nav);
            CHECK(frame && frame->profile_icon_hover &&
                  frame->profile_icon_hover->profile_index==0u &&
                  frame->profile_icon_hover->action==action);
            const auto icon=menu_profile_icon_visual_rect(
                *frame,panel->logical_width(),
                panel->logical_height(),0u,action);
            CHECK(icon && icon->width==20.0f && icon->height==20.0f);
            const auto tooltip=menu_profile_tooltip_rect(
                *frame,panel->logical_width(),panel->logical_height());
            CHECK(tooltip && tooltip->height==20.0f);
            CHECK(tooltip->y>=8.0f &&
                  tooltip->y+tooltip->height<
                    panel->logical_height());
            CHECK(tooltip->x>=8.0f &&
                  tooltip->x+tooltip->width<=panel->logical_width()-8.0f);
            CHECK(tooltip->y<icon->y);
            const auto pixel=coords(icon->x+10.0f,icon->y+10.0f);
            const auto hit=hit_test_menu(
                *frame,mode,pixel.first,pixel.second);
            if(action==1u || action==3u) {
                // Default Save/Delete are presented but not interactive.
                CHECK(!menu_profile_action_rect(
                    *frame,panel->logical_width(),
                    panel->logical_height(),0u,action));
                CHECK(!hit || hit->kind!=MenuHitKind::ProfileAction);
            } else {
                CHECK(menu_profile_action_rect(
                    *frame,panel->logical_width(),
                    panel->logical_height(),0u,action));
                CHECK(hit && hit->kind==MenuHitKind::ProfileAction &&
                      hit->catalog_index==0u &&
                      hit->control_index==action);
            }
        }
        CHECK(menu.set_profile_icon_hover(std::nullopt));
        CHECK(!menu.profile_icon_hover());
        frame=render.next(menu,&bridge,&nav);
        CHECK(!menu_profile_tooltip_rect(*frame,
            panel->logical_width(),panel->logical_height()));
        CHECK(menu.set_profile_icon_hover(MenuProfileIconHover{0u,0u}));
        frame=render.next(menu,&bridge,&nav);
        const auto enabled=menu_profile_icon_visual_rect(
            *frame,panel->logical_width(),panel->logical_height(),0u,0u);
        CHECK(enabled);
        const auto pixel=coords(enabled->x+10.0f,enabled->y+10.0f);
        MenuMouseGesture gesture;
        CHECK(gesture.press(*frame,mode,pixel.first,pixel.second,
                            1000u,true));
        CHECK(menu.set_profile_icon_hover(MenuProfileIconHover{0u,2u}));
        auto changed=render.next(menu,&bridge,&nav);
        CHECK(!gesture.release(*changed,mode,
            pixel.first,pixel.second,1020u,true));
        CHECK(menu.set_profile_icon_hover(MenuProfileIconHover{0u,0u}));
        CHECK(menu.begin_profile_action_preview(0u,MenuProfileAction::Load));
        CHECK(!menu.profile_icon_hover());
        changed=render.next(menu,&bridge,&nav);
        CHECK(changed && changed->profile_action_preview &&
              !changed->profile_icon_hover);
        CHECK(!menu_profile_tooltip_rect(*changed,
            panel->logical_width(),panel->logical_height()));
        CHECK(!menu.set_profile_icon_hover(MenuProfileIconHover{0u,0u}));
        CHECK(menu.cancel_profile_action_preview());
        CHECK(menu.set_profile_icon_hover(MenuProfileIconHover{0u,1u}));
        CHECK(nav.select_tab(MenuTab::Combat));
        CHECK(!menu.profile_icon_hover());
        CHECK(!menu.set_profile_icon_hover(MenuProfileIconHover{0u,1u}));
        CHECK(!menu.preview_enabled(0u) && !bridge.enabled(0u));
        CHECK(render.detach());
    }
    return 0;
}
