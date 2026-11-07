#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_profile_interaction_geometry.hpp"
#include "slinkywinky/menu_bind_preview.hpp"
#include "slinkywinky/menu_profile_preview.hpp"
#include <array>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R142-R143 profile line "+std::to_string(__LINE__)+": " #x);}while(false)
using namespace slinkywinky;

int main() {
    MenuState blank;
    CHECK(!blank.focus_profile_name_preview());
    CHECK(!blank.create_profile_preview());
    CHECK(!blank.begin_profile_action_preview(0,MenuProfileAction::Load));
    CHECK(!blank.profile_action_preview());
    CHECK(blank.local_profiles().size()==1u &&
          blank.local_profiles()[0].name=="Default");
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState menu;
        MenuProfileBridge bridge(menu);
        MenuNavigation nav(menu,bridge);
        CHECK(nav.select_tab(MenuTab::Profiles));
        MenuRenderSession frames;
        CHECK(frames.attach(142,900,630));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto pixel=[&](float x,float y) {
            return std::pair<int,int>{
                static_cast<int>(panel->left+std::lround(x*panel->scale)),
                static_cast<int>(panel->top+std::lround(y*panel->scale))
            };
        };
        auto frame=frames.next(menu,&bridge,&nav);
        CHECK(frame && menu_profile_preview_valid(
            *frame,panel->logical_width(),panel->logical_height()));
        CHECK(frame->is_placeholder && frame->profile_cards.size()==1u);
        const auto defaultCard=frame->profile_cards[0];
        CHECK(defaultCard.is_default && defaultCard.name=="Default");
        CHECK(defaultCard.description==
              "Contains the default configuration.");
        CHECK(!menu_profile_action_rect(*frame,panel->logical_width(),
              panel->logical_height(),0u,1u));
        CHECK(!menu_profile_action_rect(*frame,panel->logical_width(),
              panel->logical_height(),0u,3u));
        const auto firstInput=menu_profile_input_rect(
            *frame,panel->logical_width(),panel->logical_height());
        const auto firstCreate=menu_profile_create_rect(
            *frame,panel->logical_width(),panel->logical_height());
        CHECK(firstInput && firstCreate);
        CHECK(firstCreate->y==firstInput->y+32.0f);
        const auto inside=pixel(firstInput->x+20.0f,
                                firstInput->y+10.0f);
        const auto inputHit=hit_test_menu(
            *frame,mode,inside.first,inside.second);
        CHECK(inputHit && inputHit->kind==MenuHitKind::ProfileNameField);
        CHECK(menu.focus_profile_name_preview());
        CHECK(!menu.focus_profile_name_preview());
        CHECK(!menu.append_profile_name_preview("/"));
        CHECK(!menu.append_profile_name_preview("\\"));
        CHECK(!menu.append_profile_name_preview("\xC3\xA4"));
        CHECK(!menu.append_profile_name_preview("\n"));
        CHECK(menu.append_profile_name_preview(" "));
        CHECK(!menu.create_profile_preview()); // trim-to-empty
        CHECK(menu.erase_profile_name_preview());
        for(const char c:std::string("Default")) {
            const char v[1]{c};
            CHECK(menu.append_profile_name_preview(std::string_view(v,1u)));
        }
        CHECK(!menu.create_profile_preview()); // duplicate
        CHECK(menu.profile_name_taken());
        CHECK(menu.profile_name_draft().empty());
        CHECK(menu.profile_name_focused());
        const auto duplicateFrame=frames.next(menu,&bridge,&nav);
        CHECK(duplicateFrame && duplicateFrame->profile_name_taken);
        for(const char c:std::string("  legit  ")) {
            const char v[1]{c};
            CHECK(menu.append_profile_name_preview(std::string_view(v,1u)));
        }
        CHECK(!menu.profile_name_taken());
        CHECK(menu.create_profile_preview());
        CHECK(!menu.profile_name_focused());
        CHECK(menu.local_profiles().size()==2u);
        CHECK(menu.local_profiles()[1].name=="legit");
        CHECK(menu.local_profiles()[1].description.empty());
        CHECK(!menu.profile_name_draft().size());
        auto updated=frames.next(menu,&bridge,&nav);
        CHECK(updated && updated->profile_cards.size()==2u);
        CHECK(menu_profile_preview_valid(*updated,
              panel->logical_width(),panel->logical_height()));
        const auto profileChip=menu_profile_bind_rect(*updated,
            panel->logical_width(),panel->logical_height(),1u);
        CHECK(profileChip && profileChip->width>=41.0f);
        const auto chipXY=pixel(profileChip->x+profileChip->width*0.5f,
                                profileChip->y+10.0f);
        const auto chipHit=hit_test_menu(
            *updated,mode,chipXY.first,chipXY.second);
        CHECK(chipHit && chipHit->kind==MenuHitKind::ProfileBindChip &&
              chipHit->catalog_index==1u);
        MenuMouseGesture gesture;
        CHECK(gesture.press(*updated,mode,chipXY.first,chipXY.second,
                            400,true));
        const auto held=gesture.release(
            *updated,mode,chipXY.first,chipXY.second,420,true);
        CHECK(held && held->kind==MenuHitKind::ProfileBindChip);
        CHECK(menu.begin_profile_bind_capture(held->catalog_index));
        CHECK(!menu.begin_profile_bind_capture(0u));
        CHECK(!menu.focus_profile_name_preview());
        CHECK(!menu.commit_profile_bind_capture(0xFFFFu));
        CHECK(!menu.commit_profile_bind_capture(0x08u)); // long label
        auto bindFrame=frames.next(menu,&bridge,&nav);
        CHECK(bindFrame && bindFrame->profile_bind_capture_index==1u);
        CHECK(menu.commit_profile_bind_capture(0x75u)); // F6
        CHECK(menu.local_profiles()[1].bind=="F6");
        CHECK(!menu.profile_bind_capture_index());
        CHECK(menu.begin_profile_bind_capture(1u));
        CHECK(menu.commit_profile_bind_capture(menu_bind_mouse1));
        CHECK(menu.local_profiles()[1].bind=="MOUSE1");
        CHECK(menu.begin_profile_bind_capture(1u));
        CHECK(menu.cancel_profile_bind_capture());
        CHECK(menu.local_profiles()[1].bind=="MOUSE1");
        CHECK(menu.begin_profile_bind_capture(1u));
        CHECK(menu.commit_profile_bind_capture(0u)); // ESC clears
        CHECK(menu.local_profiles()[1].bind.empty());
        updated=frames.next(menu,&bridge,&nav);
        CHECK(updated && !updated->profile_bind_capture_index);
        CHECK(menu_profile_action_rect(*updated,
              panel->logical_width(),panel->logical_height(),1u,1u));
        // A repeated Default cannot be overwritten or deleted.
        CHECK(!menu.begin_profile_action_preview(
              0u,MenuProfileAction::Save));
        CHECK(!menu.begin_profile_action_preview(
              0u,MenuProfileAction::Delete));
        CHECK(!menu.begin_profile_action_preview(
              88u,MenuProfileAction::Load));
        CHECK(!menu.begin_profile_action_preview(
              1u,static_cast<MenuProfileAction>(44)));
        CHECK(menu.begin_profile_action_preview(
              1u,MenuProfileAction::Load));
        CHECK(!menu.begin_profile_action_preview(
              1u,MenuProfileAction::Delete));
        auto modal=frames.next(menu,&bridge,&nav);
        CHECK(modal && modal->profile_action_preview);
        CHECK(menu_profile_action_question(*modal)=="Load profile 'legit'?");
        const auto popup=menu_profile_modal_geometry(
            *modal,panel->logical_width(),panel->logical_height());
        CHECK(popup);
        const auto confirm=pixel(popup->continue_x+10.0f,
                                 popup->continue_y+10.0f);
        const auto hit=hit_test_menu(*modal,mode,
                                     confirm.first,confirm.second);
        CHECK(hit && hit->kind==MenuHitKind::ProfileModalContinue);
        // Modal shields the name field and every card icon.
        CHECK(!hit_test_menu(*modal,mode,inside.first,inside.second));
        CHECK(!menu.focus_profile_name_preview());
        CHECK(!menu.create_profile_preview());
        CHECK(menu.confirm_profile_action_preview());
        CHECK(menu.local_profiles().size()==2u);
        CHECK(!menu.profile_action_preview()); // no disk load
        CHECK(menu.begin_profile_action_preview(
              1u,MenuProfileAction::Save));
        modal=frames.next(menu,&bridge,&nav);
        CHECK(menu_profile_action_question(*modal)=="Save profile 'legit'?");
        CHECK(menu.cancel_profile_action_preview());
        CHECK(menu.begin_profile_action_preview(
              1u,MenuProfileAction::OpenFolder));
        modal=frames.next(menu,&bridge,&nav);
        CHECK(menu_profile_action_question(*modal)==
              "Open folder for profile 'legit'?");
        CHECK(menu.confirm_profile_action_preview());
        CHECK(menu.local_profiles().size()==2u); // folder not opened
        CHECK(menu.begin_profile_action_preview(
              1u,MenuProfileAction::Delete));
        modal=frames.next(menu,&bridge,&nav);
        CHECK(menu_profile_action_question(*modal)==
              "Delete profile 'legit'?");
        CHECK(menu.cancel_profile_action_preview());
        CHECK(menu.local_profiles().size()==2u);
        CHECK(menu.begin_profile_action_preview(
              1u,MenuProfileAction::Delete));
        CHECK(menu.confirm_profile_action_preview());
        CHECK(menu.local_profiles().size()==1u);
        CHECK(menu.local_profiles()[0].name=="Default");
        CHECK(!menu.confirm_profile_action_preview());
        // The name-input target is hidden when the synthetic capacity
        // of six cards has been reached, preventing offscreen actions.
        for(int n=0;n<5;++n) {
            CHECK(menu.focus_profile_name_preview());
            const std::string name="user"+std::to_string(n);
            for(char c:name) {
                const char v[1]{c};
                CHECK(menu.append_profile_name_preview(
                    std::string_view(v,1u)));
            }
            CHECK(menu.create_profile_preview());
        }
        CHECK(menu.local_profiles().size()==6u);
        CHECK(!menu.focus_profile_name_preview());
        CHECK(!menu.create_profile_preview());
        const auto full=frames.next(menu,&bridge,&nav);
        CHECK(full && full->profile_cards.size()==6u);
        CHECK(menu_profile_preview_valid(*full,
              panel->logical_width(),panel->logical_height()));
        CHECK(!menu_profile_input_rect(*full,panel->logical_width(),
              panel->logical_height()));
        CHECK(!menu_profile_create_rect(*full,panel->logical_width(),
              panel->logical_height()));
        // Each tab switch preserves local cards but never stores them.
        CHECK(nav.select_tab(MenuTab::Combat));
        CHECK(nav.select_tab(MenuTab::Profiles));
        CHECK(menu.local_profiles().size()==6u);
        CHECK(!menu.profile_name_focused());
        CHECK(frames.detach());
    }
    return 0;
}
