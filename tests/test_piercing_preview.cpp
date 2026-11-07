#include "slinkywinky/menu.hpp"
#include "slinkywinky/menu_navigation.hpp"
#include "slinkywinky/menu_profile.hpp"
#include "slinkywinky/menu_render.hpp"
#include "slinkywinky/menu_mouse_gesture.hpp"
#include "slinkywinky/menu_visual_geometry.hpp"
#include "slinkywinky/menu_hit_test.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <atomic>
#define CHECK(x) do {if(!(x))throw std::runtime_error( \
    "R124 line "+std::to_string(__LINE__)+": " #x);}while(false)

using namespace slinkywinky;

int main() {
    MenuState menu;
    MenuProfileBridge profile(menu);
    MenuNavigation nav(menu,profile);
    CHECK(menu.piercing_selected()==0);
    CHECK(menu.piercing_preview().sort==0);
    CHECK(menu.piercing_preview().enabled[6]); // source example default only
    for(std::size_t i=0;i<6;i++)CHECK(!menu.piercing_preview().enabled[i]);
    CHECK(!nav.apply(MenuNavAction::NextPiercingControl)); // wrong focus
    CHECK(!menu.activate_piercing_selection());
    CHECK(!menu.move_piercing_selection(0));
    CHECK(!menu.move_piercing_selection(2));
    CHECK(!nav.focus_visible_row(11)); // initially outside eight-row page
    CHECK(profile.scroll_by(4)); // show catalog items 4..11
    CHECK(nav.focus_visible_row(11));
    CHECK(!nav.apply(MenuNavAction::ActivatePiercingControl)); // collapsed
    CHECK(nav.apply(MenuNavAction::ExpandFocused));
    CHECK(menu.expanded(11));
    CHECK(nav.apply(MenuNavAction::ActivatePiercingControl));
    CHECK(menu.piercing_preview().sort==1); // Health
    CHECK(nav.apply(MenuNavAction::NextPiercingControl));
    CHECK(menu.piercing_selected()==1);
    CHECK(nav.apply(MenuNavAction::ActivatePiercingControl));
    CHECK(menu.piercing_preview().enabled[0]); // Non-player entities
    CHECK(nav.apply(MenuNavAction::PreviousPiercingControl));
    CHECK(menu.piercing_selected()==0);
    CHECK(!nav.apply(MenuNavAction::PreviousPiercingControl));
    CHECK(nav.apply(MenuNavAction::NextPiercingControl));
    for(std::size_t i=2;i<=7;i++)
        CHECK(nav.apply(MenuNavAction::NextPiercingControl));
    CHECK(menu.piercing_selected()==7 && menu.settings_offset(11)==5);
    CHECK(!nav.apply(MenuNavAction::NextPiercingControl)); // upper bound
    CHECK(nav.apply(MenuNavAction::ActivatePiercingControl));
    CHECK(!menu.piercing_preview().enabled[6]);
    const PiercingPreview saved=menu.piercing_preview();
    CHECK(saved.sort==1 && saved.enabled[0] && !saved.enabled[6]);
    const auto file=std::filesystem::temp_directory_path()/
        "slinkywinky-r124-piercing-sandbox-only.conf";
    std::error_code ignored;
    std::filesystem::remove(file,ignored);
    std::filesystem::remove(file.string()+".tmp",ignored);
    const auto text=serialize_piercing_preview(saved);
    CHECK(text.find("version=1\n")!=std::string::npos);
    CHECK(text.find("sort=1\n")!=std::string::npos);
    CHECK(text.find("flags=1000000\n")!=std::string::npos);
    CHECK(text.find("aim_enabled")==std::string::npos);
    CHECK(text.find("Minecraft")==std::string::npos);
    CHECK(save_piercing_preview(file,saved));
    PiercingPreview loaded{};
    CHECK(load_piercing_preview(file,loaded) && loaded==saved);
    CHECK(parse_piercing_preview(text,loaded));
    CHECK(loaded==saved);
    auto prior=loaded;
    for(const auto& malformed: {
        "", "version=2\nsort=1\nflags=1000000\n",
        "version=1\nsort=2\nflags=1000000\n",
        "version=1\nsort=1\nflags=100000\n",
        "version=1\nsort=1\nflags=10000x0\n",
        "version=1\nsort=1\nflags=1000000\nextra=yes\n",
        "version=1\nsort=1\nsort=0\nflags=1000000\n",
        "version=1\nsort=1\nflags=1000000",
        "version=1\nsort=1\nflags=1000000\nflags=1000000\n"
    }) {
        CHECK(!parse_piercing_preview(malformed,loaded));
        CHECK(loaded==prior); // no partial import
    }
    PiercingPreview invalid=saved;
    invalid.sort=2;
    CHECK(serialize_piercing_preview(invalid).empty());
    CHECK(!menu.adopt_piercing_preview(invalid));
    CHECK(menu.piercing_preview()==saved);
    CHECK(!save_piercing_preview(file,invalid));
    CHECK(menu.adopt_piercing_preview(loaded));
    CHECK(menu.piercing_selected()==0 && menu.settings_offset(11)==0);
    CHECK(menu.piercing_preview()==saved);

    // Snapshot values, selected callsite and mode are immutable after edits.
    MenuRenderSession frames;
    CHECK(frames.attach(124,900,630));
    auto oldFrame=frames.next(menu,&profile,&nav);
    CHECK(oldFrame && oldFrame->piercing_preview==saved);
    CHECK(oldFrame->piercing_selected==0);
    MenuMouseGesture gate;
    CHECK(gate.press(*oldFrame,MenuPanelMode::StandalonePreview,
                     40,30,100,true));
    CHECK(nav.apply(MenuNavAction::ActivatePiercingControl));
    auto newFrame=frames.next(menu,&profile,&nav);
    CHECK(newFrame && newFrame->piercing_preview.sort==0);
    CHECK(oldFrame->piercing_preview.sort==1);
    CHECK(!gate.release(*newFrame,MenuPanelMode::StandalonePreview,
                        40,30,120,true));
    CHECK(!gate.pending());
    CHECK(frames.detach());
    CHECK(nav.apply(MenuNavAction::ExpandFocused)); // collapse
    CHECK(!nav.apply(MenuNavAction::ActivatePiercingControl));
    CHECK(!nav.apply(MenuNavAction::NextPiercingControl));
    CHECK(nav.select_tab(MenuTab::Move));
    CHECK(!menu.activate_piercing_selection());
    CHECK(!menu.move_piercing_selection(1));

    std::atomic<bool> foreignRejected{false};
    std::thread foreign([&]{
        foreignRejected=!nav.apply(MenuNavAction::NextPiercingControl) &&
                        !nav.apply(MenuNavAction::ActivatePiercingControl);
    });
    foreign.join();
    CHECK(foreignRejected);
    {
        std::ofstream broken(file,std::ios::binary|std::ios::trunc);
        broken<<"version=1\nsort=1\nflags=1111111\ninvalid=1\n";
    }
    CHECK(!load_piercing_preview(file,loaded) && loaded==prior);
    {
        std::ofstream huge(file,std::ios::binary|std::ios::trunc);
        huge<<std::string(513,'x');
    }
    CHECK(!load_piercing_preview(file,loaded) && loaded==prior);

    // R125 exact drawn control geometry, not the module card header,
    // is clickable in every standalone/bounded/compact preview mode.
    // Page boundaries, group headings, and held-click identity are strict.
    for(const auto mode:{MenuPanelMode::StandalonePreview,
                         MenuPanelMode::BoundedOverlay,
                         MenuPanelMode::CompactOverlay}) {
        MenuState clicked;
        MenuProfileBridge clickedProfile(clicked);
        MenuNavigation clickedNav(clicked,clickedProfile);
        CHECK(clickedProfile.scroll_by(9)); // Piercing is row 2 in all 3
        CHECK(clickedNav.focus_visible_row(11));
        CHECK(clickedNav.apply(MenuNavAction::ExpandFocused));
        MenuRenderSession clickedFrames;
        CHECK(clickedFrames.attach(125,900,630));
        const auto panel=menu_panel_geometry(900,630,false,mode);
        CHECK(panel);
        const auto translate=[&](float x,float y) {
            return std::pair<int,int>{
                static_cast<int>(panel->left+
                    std::lround(panel->scale*x)),
                static_cast<int>(panel->top+
                    std::lround(panel->scale*y))
            };
        };
        const auto frame=clickedFrames.next(
            clicked,&clickedProfile,&clickedNav);
        CHECK(frame && frame->visible.size()>2 &&
              frame->visible[2].catalog_index==11 &&
              frame->visible[2].expanded);
        const auto card=menu_module_card_rect(
            *frame,panel->logical_width(),2);
        CHECK(card.y+card.height<
              panel->logical_height()-menu_panel_padding);
        const auto controlX=card.x+card.width*0.54f;
        const auto controlW=std::max(16.0f,card.width*0.46f-10.0f);
        const auto firstY=card.y+menu_row_body_height(*frame)+43.0f+2.0f;
        auto point=translate(controlX+controlW*0.5f,firstY+10.0f);
        auto sourceHit=hit_test_menu(*frame,mode,point.first,point.second);
        CHECK(sourceHit && sourceHit->kind==MenuHitKind::PiercingControl &&
              sourceHit->catalog_index==11 && sourceHit->control_index==0);
        auto wrongLabel=translate(card.x+35.0f,firstY+10.0f);
        CHECK(!hit_test_menu(*frame,mode,
              wrongLabel.first,wrongLabel.second));
        const auto wrongHeader=translate(controlX+12.0f,
            card.y+menu_row_body_height(*frame)+12.0f);
        CHECK(!hit_test_menu(*frame,mode,
              wrongHeader.first,wrongHeader.second));
        MenuMouseGesture clickGate;
        CHECK(!clickGate.press(*frame,mode,point.first,point.second,
                               100,false));
        CHECK(clickGate.press(*frame,mode,point.first,point.second,
                              200,true));
        CHECK(!clickGate.move(*frame,mode,
             wrongLabel.first,wrongLabel.second,true));
        CHECK(!clickGate.release(*frame,mode,point.first,point.second,
                                 220,true));
        CHECK(clickGate.press(*frame,mode,point.first,point.second,
                              1000,true));
        const auto committed=clickGate.release(*frame,mode,
                        point.first,point.second,1020,true);
        CHECK(committed && committed->control_index==0);
        CHECK(clickedNav.focus_visible_row(committed->catalog_index));
        CHECK(clicked.select_piercing_control(committed->control_index));
        CHECK(clickedNav.apply(MenuNavAction::ActivatePiercingControl));
        CHECK(clicked.piercing_preview().sort==1); // Health option
        const auto changed=clickedFrames.next(
            clicked,&clickedProfile,&clickedNav);
        CHECK(changed && changed->piercing_preview.sort==1);
        CHECK(!hit_test_menu(*changed,mode,
              wrongLabel.first,wrongLabel.second));
        // A held press from prior snapshot cannot switch another value.
        CHECK(clickGate.press(*changed,mode,point.first,point.second,
                              2000,true));
        CHECK(clickedNav.apply(MenuNavAction::ActivatePiercingControl));
        const auto stale=clickedFrames.next(
            clicked,&clickedProfile,&clickedNav);
        CHECK(stale && !clickGate.release(*stale,mode,
                  point.first,point.second,2020,true));
        CHECK(!clickGate.pending());
        // Boolean switch: only its 34x18 right-side pill is actionable.
        const auto boolY=firstY+27.0f;
        const auto boolP=translate(controlX+controlW-17.0f,boolY+10.0f);
        const auto boolLabel=translate(card.x+35.0f,boolY+10.0f);
        const auto state=clickedFrames.next(
            clicked,&clickedProfile,&clickedNav);
        CHECK(state && !state->piercing_preview.enabled[0]);
        auto boolHit=hit_test_menu(*state,mode,boolP.first,boolP.second);
        CHECK(boolHit && boolHit->kind==MenuHitKind::PiercingControl &&
              boolHit->control_index==1);
        CHECK(!hit_test_menu(*state,mode,
              boolLabel.first,boolLabel.second));
        // Moving between separate switches must not keep the hit.
        const auto nextBool=translate(controlX+controlW-17.0f,
                                     boolY+27.0f+10.0f);
        auto nextHit=hit_test_menu(*state,mode,
                                  nextBool.first,nextBool.second);
        CHECK(nextHit && nextHit->control_index==2);
        CHECK(clickGate.press(*state,mode,boolP.first,boolP.second,
                              2600,true));
        CHECK(!clickGate.release(*state,mode,
              nextBool.first,nextBool.second,2630,true));
        CHECK(!clickGate.pending() && !clicked.piercing_preview().enabled[0]);
        // Failed cross-target input is consumed before any new click.
        CHECK(clickGate.press(*state,mode,boolP.first,boolP.second,
                              3000,true));
        auto boolCommit=clickGate.release(*state,mode,
                                boolP.first,boolP.second,3020,true);
        CHECK(boolCommit && boolCommit->control_index==1);
        CHECK(clicked.select_piercing_control(boolCommit->control_index));
        CHECK(clickedNav.apply(MenuNavAction::ActivatePiercingControl));
        CHECK(clicked.piercing_preview().enabled[0]);
        CHECK(!clicked.select_piercing_control(7)); // not visible page
        CHECK(!clicked.select_piercing_control(8)); // malformed index
        // Second five-item page: condition heading inserts 20px before j1.
        CHECK(clicked.change_settings_page(11,1));
        const auto page2=clickedFrames.next(
            clicked,&clickedProfile,&clickedNav);
        CHECK(page2 && page2->visible[2].settings_offset==5);
        const auto card2=menu_module_card_rect(
            *page2,panel->logical_width(),2);
        CHECK(card2.y+card2.height<
              panel->logical_height()-menu_panel_padding);
        const float lastControlY=card2.y+
              menu_row_body_height(*page2)+43.0f+2.0f+27.0f*2.0f+20.0f;
        const auto lastP=translate(card2.x+
              card2.width*0.54f+std::max(16.0f,card2.width*0.46f-10.0f)-17.0f,
              lastControlY+10.0f);
        const auto lastHit=hit_test_menu(*page2,mode,
                                        lastP.first,lastP.second);
        CHECK(lastHit && lastHit->kind==MenuHitKind::PiercingControl &&
              lastHit->control_index==7);
        const auto groupTitle=translate(card2.x+35.0f,
              card2.y+menu_row_body_height(*page2)+43.0f+27.0f);
        CHECK(!hit_test_menu(*page2,mode,groupTitle.first,groupTitle.second));
        const auto outside=translate(card2.x+card2.width+3.0f,
                                    lastControlY+10.0f);
        CHECK(!hit_test_menu(*page2,mode,outside.first,outside.second));
        const auto tampered=[&](std::size_t offset) {
            auto bad=*page2;
            bad.visible[2].settings_offset=offset;
            return !hit_test_menu(bad,mode,lastP.first,lastP.second);
        };
        CHECK(tampered(1) && tampered(10));
        const auto wrongTab=[&]{
            auto bad=*page2;bad.tab=MenuTab::Move;
            return !hit_test_menu(bad,mode,lastP.first,lastP.second);
        };
        CHECK(wrongTab());
        CHECK(clickedFrames.detach());
    }
    std::filesystem::remove(file,ignored);
    std::filesystem::remove(file.string()+".tmp",ignored);
    return 0;
}
